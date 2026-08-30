# Sovereign Solver — Basis Update Infrastructure Design

---

## 1. Overview & Architectural Objective

The forensic audit of the `10teams` benchmark established that `SimplexState` called `SparseLU::factorize()` from scratch on **every single simplex pivot iteration**. For $12,000$ iterations, this resulted in $12,000$ full factorizations of the $230 \times 230$ basis matrix, taking $\approx 13$ minutes before Branch-and-Bound branching could begin.

This design introduces a Product-Form Eta (PFI) Basis Update component (`BasisFactorization`). The basis matrix representation is transformed from:

$$\text{Full LU Factorization Every Iteration}$$

to:

$$\text{Base LU Factorization } B^{(0)} + \text{Sequence of Eta Updates } (E_1, E_2, \dots, E_k)$$

Periodic full refactorizations occur only every $K_{\max} = 50$ iterations or when numerical instability is detected.

---

## 2. Mathematical Specification

### 2.1 Basis Pivot Transformation
When nonbasic variable $x_q$ enters the basis and replaces basic variable $x_{B(p)}$ at slot $p$, column $p$ of the basis matrix $B$ is replaced by column $a_q$ of the standard form matrix $A$:

$$B^{(k)} = B^{(k-1)} + (a_q - a_{B(p)}) e_p^T = B^{(k-1)} E_k$$

where $E_k$ is an **Eta matrix** defined as:

$$E_k = I + (\alpha - e_p) e_p^T = \begin{pmatrix} 
1 & & \alpha_0 & & \\
& 1 & \vdots & & \\
& & \alpha_p & & \\
& & \vdots & & \\
& & \alpha_{m-1} & & 1 
\end{pmatrix}$$

Here, $\alpha = (B^{(k-1)})^{-1} a_q$ is the FTRAN direction vector computed during the pivot step, and $\alpha_p$ is the pivot element.

### 2.2 Product-Form Representation
After $k$ pivot steps since the last full refactorization:

$$B^{(k)} = B^{(0)} E_1 E_2 \dots E_k$$

### 2.3 Forward Solve (FTRAN: $B^{(k)} x = b$)
To solve for $x$:

$$x = E_k^{-1} E_{k-1}^{-1} \dots E_1^{-1} \left(B^{(0)}\right)^{-1} b$$

**Algorithm**:
1. Solve base system $B^{(0)} y = b$ using SparseLU factors ($L, U$).
2. For $t = 1 \dots k$:
   - $w = y[p_t] / \alpha_{p_t}$
   - $y[p_t] = w$
   - For all $i \neq p_t$: $y[i] \leftarrow y[i] - \alpha_{i, t} \cdot w$
3. Output $x = y$.

### 2.4 Backward Solve (BTRAN: $(B^{(k)})^T y = c_B$)
To solve for $y$:

$$y = \left(B^{(0)}\right)^{-T} E_1^{-T} E_2^{-T} \dots E_k^{-T} c_B$$

**Algorithm**:
1. Initialize $w = c_B$.
2. For $t = k \text{ down to } 1$:
   - $S = \sum_{i \neq p_t} \alpha_{i, t} \cdot w[i]$
   - $w[p_t] \leftarrow \frac{w[p_t] - S}{\alpha_{p_t}}$
3. Solve base transpose system $(B^{(0)})^T y = w$ using SparseLU factors ($L, U$).
4. Output $y$.

---

## 3. Class Design & Component Interface

```cpp
namespace sovereign::lp {

struct EtaUpdate {
    Index slot_p{0};
    Float64 alpha_p{1.0};
    std::vector<Index> nz_rows;
    std::vector<Float64> nz_vals;
};

class BasisFactorization {
private:
    Index m_{0};
    linalg::CSCMatrix B_base_csc_;
    linalg::SparseLU lu_base_;
    std::vector<EtaUpdate> updates_;
    Int32 max_updates_{50};
    Float64 pivot_tol_{1e-8};
    bool is_factored_{false};

public:
    BasisFactorization() = default;

    // Full factorization from CSC matrix and Basis
    Status factorize(const linalg::CSCMatrix& A, const Basis& basis, const SolverConfig& config);

    // Apply pivot update (adds Eta update to stack)
    Status update_pivot(Index slot_p, std::span<const Float64> ftran_alpha, Float64 pivot_tol);

    // FTRAN solve: B*x = b
    Status solve_primal(std::span<const Float64> b, std::span<Float64> x) const;

    // BTRAN solve: B^T*y = c_B
    Status solve_dual(std::span<const Float64> c_B, std::span<Float64> y) const;

    // Trigger full refactorization check
    bool should_refactorize() const noexcept {
        return static_cast<Int32>(updates_.size()) >= max_updates_;
    }

    void clear_updates() { updates_.clear(); }
    Int32 update_count() const noexcept { return static_cast<Int32>(updates_.size()); }
    bool is_factored() const noexcept { return is_factored_; }
};

} // namespace sovereign::lp
```

---

## 4. Refactorization & Stability Triggers

A full refactorization is triggered when:
1. `update_count() >= max_updates_` (default $50$).
2. Pivot magnitude $|\alpha_p| < \text{pivot\_tolerance}$ ($10^{-8}$).
3. FTRAN or BTRAN residual exceeds tolerance ($> 10^{-6}$).
4. Explicit request after BFRT bound flips or Devex reset.
