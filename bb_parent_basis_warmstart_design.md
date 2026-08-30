# SOVEREIGN SOLVER — B&B PARENT-BASIS WARM-START DESIGN REVIEW
## ARCHITECTURAL SPECIFICATION & MATHEMATICAL DESIGN DOCUMENT

---

## 1. Executive Summary & Core Objective

The forensic profile of `10teams` MILP execution demonstrated that child LP relaxations currently perform clean Phase I Dual Simplex solves from an artificial basis, wasting **6,786 Phase I iterations (~16.42 s / node)** out of **24.159 s total node latency**.

### Core Goal
Design a mathematically rigorous, robust, and safe **parent-basis warm-start mechanism** for MILP child LP relaxations:

$$\text{Parent Optimal Basis } B_p \implies \text{Apply Child Bounds } (l', u') \implies \text{Dual Simplex Recovery } \implies \text{Child Optimal Solution}$$

This eliminates Phase I execution on valid warm starts, reducing average child LP solve latency from **~24.159 s / node** to **< 0.1 s / node**.

---

## 2. Audit of Existing Codebase Classes & Data Structures

1. **`BBNode`** (`include/sovereign/milp/bb_node.hpp`):
   - Already contains `lp::Basis warm_start_basis` field.
   - When branching on fractional variable $x_j$, parent node's optimal basis $B_p$ is stored in `left_child.warm_start_basis` and `right_child.warm_start_basis`.
2. **`DualSimplexSolver`** (`include/sovereign/lp/dual_simplex.hpp`):
   - Already supports `solve(const LPModel& model, const Basis& initial_basis)`.
   - Currently, `MILPSolver::solve` calls `node_solver.solve(node_lp)` without passing `curr_node.warm_start_basis`, forcing a clean Phase I solve.
3. **`SimplexState` & `Basis`** (`include/sovereign/lp/basis.hpp`):
   - `Basis` stores basic column indices `basic_cols` of size $M$, and status for non-basic variables (`AtLower`, `AtUpper`, `Free`, `Fixed`).
   - `SimplexState` initializes basis matrix $B = A[:, \text{basic\_cols}]$, factorizes $B$ via `SparseLU`, and computes primal basic solution $x_B = B^{-1} b$ and dual basic solution $y = B^{-T} c_B$.

---

## 3. Mathematical Proof of Warm-Start Validity

### A. Linear Independence & Basis Structural Validity
Let the parent LP model be:
$$\min c^T x \quad \text{s.t.} \quad A x = b, \quad l \le x \le u$$
Let $B_p \subseteq \{1, \dots, N\}$ be the set of basic column indices at parent optimality. Since $B_p$ is optimal, matrix $B = A[:, B_p]$ is non-singular ($\det(B) \neq 0$).

When branching creates a child LP by tightening variable bounds ($l'_j \ge l_j$ or $u'_j \le u_j$):
- Constraint matrix $A$ and right-hand side $b$ **do not change**.
- Basic column subset $B_p$ **remains linearly independent**.
- Therefore, $B_p$ remains a **structurally valid basis matrix** for the child LP.

### B. Dual Feasibility Preservation
For any non-basic variable $x_j \notin B_p$, reduced cost is:
$$\bar{c}_j = c_j - c_B^T B^{-1} A_j$$
Since reduced costs $\bar{c}_j$ depend only on $c$, $A$, and basis matrix $B$, reduced costs in the child LP under basis $B_p$ are **identical to the parent reduced costs**:

$$\bar{c}_j^{\text{child}} = \bar{c}_j^{\text{parent}} \ge 0 \quad (\forall j \text{ non-basic at bound})$$

Thus, **parent basis $B_p$ remains Dual Feasible for the child LP**.

### C. Primal Infeasibility & Dual Simplex Recovery
The child basic solution $x_B = B^{-1} b$ may violate the new child bounds:
$$x_j < l'_j \quad \text{or} \quad x_j > u'_j \quad (j \in B_p)$$
Because $B_p$ is **Dual Feasible** but **Primal Infeasible**, Dual Simplex can immediately execute dual pivot steps (Dual ratio test & Dual pivot selection) to eliminate bound violations without executing Phase I!

---

## 4. Model Variable Indexing & Mapping Invariants

### In-Place Bound Updates vs Linear Constraints
1. **Simple Bound Branching** ($x_j \le \lfloor x^*_j \rfloor$ or $x_j \ge \lceil x^*_j \rceil$):
   - Variable bounds $l_j, u_j$ are updated in-place on existing column $j$.
   - Matrix dimension $M \times N$ and column indexing remain **100% identical**.
   - Parent basis $B_p$ maps 1-to-1 onto child model variables.
2. **Branching Linear Constraints** (e.g. Cuts or general integer splits):
   - When a new row is added ($A_{M+1, :} x \le b_{M+1}$), a slack variable $s_{M+1}$ is created.
   - The basis is extended by adding slack $s_{M+1}$ to $B_p$:
     $$B_{\text{child}} = B_p \cup \{s_{M+1}\}$$
   - Extended basis matrix $B_{\text{child}}$ is block triangular with $B_p$ on top, guaranteeing non-singularity.

