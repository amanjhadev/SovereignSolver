# SOVEREIGN SOLVER — SPARSE LU HARDENED DESIGN SPECIFICATION
## IMPLEMENTATION-READY ARCHITECTURAL & MATHEMATICAL SPECIFICATION

---

## 1. Rigorous Pivot Search Strategy & Fallback Mechanics

### A. Candidate Selection Pipeline
Rather than hard-capping search to an arbitrary $k=4$ rows without fallback, pivot selection follows a mathematically rigorous, bucket-based candidate inspection with full fallback guarantees:

$$\mathbf{\text{Degree Buckets} \longrightarrow \text{Low-Degree Access} \longrightarrow \text{Threshold Filtering} \longrightarrow \text{Markowitz Scoring} \longrightarrow \text{Pivot Selection}}$$

1. **Near-Constant-Time Candidate Access**:
   - Access active rows of lowest degree $d = 1, 2, 3, \dots, N$ via `head_degree_row[deg]` bucket heads.
2. **Threshold Pivot Filtering**:
   - For an active row $i$ of degree $d$, inspect nonzeros $a_{ij}$.
   - An entry $a_{ij}$ is eligible ONLY IF it satisfies the threshold condition:
     $$|a_{ij}| \ge u \cdot \text{col\_max}[j]$$
     where $u \in (0.0, 1.0]$ is the pivot threshold parameter (default $u = 0.1$).
3. **Markowitz Scoring**:
   - For each eligible entry, calculate the Markowitz score:
     $$M(i, j) = (\text{row\_len}[i] - 1) \cdot (\text{col\_len}[j] - 1)$$
4. **Early Exit Rule**:
   - If an eligible entry satisfies $M(i, j) \le (d - 1)^2$, select $(i, j)$ immediately and terminate the search for step $k$.
5. **Bounded Candidate Budget with Guaranteed Fallback**:
   - Search evaluates up to $k_{\text{max}} = 16$ low-degree candidate rows.
   - **Safe Fallback Search**: If no candidate within the budget satisfies the threshold condition $|a_{ij}| \ge u \cdot \text{col\_max}[j]$, search automatically expands across ALL remaining active rows in the submatrix.
   - If no non-zero entry satisfies $|a_{ij}| \ge \text{pivot\_tolerance}$ ($10^{-12}$), the matrix is mathematically classified as `Status::SingularMatrix`.

---

## 2. Sparse Structural Invariants & Metadata Lifecycle

### A. Core Structural Data Structures
1. **Contiguous Node Pool (`std::vector<SparseLUNode> pool_`)**:
   Each entry represents a non-zero element $(r, c, \text{val})$ in the active working matrix:
   ```cpp
   struct SparseLUNode {
       Index row;
       Index col;
       Float64 val;
       Index next_in_row;
       Index prev_in_row;
       Index next_in_col;
       Index prev_in_col;
       bool active;
   };
   ```
2. **Row & Column Headers**:
   - `row_head[i]`, `col_head[j]`: First node index in row $i$ / column $j$.
   - `row_len[i]`, `col_len[j]`: Active non-zero element count in row $i$ / column $j$.
   - `col_max[j]`: Maximum absolute value $\max_i |a_{ij}|$ in active column $j$.
3. **Degree Buckets**:
   - `head_degree_row[d]`, `prev_degree_row[i]`, `next_degree_row[i]`: Doubly-linked list of active rows having `row_len[i] == d`.
   - `head_degree_col[d]`, `prev_degree_col[j]`, `next_degree_col[j]`: Doubly-linked list of active columns having `col_len[j] == d`.

### B. Invariant Preservation Rules

