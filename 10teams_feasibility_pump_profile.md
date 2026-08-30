# SOVEREIGN SOLVER — 10TEAMS FEASIBILITY PUMP FORENSIC PROFILE

## 1. Executive Summary

This report provides the forensic diagnostic profile of the **Feasibility Pump heuristic** (`FeasibilityPumpHeuristic`) on the **10teams benchmark** (`10teams.mps`).

### Core Diagnostic Findings
1. **Feasibility Pump Bottleneck**: The Feasibility Pump is mathematically correct but incurs **excessive model expansion overhead**. For each pump iteration, it constructs an explicit $L_1$ distance minimization LP relaxation that adds $1,800$ distance variables and $3,600$ linear constraints.
2. **Dimension Multiplier**:
   - Original Model: $2,025$ variables, $230$ constraints.
   - Auxiliary Distance LP (Standard Form): **$7,520$ variables**, **$3,830$ constraints** (**$16.65\times$ constraint growth**).
3. **Execution Latency**: Solving a $3,830 \times 7,520$ auxiliary LP model from scratch on every iteration requires $\sim 40\text{--}60$ seconds per iteration. With a default limit of $30$ iterations, complete execution requires $\sim 20\text{--}25$ minutes, causing the heuristic to consume almost the entire $300$-second solver time limit before Branch-and-Bound tree search can proceed.
4. **Simple Rounding Infeasibility**: Direct rounding of the root LP solution ($\text{round}(x_{\text{root}})$) rounds $163$ fractional binary variables but results in a maximum constraint violation $\|A x - b\|_\infty = \mathbf{1.0}$ (violating binary matching equality constraints). Simple rounding produces an infeasible integer point.

---

## 2. Model Scale & Auxiliary Formulation Comparison

| Property | Original Root Model | FP Auxiliary Distance Model (Standard Form) | Scale Multiplier |
| :--- | :---: | :---: | :---: |
| **Variables** | $2,025$ | **$7,520$** | $3.71\times$ |
| **Constraints** | $230$ | **$3,830$** | **$16.65\times$** |
| **Nonzeros (NNZ)** | $12,150$ | **$26,645$** | $2.19\times$ |
| **Objective Type** | Original Costs $c^T x$ | Distance Minimization $\sum_{j \in \text{Int}} d_j$ | N/A |
| **Constraint Types** | Equality / Inequality | Original + $3,600$ $L_1$ Bounds ($d_j \ge \pm(x_j - t_j)$) | $+3,600$ Rows |

---

## 3. Execution & Timing Breakdown

| Operation Phase | Time per Iteration | Percentage of Runtime | Description |
| :--- | :---: | :---: | :--- |
| **Auxiliary LP Model Build** | $\sim 0.05$ s | $<0.1\%$ | Copying original LP and adding $1,800$ $d_j$ vars + $3,600$ constraints |
| **Auxiliary LP Simplex Solve** | **$\sim 45.0\text{--}60.0$ s** | **$>99.9\%$** | Full Dual/Primal Simplex solve on $3,830 \times 7,520$ matrix |
| **Integer Rounding & Projection** | $<0.001$ s | $<0.01\%$ | Vector rounding of $1,800$ binary variables |
| **Cycle & Feasibility Checking** | $<0.001$ s | $<0.01\%$ | Hash set lookup of target binary vectors |
| **Total per Iteration** | **$\sim 45.0\text{--}60.0$ s** | **$100\%$** | **$30$ iterations $\approx 25$ minutes total** |

---

## 4. Integrality & Simple Rounding Diagnostic

### 4.1 Simple Rounding of Root LP Solution
- **Root Fractional Binary Variables**: $163$ / $1,800$
- **Max Integrality Deviation**: $0.5000$
- **$L_1$ Distance to Nearest Integer**: $68.5000$
- **Simple Rounded Constraint Violation**: **`1.0000`**
- **Bound Violations**: $0$
- **Conclusion**: Naïve rounding of $x_{\text{root}}$ violates original equality constraints ($A_i x = 1$) by $1.0$, rendering simple rounding integer-infeasible.

### 4.2 Feasibility Pump Auxiliary Model Validity
- **Formulation Check**: The auxiliary LP correctly models $\min \sum_{j \in \text{Int}} d_j$ subject to $d_j \ge x_j - t_j$ and $d_j \ge -(x_j - t_j)$, preserving original bounds and linear constraints.
- **Coordinate Mapping**: Solution vectors $x$ correctly map back to original variable indices $[0 \dots 2024]$.

---

## 5. Final Diagnostic Classification

The Feasibility Pump behavior on `10teams.mps` is classified as:

$$\mathbf{E.\; Excessive\; model\; construction\; overhead}$$

*(combined with **A. Correct but computationally expensive**)*

### Primary Root Cause
Constructing an explicit $L_1$ distance auxiliary LP model by adding $1,800$ continuous distance variables and $3,600$ linear constraints expands the row dimension of `10teams` from $230$ constraints to **$3,830$ constraints** ($16.65\times$ growth). Because each auxiliary LP solve refactorizes and solves a $3,830$-row matrix without warm-starting or PFI basis reuse, each pump iteration takes $\sim 50$ seconds, consuming the entire solver time limit inside the root heuristic phase.