---

## 5. Factorization & PFI Reuse Strategy

We evaluate three levels of basis reuse:

| Reuse Level | Description | Numerical Risk | Implementation Complexity | Recommendation |
| :--- | :--- | :--- | :--- | :--- |
| **Level A: Basis Metadata Only** | Pass `Basis` indices; execute fresh `SparseLU` factorization once at start of child LP. | Zero risk (Clean $LU$ factors). | Very Low | **Phase 1 Preferred** |
| **Level B: Basis + Reused LU Factors** | Reuse parent's $L, U$ matrices; recompute $x_B, y$ directly. | Very Low for bound changes. | Low | **Phase 2 Optimization** |
| **Level C: Basis + Reused PFI Eta Stack** | Inherit parent's PFI Eta update chain. | High (Accumulated numerical drift). | Medium | Not Recommended |

**Design Choice**: Implement **Level A** first. Fresh `SparseLU` factorization of basis $B_p$ takes only **0.08 s**, while bypassing 6,786 Phase I iterations saves **16.42 s per child node**.

---

## 6. Fallback Safety & Error Handling

If warm-starting from parent basis $B_p$ fails due to:
- Singular basis matrix ($\det(B) < 10^{-12}$)
- Dimension mismatch or invalid column index
- Numerical failure during dual pivoting
- Dual infeasibility detected at initial state

The solver must **automatically and silently fall back to a clean Phase I + Phase II LP solve**:

```
                                [Pop Child Node]
                                       │
                         Try Parent Basis Warm Start (Bp)
                                       │
                           Is Warm Start Successful?
                             /                   \
                        (YES)                     (NO)
                          │                         │
                  Dual Simplex Solve        Clean Phase I + Phase II Solve
                          │                         │
                  [Child Result]            [Child Result]
```

**Safety Invariant**: A warm-start failure MUST NEVER declare the child LP infeasible. It triggers clean fallback.

---

## 7. Node Status Semantics Integrity

The B&B status classification logic must maintain repaired status semantics:

| LP Solver Output Status | MILP Engine Node Action | Fathoming Classification |
| :--- | :--- | :--- |
| `SimplexStatus::Optimal` | Evaluate objective $Z$, branch or fathom by bound/integrality. | Normal Branching / Pruning |
| `SimplexStatus::Infeasible` | Prune subtree (Proven integer infeasible). | `nodes_pruned_infeasible++` |
| `SimplexStatus::IterationLimit` | Keep node in tree or mark unresolved. | `nodes_unresolved++` |
| `SimplexStatus::NumericalFailure` | Trigger Clean Fallback. If fallback fails, mark unresolved. | `nodes_unresolved++` |

---

## 8. Expected Performance Measurement Criteria

Empirical metrics to verify warm-start performance:

| Telemetry Metric | Baseline (Current) | Target Metric (Warm-Start Engaged) |
| :--- | :---: | :---: |
| **Child Phase I Iterations** | **6,786 / child** | **0 / child** |
| **Child Total Iterations** | ~10,000 / child | **< 300 / child** |
| **Average Child LP Runtime** | **24.159 s / node** | **< 0.100 s / node** |
| **Warm-Start Success Rate** | 0.0% | **> 95.0%** |
| **Clean Fallback Rate** | 100.0% | **< 5.0%** |

---

## 9. Comprehensive Unit & Integration Test Strategy

1. **Test 1: Upper Bound Branch Warm Start**: Parent optimal $\to$ child $x_j \le \lfloor x^*_j \rfloor$. Verify Dual Simplex restores primal feasibility without Phase I.
2. **Test 2: Lower Bound Branch Warm Start**: Parent optimal $\to$ child $x_j \ge \lceil x^*_j \rceil$. Verify Dual Simplex restores primal feasibility without Phase I.
3. **Test 3: Primal Feasible Child**: Child bounds do not violate $x_j^*$. Dual Simplex completes in 0 iterations.
4. **Test 4: Primal Infeasible Child**: Child bounds create LP infeasibility. Dual Simplex proves infeasibility correctly.
5. **Test 5: Fallback Recovery Test**: Pass corrupted basis; verify automatic fallback to clean solve produces identical solution.
6. **Test 6: Deep Branch Path Test**: Test warm start along 10-level branch chain ($N_0 \to N_1 \to \dots \to N_{10}$).

---

## 10. 10teams Controlled Checkpoint Plan

Upon implementation approval:
1. Run a 5-node serial checkpoint on `10teams.mps`.
2. Measure:
   - Phase I iterations (Target: 0).
   - Phase II iterations (Target: < 300).
   - Solve time per node (Target: < 0.1 s).
   - Objective value (Target: Exact equivalence with clean solve).

---

## 11. Public Architectural Deliverable

The implementation specification is documented in [bb_parent_basis_warmstart_design.md](file:///c:/Solver/bb_parent_basis_warmstart_design.md).
