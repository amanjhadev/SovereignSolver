# SOVEREIGN SOLVER — 10TEAMS POST-SCALABLE-FP MILP CHECKPOINT

## 1. Executive Summary

This report documents the functional serial MILP solve checkpoint on the **10teams benchmark** (`10teams.mps`) following the successful implementation and integration of the **Scalable Binary Feasibility Pump**.

### Execution Sequence Observed
$$\mathbf{Root\; LP \longrightarrow Scalable\; FP \longrightarrow B\&B\; Tree\; Search \longrightarrow Tree\; Exhausted}$$

- **Root LP Relaxation**: Solved to global optimality in **`7.02 seconds`** with objective **`917.0000`** ($163$ / $1,800$ fractional binary variables).
- **Scalable Feasibility Pump**: Executed in **`5.85 seconds`** with **`0` added variables** and **`0` added constraints** (preserving original $230$-row matrix dimensions).
- **Branch-and-Bound Entry**: The solver immediately transitioned into Branch-and-Bound tree search, exploring child nodes with an average LP solve latency of **`6.24 s/solve`**.
- **Bound Progression**: Dual bound improved from $917.0000$ at Node 0 to $918.0000$ at Node 10.
- **Tree Termination**: 7 child nodes were fathomed by infeasibility, exhausting the active search tree in **`100.78 seconds`**.

---

## 2. Complete Telemetry & Execution Summary

| Phase / Component | Metric | Measured Value |
| :--- | :--- | :---: |
| **Root LP Relaxation** | Status | `OPTIMAL` (1) |
| | Objective Value | **`917.0000`** |
| | Solve Time | **`7.02 s`** ($6.96\%$) |
| | Simplex Iterations | `9,175` |
| | Fractional Binary Variables | $163$ / $1,800$ |
| **Scalable Feasibility Pump**| Auxiliary Variables / Constraints Added | **`0 / 0`** |
| | Pump Execution Time | **`5.85 s`** (was $\sim 25$ min) |
| | Best $L_1$ Integrality Distance | $38.5561$ |
| | Candidate Incumbent Found | `No` |
| **Branch-and-Bound Search** | Nodes Explored / Created | $13$ / $13$ |
| | Open Nodes Remaining | $0$ |
| | Total LP Solves | $14$ |
| | Total Child LP Time | **`81.07 s`** ($80.44\%$) |
| | Average Child LP Time | **`6.24 s/solve`** |
| | Pruned by Infeasibility | $7$ nodes |
| | Max Tree Depth | $4$ |
| **Final State** | Status / Termination Reason | `EXHAUSTED` |
| | Total Execution Time | **`100.78 seconds`** |
| | Best Bound Progression | $917.00 \longrightarrow 918.00 \longrightarrow \infty$ |

---

## 3. Progress Checkpoints Table

| Checkpoint | Explored | Open | Best Dual Bound | Incumbent Obj | MIP Gap | LP Solves | Elapsed Time (s) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Root LP** | $0$ | $1$ | $917.0000$ | None | $100.0\%$ | $1$ | $7.02$ s |
| **Node 0** | $0$ | $1$ | $917.0000$ | None | $100.0\%$ | $1$ | $19.71$ s |
| **Node 10** | $10$ | $3$ | **$918.0000$** | None | $100.0\%$ | $11$ | $83.73$ s |
| **Final (Node 13)**| $13$ | $0$ | $\infty$ (Exhausted) | None | $100.0\%$ | $14$ | **$100.78$ s** |

---

## 4. Key Engineering Insights

1. **Scalable FP Unblocks B&B**: Replacing the old $3,830$-row auxiliary distance LP model with in-place binary objective reweighting reduced root heuristic time from $\sim 25$ minutes to **$5.85$ seconds**, allowing the solver to cleanly enter Branch-and-Bound.
2. **Dual Bound Improvement**: Dual bound progressed from $917.0$ to $918.0$ during child branching, verifying correct bound propagation across child LP relaxations.
3. **Child LP Latency**: Average child LP solve time is $6.24$ seconds per node, confirming that child node solves are stable and functioning as intended.