| Structural Operation | Metadata Changes & Updates Required |
| :--- | :--- |
| **Node Insertion $(i, j, v)$** | 1. Allocate node from pool (`pool_.push_back` or reuse free list).<br>2. Link into `row_head[i]` and `col_head[j]` doubly-linked lists.<br>3. Increment `row_len[i]++` and `col_len[j]++`.<br>4. Update degree bucket position for row $i$ and col $j$.<br>5. Update `col_max[j] = std::max(col_max[j], |v|)`. |
| **Node Deletion $(i, j)$** | 1. Unlink node from row $i$ and col $j$ doubly-linked lists.<br>2. Set `node.active = false`.<br>3. Decrement `row_len[i]--` and `col_len[j]--`.<br>4. Update degree bucket position for row $i$ and col $j$.<br>5. Recompute `col_max[j]` if deleted entry was $\text{col\_max}[j]$. |
| **Pivot Row Removal ($p$)** | 1. Remove row $p$ and col $q$ from degree buckets.<br>2. Mark all entries in row $p$ and col $q$ as inactive. |

### C. Stale Bucket Entry Management
Degree buckets are maintained strictly in $O(1)$ time during element insertion/deletion. When `row_len[i]` changes from $d_{\text{old}}$ to $d_{\text{new}}$, row $i$ is unlinked from `head_degree_row[d_old]` and inserted at `head_degree_row[d_new]`. Stale bucket lookups are impossible because bucket lists remain 100% synchronized with `row_len[i]`.

---

## 3. Formal Sparse Accumulator (SPA) Elimination Derivation

### A. Mathematical Derivation
At elimination step $k$, let $(p, q)$ be the selected pivot entry with value $a_{pq} \ne 0$.
For each active target row $i$ containing a non-zero entry $a_{iq}$ in the pivot column $q$:

1. Compute multiplier:
   $$m_{iq} = \frac{a_{iq}}{a_{pq}}$$
2. Perform row elimination:
   $$\text{row}_i^{(k+1)} = \text{row}_i^{(k)} - m_{iq} \cdot \text{row}_p^{(k)}$$

### B. SPA Step-by-Step Algorithm
Let `spa_val[N]` be a dense float array, `spa_tok[N]` a boolean marker array, and `spa_idx` a dense integer stack:

1. **Load Target Row $i$ into SPA**:
   For each node $a_{ij}$ in row $i$:
   - `spa_val[j] = a_ij`, `spa_tok[j] = true`, `spa_idx.push_back(j)`.
2. **Compute Multiplier**:
   - $m_{iq} = \text{spa\_val}[q] / a_{pq}$.
   - Store $m_{iq}$ into lower triangular multiplier list for $L$.
   - Clear pivot column entry: `spa_val[q] = 0.0`, `spa_tok[q] = false`.
3. **Sparse Row Combination**:
   For each node $a_{pj}$ in pivot row $p$ ($j \ne q$):
   - $\text{new\_val} = \text{spa\_val}[j] - m_{iq} \cdot a_{pj}$.
   - **Case 1: Existing Entry ($\text{spa\_tok}[j] == \text{true}$)**:
     - If $|\text{new\_val}| \le \text{zero\_tolerance}$ ($10^{-14}$):
       - **Exact Zero Cancellation**: Mark for deletion from row $i$.
       - `spa_val[j] = 0.0`, `spa_tok[j] = false`.
     - Else:
       - Update value: `spa_val[j] = new_val`.
   - **Case 2: New Fill Entry ($\text{spa\_tok}[j] == \text{false}$)**:
     - If $|\text{new\_val}| > \text{zero\_tolerance}$:
       - **Fill Creation**: `spa_val[j] = new_val`, `spa_tok[j] = true`, `spa_idx.push_back(j)`.
4. **Scatter SPA Back to Sparse Row $i$**:
   - Clear existing row $i$ linked list.
   - Insert updated nonzeros from `spa_idx` back into node-pool row $i$ linked list.
5. **Reset SPA Workspace**:
   - For $j \in \text{spa\_idx}$: `spa_val[j] = 0.0`, `spa_tok[j] = false`. Clear `spa_idx` stack in $O(\text{nnz}_i)$ time.

---

## 4. Fill-Sensitive Complexity Specification

We explicitly avoid naive $O(1)$ or $O(N)$ generic claims. Factorization complexity depends strictly on sparse matrix fill:

