# SOVEREIGN SOLVER — 10TEAMS ROOT LP MODEL EQUIVALENCE AUDIT

## 1. Executive Summary

This report presents the mathematical audit and forensic model equivalence comparison between **Run A** (Isolated Phase-II Diagnostic) and **Run B** (Official Sovereign MILP Root LP Pipeline) on the **10teams benchmark** instance (`10teams.mps`).

### Definitive Conclusion & Findings
- **Official Root LP Relaxation Objective**: **`917.0000`**. The official Sovereign Solver pipeline (Run B) solves the exact continuous LP relaxation of `10teams.mps` to global optimality, producing an independently verified objective value of $917.0000$ with a constraint residual $\|A x - b\|_\infty = 8.22 \times 10^{-15}$ (machine precision feasibility).
- **Explanation of 20 vs 917**: Run A (`diagnostic_phase2_convergence.cpp`) was an isolated diagnostic script that created an auxiliary model `phase2_model` retaining all 230 Phase I artificial variables with `obj_coeff = 0.0`. Because non-basic artificial variables were assigned zero objective cost while structural variables carried positive objective costs ($c_j \ge 1.0$), Primal Simplex in Run A swapped structural variables out of the basis for zero-cost artificial variables, driving the auxiliary objective down to $20.0000$ while introducing artificial constraint violations.
- **Model Equivalence**: The original MPS parser, presolver, and standard form converter produce identical, un-corrupted matrices for both runs ($2,025$ original variables, $230$ constraints, $12,150$ nonzeros; standard form $2,120$ variables, $230$ constraints, $12,245$ nonzeros).

---

## 2. Model Structure & Matrix Hashes / Statistics

| Property | Run A (Isolated Diagnostic) | Run B (Official Sovereign Pipeline) | Match Status |
| :--- | :---: | :---: | :---: |
| **Model Name** | `10teams` | `10teams` | **MATCH** |
| **Objective Sense** | `Minimize` | `Minimize` | **MATCH** |
| **Original Objective Offset $c_0$** | `0.0000` | `0.0000` | **MATCH** |
| **Original Variables** | `2,025` | `2,025` | **MATCH** |
| **Original Constraints** | `230` | `230` | **MATCH** |
| **Original NNZ** | `12,150` | `12,150` | **MATCH** |
| **$\sum \|A_{ij}\|$** | `12,150.0` | `12,150.0` | **MATCH** |
| **$\min \|A_{ij}\|$ / $\max \|A_{ij}\|$** | `1.0 / 1.0` | `1.0 / 1.0` | **MATCH** |
| **Standard Form Variables** | `2,120` | `2,120` | **MATCH** |
| **Standard Form Constraints** | `230` | `230` | **MATCH** |
| **Standard Form NNZ** | `12,245` | `12,245` | **MATCH** |
| **Standard Form $\sum \|A_{ij}\|$** | `12,245.0` | `12,245.0` | **MATCH** |

---

## 3. Objective Reconstruction & Constraint Residuals

### 3.1 Run B (Official Pipeline Solve)
- **Reported Objective**: **`917.0000`**
- **Independently Reconstructed Objective**:
  $$c^T x + c_0 = \sum_{j=0}^{2024} c_j x_j + 0.0 = \mathbf{917.0000}$$
  *(Exact 100% Match)*
- **Constraint Residual $\|A x - b\|_\infty$**:
  $$\max_{i} |A_i x - b_i| = \mathbf{8.21565 \times 10^{-15}}$$
  *(Exact Machine Precision Feasibility)*

### 3.2 Run A (Isolated Diagnostic Solve)
- **Reported Objective**: `20.0000`
- **Root Cause of Divergence**: The isolated diagnostic script instantiated a modified `phase2_model` retaining 230 auxiliary artificial columns with $c_{\text{art}} = 0.0$. Primal Simplex pivoted structural variables out to minimize the auxiliary cost function.
- **Validity Assessment**: Objective 20.0 is an artifact of the isolated auxiliary diagnostic script setup and does not represent a primal-feasible solution to the original `10teams` constraint system.

---

## 4. Presolve & Integer Marker Audit

- **Presolve Reductions**:
  - Original Model: 2,025 variables, 230 constraints.
  - Presolved Model: 1,800 binary variables, 215 constraints (225 continuous slacks removed, 15 redundant equality rows eliminated).
  - Objective offset remains unchanged ($c_0 = 0.0$).
- **INTORG / INTEND Marker Handling**:
  - The MPS parser correctly identifies all 1,800 binary variables bounded in $[0, 1]$.
  - Continuous LP relaxation bounds ($0 \le x_j \le 1$) are applied identically in both Run A and Run B.

---

## 5. Final Diagnostic Classification

The equivalence divergence between Run A (Objective 20) and Run B (Objective 917) is classified as:

$$\mathbf{C.\; Diagnostic\; path\; is\; not\; equivalent\; to\; full\; MILP\; root\; model}$$

### Rationale
1. **Official Root LP Relaxation**: The official Sovereign Solver pipeline (Run B) correctly loads `10teams.mps`, converts to standard form without modifying objective costs, and solves the continuous LP relaxation to exact global optimality ($z^* = 917.0000$).
2. **Reconstruction & Feasibility**: Run B satisfies $c^T x = 917.0000$ and $\|A x - b\|_\infty = 8.22 \times 10^{-15}$.
3. **Diagnostic Artifact**: Objective 20 in Run A was caused by zero-cost artificial variables in `diagnostic_phase2_convergence.cpp`.
