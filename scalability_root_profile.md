# Sovereign Solver — Scalability Root LP Profile & Bottleneck Audit

---

## 1. Executive Summary

This forensic profile traces the exact execution path, memory structures, and call counts during the root LP relaxation of the `10teams` benchmark ($2,025$ variables, $230$ constraints, $12,150$ nonzeros, $1,800$ binary variables).

### Key Audit Findings

1. **LP Solver Selected**: `sovereign::lp::DualSimplexSolver`
   - In `ParallelMILPSolver::solve()` line 46, `lp::DualSimplexSolver root_lp_solver` is instantiated.
   - `BarrierSolver` is **NOT** used for root MILP relaxations.

2. **Factorization Called**: `sovereign::linalg::SparseLU`
   - Invoked via `SimplexState::factorize_basis()` $\to$ `BasisMatrix::factorize()`.
   - `SparseCholesky` is **NOT** called during MILP root LP relaxations.

3. **Dominant Bottlenecks Identified**:
   - **Bottleneck A (Primary)**: `SimplexState` lacks a rank-1 basis update mechanism (e.g. Forrest-Tomlin or Eta updates). Consequently, `state.factorize_basis()` calls `SparseLU::factorize()` **from scratch on every single simplex iteration**.
   - **Bottleneck B (Secondary)**: `SparseLU::factorize()` contains a dense $O(N^2)$ working matrix (`std::vector<std::vector<Float64>> work(n_, std::vector<Float64>(n_, 0.0))`) and an $O(N^3)$ pivot candidate search.
   - **Combined Impact**: On $10\text{teams}$, the basis matrix dimension is $N = 230 \times 230$ (905 nonzeros). A single `SparseLU::factorize()` takes **$64.64\text{ ms}$**. Because full refactorization is executed on every pivot step, $\approx 12,000$ iterations take $12,000 \times 68.41\text{ ms/iter} \approx 820.9\text{ seconds} \approx 13.68\text{ minutes}$.

---

## 2. Empirical Execution Profile Breakdown

| Execution Stage / Component | Called? | Matrix Dimensions | Call Count | Measured / Extrapolated Time | % Total Time | Bottleneck Mechanism |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Model Parsing & Setup** | YES | $2,025 \times 230$ | 1 | $26.0\text{ ms}$ | $0.003\%$ | String parsing |
| **LP Validation (`LPValidator`)** | YES | $2,025 \times 230$ | 1 | $0.08\text{ ms}$ | $< 0.001\%$ | Bounds check |
| **Standard Form Conversion** | YES | $2,120 \times 230$ | 1 | $10.0\text{ ms}$ | $0.001\%$ | $O(M \cdot N)$ column row-index search |
| **Initial Basis Construction** | YES | $230 \times 230$ | 1 | $60.6\text{ ms}$ | $0.007\%$ | Dense pivot check |
| **BarrierSolver** | **NO** | N/A | 0 | $0.0\text{ ms}$ | $0.0\%$ | N/A |
| **SparseCholesky** | **NO** | N/A | 0 | $0.0\text{ ms}$ | $0.0\%$ | N/A |
| **DualSimplex / PrimalSimplex Pipeline** | YES | $2,120 \times 230$ | 1 solve (~12k iters) | $820.9\text{ s}$ | $100.0\%$ | Full Simplex loop |
| ├── **`SparseLU::factorize()` (Refactorization per Iteration)** | **YES** | **$230 \times 230$** | **~12,000** | **~775.8 s** | **94.5%** | **Dense $O(N^2)$ table + $O(N^3)$ pivot scan per step** |
| └── **Simplex Pricing, Ratio Test & FTRAN/BTRAN Solves** | YES | $2,120 \times 230$ | ~12,000 | ~45.1 s | 5.5% | Pricing & triangular solves |

---

## 3. Hidden Dense Structures & Memory Bottlenecks

### 1. `SparseLU::factorize()` (Dense Working Storage)
- **Location**: `src/linalg/lu_factor.cpp` line 31.
- **Code**: `std::vector<std::vector<Float64>> work(n_, std::vector<Float64>(n_, 0.0));`
- **Impact**: Allocates an $N \times N$ dense grid on every factorization call. Scans all uneliminated $(i, j)$ entries for Markowitz minimums.

### 2. `SimplexState` (Absence of Basis Update)
- **Location**: `src/lp/dual_simplex.cpp` (lines 248) and `src/lp/primal_simplex.cpp` (line 18).
- **Code**: `if (!is_ok(state.factorize_basis(config_))) ...` inside the main iteration loop.
- **Impact**: Rebuilds the CSC basis matrix $B$ from $A$ and invokes full `SparseLU::factorize()` on **every single iteration**, rather than updating the existing LU factors via rank-1 updates (e.g. Forrest-Tomlin or Eta matrices).

### 3. `LPModel::convert_to_standard_form()` ($O(M \cdot N)$ Search Loop)
- **Location**: `src/lp/lp_model.cpp` lines 160–183.
- **Code**: Double loop over constraints $i \in [0, M)$ and variables $j \in [0, N)$, calling `csc.get_col_row_indices(j)`.
- **Impact**: Takes $10.0\text{ ms}$ on $10\text{teams}$, but scales quadratically to several seconds on $N \ge 25,000$.

### 4. `MPSParser` (Missing Marker Support)
- **Location**: `src/lp/mps_parser.cpp` Section::Columns.
- **Impact**: Fails to parse standard MPS `MARKER 'INTORG' / 'INTEND'` lines, attempting to look up `'MARKER'` as a row name.

---

## 4. Architectural Scaling Roadmap

To achieve scalability across larger LP/QP/MILP models, the engineering plan is structured into clear phases:

1. **Phase 7 (MPS Parser Hardening)**: Add native generic support for `MARKER INTORG` / `MARKER INTEND` in `MPSParser`.
2. **Phase 1 & 2 (True Sparse LU & Fill-In Control)**: Replace dense $O(N^2)$ `work` array in `SparseLU` with dynamic dual-linked dynamic sparse structures and fill-reducing ordering.
3. **Phase 4 (Simplex Basis Update & Solve Efficiency)**: Implement basis refactorization frequency management (e.g., refactorize only every $K = 50\text{--}100$ iterations, using fast rank-1 updates or updating factors between refactorizations).
4. **Phase 6 (Time Limit & Cancellation Correctness)**: Check cancellation tokens / time limits inside long LP solves and simplex iteration loops.
5. **Phase 8–11 (Validation, Benchmarking & Comparison)**: Execute scalability ladder ($1\text{k}\to 100\text{k}$) and external comparison.