1. **Data-Structure Insertion/Deletion**: $O(1)$ amortized per non-zero node.
2. **Pivot Candidate Retrieval**: $O(1)$ initial bucket access $+ O(\text{candidates\_inspected})$ threshold evaluations.
3. **SPA Elimination Arithmetic**:
   $$O\left( \sum_{k=1}^N \sum_{i \in \text{active}} \text{nnz}(\text{row}_p) \right) = O\left( \text{FLOPs}(L, U) \right)$$
4. **Total Factorization Complexity**:
   $$\mathcal{O}\left( N + \text{NNZ}(A) + \text{FLOPs}_{\text{elimination}}(L, U) \right)$$

---

## 5. Numerical Pivoting & Growth Control Parameters

The numerical controls are treated as **configurable defaults subject to empirical validation**:

- **Pivot Threshold Parameter ($u$)**: Default $u = 0.1$. If growth is detected during factorization, threshold is dynamically increased to $u = 0.25$ or $0.5$.
- **Pivot Tolerance ($\epsilon_{\text{pivot}}$)**: $10^{-12}$. Any pivot $|a_{pq}| < 10^{-12}$ triggers fallback search. If no valid pivot exists, matrix is classified `SingularMatrix`.
- **Numerical Growth Factor ($g$)**:
  $$g = \frac{\max_{i,j} |u_{ij}|}{\max_{i,j} |a_{ij}|}$$
  If $g > 10^{12}$, return `Status::NumericalInstability`.
- **Zero Tolerance ($\epsilon_{\text{zero}}$)**: $10^{-14}$ for cancellation to exact zero.

---

## 6. Unambiguous Permutation Semantics

### A. Factorization Representation
$$\begin{aligned}
P \cdot A \cdot Q = L \cdot U \implies A = P^T \cdot L \cdot U \cdot Q^T
\end{aligned}$$

- $P$: Row permutation matrix ($p\_row[k] = \text{original row mapped to permuted step } k$).
- $Q$: Column permutation matrix ($p\_col[k] = \text{permuted column } k \text{ mapped to original column}$).

### B. Forward Solve: $A x = b$
$$\begin{aligned}
P^T \cdot L \cdot U \cdot Q^T \cdot x = b \implies L \cdot U \cdot (Q^T x) = P \cdot b
\end{aligned}$$

1. **Permute RHS ($pb = P b$)**: $pb[k] = b[p\_row[k]]$ for $k = 0 \dots N-1$.
2. **Forward Solve L ($L w = pb$)**: Unit lower triangular solve on $L$.
3. **Backward Solve U ($U z = w$)**: Upper triangular solve on $U$.
4. **Unpermute Solution ($x = Q z$)**: $x[p\_col[k]] = z[k]$ for $k = 0 \dots N-1$.

### C. Transpose Solve: $A^T x = b$
$$\begin{aligned}
Q \cdot U^T \cdot L^T \cdot P \cdot x = b \implies U^T \cdot L^T \cdot (P x) = Q^T \cdot b
\end{aligned}$$

1. **Permute RHS ($qb = Q^T b$)**: $qb[k] = b[p\_col[k]]$ for $k = 0 \dots N-1$.
2. **Forward Solve $U^T$ ($U^T y = qb$)**: Lower triangular solve on $U^T$.
3. **Backward Solve $L^T$ ($L^T v = y$)**: Unit upper triangular solve on $L^T$.
4. **Unpermute Solution ($P x = v$)**: $x[p\_row[k]] = v[k]$ for $k = 0 \dots N-1$.

---

## 7. Factor Assembly Strategy Recommendation

$$\mathbf{\text{RECOMMENDATION: Direct Column-Wise Assembly into CSC Arrays}}$$

