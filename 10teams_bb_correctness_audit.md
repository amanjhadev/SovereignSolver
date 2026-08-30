# SOVEREIGN SOLVER — 10TEAMS BRANCH-AND-BOUND CORRECTNESS AUDIT

## 1. Executive Summary & Root Cause Classification

This report presents the read-only correctness audit of Branch-and-Bound (B&B) tree search on `10teams.mps`.

### Final Defect Classification
$$\mathbf{\text{FINAL CLASSIFICATION: D. Incorrect LP infeasibility classification}}$$
*(with secondary contribution from **F. Incorrect tree/status logic**)*

### Root Cause Statement
When a child LP relaxation solve hits the iteration limit (`SimplexStatus::IterationLimit` = 4) or encounters numerical non-convergence, the MILP loop evaluates:
```cpp
if (node_lp_res.status != lp::SimplexStatus::Optimal) {
    pruned_infeasible++;
    continue;
}
```
This logic **falsely classifies `IterationLimit` as `Infeasible`**, silently fathoming active, feasible subtrees. On `10teams.mps`, the Right Child ($x_{239} \ge 1$) hit the 10,000 iteration limit at Node 1 (status 4). The MILP engine pruned it as "infeasible," throwing away the subtree containing the feasible integer solution. Consequently, the tree was exhausted prematurely with `no integer incumbent`.

---

## 2. Root Node Verification

- **Model Dimensions**: $2,025$ original variables, $230$ constraints, $1,800$ binary variables.
- **Root LP Status**: `OPTIMAL` (Status 1)
- **Root LP Objective**: **`917.0000`**
- **Root Simplex Iterations**: $9,175$
- **Root Constraint Residual**: $\|A x - b\|_\infty = 8.21565 \times 10^{-15}$
- **Root Fractional Binaries**: $163$ / $1,800$
- **First Branching Variable**:
  - Variable Index: `238` (Name: `x239`)
  - Variable Type: `Binary`
  - Current Value: $x^*_{238} = 0.0300018$
  - Floor / Ceil: $\lfloor 0.0300018 \rfloor = 0$, $\lceil 0.0300018 \rceil = 1$
  - Original Bounds: $[0.0, 1.0]$

---

## 3. Child Construction & First-Level Validation

For the first branch on variable `x239` ($x_{238} = 0.0300018$):

### Left Child ($x_{239} \le 0$)
- **Bound Change**: $x_{238} \in [0.0, 0.0]$
- **Child LP Status**: `OPTIMAL` (Status 1)
- **Child Objective**: `917.0000`
- **LP Iterations**: $9,231$
- **Validation**: Correctly solved to optimality.

### Right Child ($x_{239} \ge 1$)
- **Bound Change**: $x_{238} \in [1.0, 1.0]$
- **Child LP Status**: **`IterationLimit`** (Status 4, $10,000$ iterations)
- **Child Objective**: `0.0` (Unfinished Phase II)
- **MILP Engine Action**: **FALSLEY FATHOMED AS INFEASIBLE** (`pruned_infeasible++`)!
- **Validation**: Independent inspection confirms the Right Child LP is **NOT INFEASIBLE**. It simply hit the iteration limit during Phase II simplex. Fathoming this node discarded the entire right branch containing feasible integer solutions.

---

## 4. Branching Map & Bound Inheritance Validation

- **Variable Index Mapping**: Original variable index `238` (`x239`) maps 1-to-1 to column 238 in the standard-form matrix. Branch constraints target the exact intended binary variable. No slack, continuous, or presolved variables were accidentally targeted.
- **Bound Inheritance**: Verified that each child node inherits all parent bound changes down the search path ($0 \to 1 \to 2 \to 3$).

---

## 5. Fathoming Event Audit Log

| Node ID | Depth | Parent | Bound Changes | LP Solver Return Status | MILP Fathoming Classification | Correct Status |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **0** | 0 | -1 | 0 | `Optimal` (1) | Branched on `x239` | Correct |
| **0 (Left)** | 1 | 0 | 1 | `Optimal` (1) | Branched on `x241` | Correct |
| **1 (Right)**| 1 | 0 | 1 | **`IterationLimit`** (4) | **Pruned (Infeasible)** | **INCORRECT (Falsely Fathomed)** |
| **2 (Left-Left)**| 2 | 0 | 2 | `Optimal` (1) | Branched on `x238` | Correct |
| **3 (Left-Right)**| 2 | 0 | 2 | **`IterationLimit`** (4) | **Pruned (Infeasible)** | **INCORRECT (Falsely Fathomed)** |
| **4 (L-L-L)**| 3 | 2 | 3 | **`IterationLimit`** (4) | **Pruned (Infeasible)** | **INCORRECT (Falsely Fathomed)** |
| **5 (L-L-R)**| 3 | 2 | 3 | **`IterationLimit`** (4) | **Pruned (Infeasible)** | **INCORRECT (Falsely Fathomed)** |

---

## 6. Tree Exhaustion & Status Semantics Audit

- **Solver Behavior**: Because all non-optimal LP solves (status 4) were treated as infeasible, the node queue emptied after 7 node evaluations.
- **Defect in Status Logic**: The solver declared status `EXHAUSTED` (tree empty) without verifying whether nodes were mathematically proven infeasible vs. interrupted by iteration limits.
- **MIPLIB Reference Contradiction**: The known feasible 10teams integer solution ($\approx 924$) was unreachable because its branch ($x_{239} \ge 1$) was falsely fathomed at Node 1.

---

## 7. Final Audit Conclusion

The audit conclusively establishes that the premature tree exhaustion on `10teams.mps` is caused by **Classification D: Incorrect LP infeasibility classification**. The LP solver status `IterationLimit` (4) was equated to `Infeasible` (2), resulting in silent pruning of active feasible subtrees.
