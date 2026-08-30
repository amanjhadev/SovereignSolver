# SOVEREIGN SOLVER — TRUE SPARSE LU REDESIGN
## PHASE 2 ARCHITECTURAL DESIGN REVIEW

## 1. Current Implementation Audit & Bottleneck Analysis

An audit of the current `SparseLU` implementation ([lu_factor.cpp](file:///c:/Solver/src/linalg/lu_factor.cpp)) identifies $O(N^2)$ memory allocations and $O(N^3)$ computational bottlenecks.

### A. Memory Bottlenecks ($O(N^2)$)
1. **Dense Working Matrix Allocation**:
   `std::vector<std::vector<Float64>> work(n_, std::vector<Float64>(n_, 0.0));`
   Allocates an $N \times N$ dense matrix of double-precision floats. For a basis of dimension $N = 2,120$ (`10teams`), this allocates $2,120$ heap vectors with $4.49 \times 10^6$ entries ($\approx 36 \text{ MB}$ per factorization).
2. **Dense Workspace Vectors**:
   Allocates 6 dense vector arrays for row/col usage, column maxes, and row/column non-zero counts (`col_max`, `row_nnz`, `col_nnz`).

### B. Computational Bottlenecks ($O(N^3)$)
1. **Dense Pivot Search ($O(N^3)$ Total)**:
   For each of the $N$ elimination steps:
   - **Step 1 (Column Max)**: Scans all active entries across all $N$ columns $\to O(N^2)$ per step.
   - **Step 2 (Markowitz Counts)**: Scans all $N \times N$ matrix elements to count active nonzeros $\to O(N^2)$ per step.
   - **Step 3 (Pivot Selection)**: Scans all $N \times N$ candidate pairs to evaluate threshold condition $|a_{ij}| \ge u \max_k |a_{ik}|$ and Markowitz score $(r_i - 1)(c_j - 1) \to O(N^2)$ per step.
2. **Dense Elimination ($O(N^3)$ Total)**:
   - `work[i][j] -= mult * work[best_row][j];` updates all $N$ columns in row $i$, ignoring sparsity patterns.
3. **COO Conversion Pass ($O(N^2)$)**:
   - Reconstructs $L$ and $U$ via `COOMatrix` before converting to `CSCMatrix`.

---

## 2. Data Structure Options Evaluation

| Data Structure / Representation | Memory Footprint | Insertion / Fill Cost | Pivot Search Cost | Cache Locality | Complexity | Recommendation |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **A. Sparse Row/Col Linked Lists** | $O(N + \text{NNZ})$ | High (Pointer updates) | $O(\text{degree})$ | Poor (Pointer chasing) | Medium | Rejected |
| **B. Dictionary of Keys (DOK) / Maps** | $O(\text{NNZ} \log \text{NNZ})$ | $O(\log \text{NNZ})$ | High | Poor (Node overhead) | Low | Rejected |
| **C. Contiguous Node-Pool + SPA + Degree Buckets** | **$O(N + \text{NNZ})$** | **$O(1)$ Amortized** | **$O(1)$ Best / $O(\text{candidates})$** | **High (Contiguous array)** | **Medium** | **RECOMMENDED** |

### Recommended Architecture: Option C (Contiguous Node-Pool + SPA + Degree Buckets)
- **Node-Pool Array**: Single contiguous `std::vector<SparseLUNode>` array storing matrix entries $(i, j, a_{ij})$ with 4 integer links (`next_in_row`, `prev_in_row`, `next_in_col`, `prev_in_col`).
- **Row & Column Headers**: Array of headers holding row/col non-zero counts (`row_len`, `col_len`), max absolute values (`col_max`), and head pointers (`row_head`, `col_head`).
- **Degree Buckets**: `head_degree_row[deg]` and `head_degree_col[deg]` doubly-linked lists linking active rows/cols of equal degree. Enables $O(1)$ retrieval of minimum-degree pivot candidates.
- **Sparse Accumulator (SPA)**: A dense vector `spa_val[N]`, boolean mask `spa_tok[N]`, and index array `spa_idx[N]` for $O(\text{nnz})$ row elimination without zero scans.

---

## 3. Sparse Pivot Selection Strategy

### Markowitz Criterion & Threshold Pivoting
1. **Candidate Score**: $M(i, j) = (\text{row\_len}[i] - 1)(\text{col\_len}[j] - 1)$.
2. **Threshold Condition**: Entry $a_{ij}$ must satisfy:
   $$|a_{ij}| \ge u \cdot \text{col\_max}[j] \quad \text{with threshold } u = 0.1$$
3. **Bucket-Search Order**:
   - Search active rows in order of increasing degree: $d = 1, 2, 3, \dots, N$.
   - For a row $i$ of degree $d$, inspect nonzeros $a_{ij}$.
   - The first entry satisfying the threshold condition with $M(i, j) \le (d-1)^2$ is selected immediately (*early exit*).
   - Scans at most $k = 4$ candidate rows before selecting best valid pivot.
4. **Stale Pivot Detection**: Degree buckets are updated in $O(1)$ time whenever entries are added or removed during elimination. Singularity is flagged if no candidate entry exceeds `config.pivot_tolerance`.

---

## 4. Fill-In & Elimination Mechanics

```
   Row Elimination with Sparse Accumulator (SPA):
   For active pivot row p and target row i:
     1. Load row i entries into SPA: spa_val[j] = a_ij, spa_tok[j] = true
     2. Multiplier m_ip = spa_val[pivot_col] / pivot_val
     3. Store m_ip into L structure
     4. For each entry a_pj in pivot row p (j != pivot_col):
          val = spa_val[j] - m_ip * a_pj
          If spa_tok[j] is true:
              Update spa_val[j] = val
          Else (NEW FILL ENTRY):
              Add j to spa_idx, spa_val[j] = val, spa_tok[j] = true
     5. Scatter SPA back to Node-Pool row i linked list.
     6. Clear SPA in O(row_len) time using spa_idx.
```

### Fill-In Metrics Tracked
- $\text{original\_nnz} = \text{NNZ}(A)$
- $\text{L\_nnz} = \text{NNZ}(L)$, $\text{U\_nnz} = \text{NNZ}(U)$
- $\text{fill\_nnz} = (\text{L\_nnz} + \text{U\_nnz}) - \text{original\_nnz}$
- $\text{fill\_ratio} = \frac{\text{L\_nnz} + \text{U\_nnz}}{\text{original\_nnz}}$

---

## 5. Ordering Strategy

- **Minimal Initial Implementation**: Markowitz-based dynamic ordering with degree bucket priority queues.
- **Future Extension**: AMD (Approximate Minimum Degree) pre-ordering for symmetric/unsymmetric structure reduction prior to numerical factorization.

---

## 6. Numerical Safety & Refactorization Criteria

1. **Threshold Pivoting**: $u = 0.1$ default (configurable to $0.25$ or $0.5$ if numerical growth occurs).
2. **Growth Factor Monitoring**:
   $$g = \frac{\max_{i,j} |u_{ij}|}{\max_{i,j} |a_{ij}|}$$
   If $g > 10^{12}$, return `Status::NumericalInstability`.
3. **Iterative Refinement**:
   In `solve(b, x)`: Solve $A x_0 = b \implies r_0 = b - A x_0 \implies A \delta x = r_0 \implies x_1 = x_0 + \delta x$. Repeat up to 3 iterations until $\|r\|_\infty \le 10^{-12} \|b\|_\infty$.
4. **Refactorization Triggers**:
   - Basis update count reaches threshold ($k \ge 50$).
   - Growth factor $g > 10^{12}$.
   - Singularity or ill-conditioning detected during FTRAN/BTRAN.

---

## 7. API Compatibility Mapping

```cpp
namespace sovereign::linalg {

class SparseLU {
private:
    Index n_{0};
    bool is_factored_{false};
    Status status_{Status::Ok};

    // Permutation vectors
    std::vector<Index> p_row_;     // Row permutation P
    std::vector<Index> p_row_inv_; // P^-1
    std::vector<Index> p_col_;     // Column permutation Q
    std::vector<Index> p_col_inv_; // Q^-1

    // Factorized L and U matrices in CSC format
    CSCMatrix L_; // Unit lower triangular
    CSCMatrix U_; // Upper triangular

    // Scalable Diagnostics
    Float64 min_pivot_magnitude_{INFINITY_VAL};
    Float64 max_pivot_magnitude_{0.0};
    Float64 pivot_growth_factor_{1.0};
    std::size_t fill_in_count_{0};

public:
    SparseLU() = default;

    // Preserved Public API Contract
    Status factorize(const CSCMatrix& A, const SolverConfig& config = SolverConfig::default_config());
    Status solve(std::span<const Float64> b, std::span<Float64> x) const;
    Status solve_transpose(std::span<const Float64> b, std::span<Float64> x) const;

    bool is_factored() const noexcept { return is_factored_; }
    Status status() const noexcept { return status_; }
    Index dimension() const noexcept { return n_; }

    std::span<const Index> row_perm() const noexcept { return p_row_; }
    std::span<const Index> col_perm() const noexcept { return p_col_; }

    const CSCMatrix& L() const noexcept { return L_; }
    const CSCMatrix& U() const noexcept { return U_; }

    Float64 min_pivot() const noexcept { return min_pivot_magnitude_; }
    Float64 max_pivot() const noexcept { return max_pivot_magnitude_; }
    std::size_t fill_in() const noexcept { return fill_in_count_; }
};

} // namespace sovereign::linalg
```

---

## 8. Product-Form Eta (PFI) Integration

$$\begin{aligned}
B_{\text{current}} = B_{\text{base}} \cdot E_1 \cdot E_2 \cdots E_k
\end{aligned}$$

1. `SparseLU` represents the base basis $B_{\text{base}} = P^T L U Q^T$.
2. FTRAN solves $B_{\text{current}} x = b$:
   - Solve base system $B_{\text{base}} y = b$ via `SparseLU::solve()`.
   - Apply forward Eta updates: $y \leftarrow E_1^{-1} y, \dots, y \leftarrow E_k^{-1} y$.
3. BTRAN solves $B_{\text{current}}^T y = c$:
   - Apply backward Eta updates: $c \leftarrow E_k^{-T} c, \dots, c \leftarrow E_1^{-T} c$.
   - Solve base transpose system $B_{\text{base}}^T y = c$ via `SparseLU::solve_transpose()`.
4. When $k \ge 50$, `BasisFactorization` refactorizes $B_{\text{current}}$ using `SparseLU::factorize()`, clears the Eta stack, and sets $B_{\text{base}} = B_{\text{current}}$.

---

## 9. Performance Target Measurements

- **Memory Allocation**: Reduce workspace allocation from $O(N^2)$ ($36 \text{ MB}$) to **$O(N + \text{NNZ})$ ($\approx 0.8 \text{ MB}$)**.
- **Pivot Search Speed**: Reduce pivot candidate evaluation from $O(N^2)$ ($4.5 \times 10^6$ scans) to **$O(1)$ amortized bucket lookup**.
- **Child LP Solve Time**: Target child LP solve time reduction from $\sim 6.24 \text{ s}$ to **$< 0.05 \text{ s}$ per node**.

---

## 10. Validation & Benchmark Plan

1. **Unit & Matrix Stress Tests**:
   - Random sparse matrices ($N = 100, 500, 1000, 2500$, sparsity $0.1\% \dots 2\%$).
   - Lower/Upper triangular matrices.
   - Ill-conditioned & singular matrices.
   - Permutation-heavy matrices.
   - Compare `solve()` and `solve_transpose()` residuals $\|A x - b\|_\infty < 10^{-10}$.

2. **Simplex Integration**:
   - Run `sovereign_tests.exe` (151 unit tests).
   - Run `10teams` Root LP solve and measure factorization & FTRAN/BTRAN times.