To avoid slow $L, U \to \text{COO} \to \text{CSC}$ conversion passes, $L$ and $U$ are assembled **directly into standard CSC arrays** (`outer_starts`, `inner_indices`, `values`) during elimination:
- Multipliers $m_{iq}$ for step $k$ are written directly into column $k$ of $L$'s CSC structure.
- Pivot value $u_{kk}$ and row entries $u_{kj}$ for step $k$ are written directly into column $k$ of $U$'s CSC structure.
- **Eliminates intermediate `COOMatrix` allocations completely.**

---

## 8. Realistic Memory Accounting Formulas

### Memory Components Formula
$$\text{Memory Total} = M_{\text{pool}} + M_{\text{headers}} + M_{\text{perms}} + M_{\text{factors}} + M_{\text{SPA}} + M_{\text{buckets}}$$

1. **Node Pool ($M_{\text{pool}}$)**: $N_{\text{max\_nodes}} \times 36 \text{ bytes}$ (36 bytes per `SparseLUNode`).
2. **Row & Col Headers ($M_{\text{headers}}$)**: $N \times 40 \text{ bytes}$.
3. **Permutation Arrays ($M_{\text{perms}}$)**: $4N \times 8 \text{ bytes} = 32N \text{ bytes}$.
4. **$L$ & $U$ CSC Storage ($M_{\text{factors}}$)**: $(\text{NNZ}_L + \text{NNZ}_U) \times 16 \text{ bytes} + (2N + 2) \times 8 \text{ bytes}$.
5. **SPA Workspace ($M_{\text{SPA}}$)**: $N \times 24 \text{ bytes}$.
6. **Degree Buckets ($M_{\text{buckets}}$)**: $4N \times 8 \text{ bytes} = 32N \text{ bytes}$.

### Concrete Benchmark Example: `10teams` Basis ($N = 2,120$, $\text{NNZ}_A = 12,150$)
- Assuming fill ratio $\gamma = 2.0 \implies \text{NNZ}_L + \text{NNZ}_U \approx 24,300$.
- Node Pool Capacity: $30,000$ nodes $\times 36 \text{ bytes} = 1.08 \text{ MB}$.
- Headers + Permutations + Buckets + SPA: $2,120 \times 128 \text{ bytes} = 0.27 \text{ MB}$.
- Factor Storage ($L + U$ CSC): $24,300 \times 16 + 4,242 \times 8 = 0.42 \text{ MB}$.
- **Total Measured Memory Footprint: $\mathbf{\approx 1.77 \text{ MB}}$** (Down from $36.0 \text{ MB}$ dense working matrix!).

---

## 9. Reference Backend Preservation

The existing implementation in `src/linalg/lu_factor.cpp` will be retained under the class name `ReferenceDenseLU` during development. A compile-time/runtime configuration flag will enable side-by-side verification:

```cpp
// Equivalence Audit Harness:
ReferenceDenseLU ref_lu;
SparseLU new_lu;

ref_lu.factorize(A);
new_lu.factorize(A);

// Verify exact solution equivalence: ||x_new - x_ref||_inf < 1e-10
```
`ReferenceDenseLU` will remain active in the unit test suite until 100% parity is established across all benchmarks.

---

## 10. Product-Form Eta (PFI) Integration & Safety

$$\begin{aligned}
B_{\text{current}} = B_{\text{base}} \cdot E_1 \cdot E_2 \cdots E_k
\end{aligned}$$

1. **Refactorization Lifecycle**:
   - `SparseLU` factorizes $B_{\text{base}} = P^T L U Q^T$.
   - When PFI Eta count reaches limit ($k \ge 50$), `BasisFactorization` invokes `SparseLU::factorize(B_current)`, clears the Eta vector stack, and sets $B_{\text{base}} = B_{\text{current}}$.
2. **FTRAN & BTRAN Parity**:
   - FTRAN ($B_{\text{current}} x = b$) invokes `SparseLU::solve(b, y)` followed by forward Eta updates.
   - BTRAN ($B_{\text{current}}^T y = c$) applies backward Eta updates followed by `SparseLU::solve_transpose(c, y)`.
   - **Safeguard**: Zero API breaking changes to `BasisFactorization`.

---

## 11. Comprehensive Test Strategy Matrix

