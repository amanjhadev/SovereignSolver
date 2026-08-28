#ifndef SOVEREIGN_LP_SIMPLEX_STATE_HPP
#define SOVEREIGN_LP_SIMPLEX_STATE_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/core/status.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/lp/basis.hpp"
#include "sovereign/lp/basis_matrix.hpp"
#include "sovereign/lp/simplex_types.hpp"
#include <span>
#include <vector>

namespace sovereign::lp {

class SimplexState {
private:
    const LPModel* std_model_{nullptr};
    Basis basis_;
    BasisMatrix basis_matrix_;

    Index m_{0};
    Index n_{0};

    std::vector<Float64> x_B_;           // Size m: primal basic variable values
    std::vector<Float64> x_all_;         // Size n: primal values for all standard variables
    std::vector<Float64> y_;             // Size m: dual variable vector y = B^-T c_B
    std::vector<Float64> reduced_costs_; // Size n: d_j = c_j - y^T A_j

    std::vector<Float64> b_eff_;         // Size m: b - sum_{j in N} A_j x_j^NB

    Int64 iterations_{0};
    bool is_factored_{false};

public:
    SimplexState() = default;
    SimplexState(const LPModel& std_model, Basis basis);

    // Inspection
    const LPModel& std_model() const noexcept { return *std_model_; }
    const Basis& basis() const noexcept { return basis_; }
    Basis& basis() noexcept { return basis_; }
    const BasisMatrix& basis_matrix() const noexcept { return basis_matrix_; }
    BasisMatrix& basis_matrix() noexcept { return basis_matrix_; }

    Index num_rows() const noexcept { return m_; }
    Index num_cols() const noexcept { return n_; }
    Int64 iterations() const noexcept { return iterations_; }
    void increment_iterations() noexcept { ++iterations_; }

    std::span<const Float64> x_B() const noexcept { return x_B_; }
    std::span<const Float64> x_all() const noexcept { return x_all_; }
    std::span<const Float64> y() const noexcept { return y_; }
    std::span<const Float64> reduced_costs() const noexcept { return reduced_costs_; }
    std::span<const Float64> b_eff() const noexcept { return b_eff_; }

    // Core Computations
    Status factorize_basis(const SolverConfig& config = SolverConfig{});
    Status compute_b_eff();
    Status solve_primal_solution();
    Status solve_dual_solution();
    void compute_reduced_costs();

    // Direction & Row Computations
    Status compute_ftran_direction(Index entering_col, std::vector<Float64>& d_out) const;
    Status compute_btran_row(Index leaving_slot, std::vector<Float64>& y_p_out) const;
    void compute_row_alphas(std::span<const Float64> y_p, std::vector<Float64>& alphas_out) const;

    // Feasibility Checks
    bool check_primal_feasibility(Float64 tolerance, Float64& max_violation, Index& worst_row) const;
    bool check_dual_feasibility(Float64 tolerance, Float64& max_violation, Index& worst_col) const;

    Float64 compute_objective_value() const;
    
    // Updates
    void update_nonbasic_variable_values();
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_SIMPLEX_STATE_HPP
