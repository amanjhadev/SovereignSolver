#ifndef SOVEREIGN_QP_QP_MODEL_HPP
#define SOVEREIGN_QP_QP_MODEL_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/linalg/coo_matrix.hpp"
#include "sovereign/linalg/csc_matrix.hpp"
#include "sovereign/qp/qp_types.hpp"
#include <span>
#include <vector>

namespace sovereign::qp {

class QPModel {
private:
    lp::LPModel lp_model_;
    
    // Triangular sparse entries (i <= j)
    struct QEntry {
        Index i;
        Index j;
        Float64 val;
    };
    std::vector<QEntry> q_entries_;
    mutable bool csc_dirty_{true};
    mutable linalg::CSCMatrix q_csc_;

    void rebuild_csc_if_needed() const;

public:
    QPModel() = default;
    explicit QPModel(const lp::LPModel& lp_model) : lp_model_(lp_model) {}
    QPModel(std::string name, ObjectiveSense sense) : lp_model_(std::move(name), sense) {}

    // LP Model delegates
    const lp::LPModel& lp_model() const noexcept { return lp_model_; }
    lp::LPModel& lp_model() noexcept { return lp_model_; }

    const std::string& name() const noexcept { return lp_model_.name(); }
    ObjectiveSense objective_sense() const noexcept { return lp_model_.objective_sense(); }
    Float64 objective_offset() const noexcept { return lp_model_.objective_offset(); }
    Index num_variables() const noexcept { return lp_model_.num_variables(); }
    Index num_constraints() const noexcept { return lp_model_.num_constraints(); }

    Index add_variable(const std::string& name, Float64 lb, Float64 ub, Float64 obj_coeff, VariableType type = VariableType::Continuous) {
        csc_dirty_ = true;
        return lp_model_.add_variable(name, lb, ub, obj_coeff, type);
    }

    Index add_constraint(const std::string& name, lp::ConstraintSense sense, Float64 rhs, std::span<const Index> col_indices, std::span<const Float64> values) {
        return lp_model_.add_constraint(name, sense, rhs, col_indices, values);
    }

    // Add symmetric quadratic objective term (1/2 * Q_ij * x_i * x_j for i != j, or 1/2 * Q_ii * x_i^2)
    void add_quadratic_term(Index var1, Index var2, Float64 val);

    // Symmetric Q * x matrix-vector product
    void Q_multiply(std::span<const Float64> x, std::span<Float64> Qx) const;

    // Evaluates 1/2 * x^T * Q * x + c^T * x + c_0
    Float64 evaluate_objective(std::span<const Float64> x) const;

    // Evaluates gradient Q * x + c
    void evaluate_gradient(std::span<const Float64> x, std::span<Float64> grad) const;

    // Validates Q = Q^T and Q >= 0 (positive semidefinite)
    QPStatus validate_convexity() const;

    const std::vector<QEntry>& q_entries() const noexcept { return q_entries_; }
    const linalg::CSCMatrix& get_q_csc() const;
};

} // namespace sovereign::qp

#endif // SOVEREIGN_QP_QP_MODEL_HPP