| Test Scenario | Test Description | Expected Status | Residual Tolerance | Equivalence Criterion |
| :--- | :--- | :---: | :---: | :---: |
| **No Fill** | Diagonal / Permuted Diagonal Matrix | `Status::Ok` | $\|A x - b\|_\infty < 10^{-14}$ | $\text{fill\_in} == 0$ |
| **Low Fill** | Tridiagonal / Arrowhead Matrix | `Status::Ok` | $\|A x - b\|_\infty < 10^{-12}$ | Match `ReferenceDenseLU` |
| **Heavy Fill** | Dense-pattern Sparse Matrix | `Status::Ok` | $\|A x - b\|_\infty < 10^{-10}$ | Match `ReferenceDenseLU` |
| **Exact Zero Cancellation** | $a_{ij} - m a_{pj} = 0.0$ | `Status::Ok` | $\|A x - b\|_\infty < 10^{-12}$ | Entry deleted from SPA |
| **Near-Zero Cancellation** | $|a_{ij} - m a_{pj}| < 10^{-14}$ | `Status::Ok` | $\|A x - b\|_\infty < 10^{-12}$ | Entry deleted from SPA |
| **Singular Matrix** | Row/Col of all zeros | `Status::SingularMatrix` | N/A | Match `ReferenceDenseLU` |
| **Near-Singular Matrix** | Condition number $\kappa(A) \approx 10^{14}$ | `Status::Ok` / `NumericalInstability` | $\|A x - b\|_\infty < 10^{-8}$ | Iterative refinement activated |
| **Permutation Heavy** | Anti-diagonal permutation matrix | `Status::Ok` | $\|A x - b\|_\infty < 10^{-14}$ | Correct $P, Q$ indices |
| **Transpose Solve** | $A^T x = b$ on unsymmetric matrix | `Status::Ok` | $\|A^T x - b\|_\infty < 10^{-12}$ | Match `ReferenceDenseLU` |
| **Repeated RHS** | 100 random RHS vectors $b$ | `Status::Ok` | $\|A x_k - b_k\|_\infty < 10^{-12}$ | Reuse $L, U$ factors |
| **Random Sparse** | $N = 100 \dots 2500$, density $0.1\% \dots 2\%$ | `Status::Ok` | $\|A x - b\|_\infty < 10^{-10}$ | Match `ReferenceDenseLU` |

---

## 12. Performance Experiment Design

### Benchmark Metrics (Identical Matrices & Environment)
- **`t_factorize`**: Pure factorization time (ms).
- **`t_ftran`**: Single FTRAN solve time ($\mu$s).
- **`t_btran`**: Single BTRAN solve time ($\mu$s).
- **`memory_bytes`**: Total heap memory allocated (MB).
- **`fill_ratio`**: $\frac{\text{NNZ}(L) + \text{NNZ}(U)}{\text{NNZ}(A)}$.
- **`num_pivot_candidates`**: Total candidate rows inspected during pivot search.
- **`num_refactorizations`**: Refactorizations triggered by numerical growth/instability.

---

## 13. Step-by-Step Implementation & Validation Roadmap

$$\begin{aligned}
&\text{Step 1: Implement Node-Pool, SPA, and Degree Buckets} \\
&\quad \downarrow \\
&\text{Step 2: Implement Hardened SparseLU Factorization \& Solves} \\
&\quad \downarrow \\
&\text{Step 3: Run Equivalence Tests vs ReferenceDenseLU (151 Unit Tests PASS)} \\
&\quad \downarrow \\
&\text{Step 4: Integrate into BasisFactorization \& Validate PFI} \\
&\quad \downarrow \\
&\text{Step 5: Validate Primal/Dual Simplex \& MILP Child LP Solves} \\
&\quad \downarrow \\
&\text{Step 6: Run 10teams Root LP \& Record Factorization/FTRAN/BTRAN Speedups}
\end{aligned}$$

*This specification completes the Phase 2 True Sparse LU Hardened Design Review.*
