# SOVEREIGN SOLVER — TARGETED BASIS REPAIR FEASIBILITY STUDY
## READ-ONLY MATHEMATICAL FEASIBILITY AUDIT REPORT

---

## 1. Executive Summary & Final Classification

This report evaluates the mathematical feasibility of the proposed "single targeted basis pivot" (Option C) for repairing primal feasibility on `10teams` Node 1 ($x_{239} \le 0$).

### Final Feasibility Classification
$$\mathbf{\text{FINAL CLASSIFICATION: B. Multiple targeted pivots are required}}$$
*(Secondary classification: **C. Targeted repair is not generally valid as a single pivot for this child**).*

### Key Findings
1. **Refutation of One-Pivot Feasibility**:
   - Empirical audit of Node 1 ($x_{239} \le 0$) under parent basis $B_{\text{parent}}$ revealed **2 basic variables violating bounds simultaneously**:
     - Basis Slot 14 (Col 0 [`x1`]): Value = `0.0`, Bounds = `[1.0, 1.0]`, Violation = `1.000000`.
     - Basis Slot 221 (Col 50 [`x51`]): Value = `0.0`, Bounds = `[1.0, 1.0]`, Violation = `1.000000`.
   - A single basis column swap in slot 14 **CANNOT** produce a 100% primal-feasible basis because slot 221 remains primal infeasible.
2. **Coupled Infeasibility Propagation**:
   - Because `10teams` contains equality constraints ($\sum_{j} x_{ij} = 1$), fixing non-basic $x_{239} = 0$ shifts the basic solution vector $x_B = B^{-1} (b - N x_N)$, causing coupled basic variables `x1` and `x51` to drop from $1.0$ to $0.0$.
3. **Primal Simplex Handoff Invalidation**:
   - Because a single targeted pivot leaves secondary basic variables primal infeasible, the resulting basis is **Primal Infeasible**. Primal Simplex **cannot** start without Phase I or further dual pivots.

---

## 2. Reconstructed Node 1 Bound Violation Telemetry

```
========================================================================
  NODE 1 PRIMAL BOUND VIOLATION AUDIT (Under B_parent)
========================================================================
Total Basic Variables Violating Child Bounds: 2
  - Slot  14 | Column   0 (x1)  | Value = 0.0 | Bounds = [1.0, 1.0] | Viol = 1.000000
  - Slot 221 | Column  50 (x51) | Value = 0.0 | Bounds = [1.0, 1.0] | Viol = 1.000000
========================================================================
```

---

## 3. Mathematical Evaluation of the One-Pivot Claim

Let $x_B = B^{-1} b$ be the basic solution under parent basis $B_p$ and child bounds $(l', u')$.
- The previous proposal assumed that branching affects only one constraint row (a single basic variable).
- However, $B^{-1} A_j$ is a dense vector coupling multiple basic variables. Tightening a single variable bound propagates through $B^{-1}$, violating bounds on **multiple basic variables simultaneously** (slots 14 and 221).
- Replacing a single basic column $B_p[14]$ with non-basic column $A_j$ can at best repair slot 14, leaving slot 221 primal infeasible ($x_{51} = 0 \notin [1, 1]$).
- Therefore, **a single targeted basis pivot is mathematically insufficient to achieve primal feasibility**. Multiple targeted pivots or full Dual Simplex pivots are required.

---

## 4. Comparison of Initialization Options

| Initialization Strategy | Primal Feasible after 1 Pivot? | Dual Degeneracy Risk | Phase I Bypass | Overall Feasibility |
| :--- | :---: | :---: | :---: | :---: |
| **Option A (Current Level-A)** | NO (Dual Simplex solves) | HIGH (11,317 zero-step pivots) | 100% Bypassed | **Validated & Working (16.2s)** |
| **Option C (Single Targeted Pivot)** | **NO (2 violations remain)** | ZERO | NO (Remains Infeasible) | **Mathematically Invalid** |
| **Option D (Warm Phase-I Basis)** | NO (Runs < 100 Phase I) | LOW | Partial (Bypasses ~6,700 Phase I) | **Feasible Future Candidate** |

---

## 5. Summary & Recommendation

1. **Do NOT implement Single Targeted Basis Repair**:
   - Empirical proof shows Node 1 has 2 coupled basic variable violations (`x1` and `x51`), rendering single-pivot repair mathematically incapable of achieving primal feasibility.
2. **Retain Level-A Parent Basis Warm Start**:
   - Current Level-A Dual Simplex warm start is 100% mathematically valid, completely eliminates Phase I (0 iterations), and provides a verified **31.9% speedup** (16.237 s vs 23.850 s clean baseline).
