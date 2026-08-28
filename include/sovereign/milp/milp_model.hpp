#ifndef SOVEREIGN_MILP_MILP_MODEL_HPP
#define SOVEREIGN_MILP_MILP_MODEL_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include "sovereign/milp/milp_types.hpp"
#include <span>
#include <vector>

namespace sovereign::milp {

class MILPModel {
private:
    lp::LPModel lp_model_;
    std::vector<VariableType> var_types_;

public:
    MILPModel() = default;
    explicit MILPModel(const lp::LPModel& lp_model);
    MILPModel(std::string name, ObjectiveSense sense);

    const lp::LPModel& lp_model() const noexcept { return lp_model_; }
    lp::LPModel& lp_model() noexcept { return lp_model_; }

    const std::string& name() const noexcept { return lp_model_.name(); }
    ObjectiveSense objective_sense() const noexcept { return lp_model_.objective_sense(); }
    Float64 objective_offset() const noexcept { return lp_model_.objective_offset(); }
    Index num_variables() const noexcept { return lp_model_.num_variables(); }
    Index num_constraints() const noexcept { return lp_model_.num_constraints(); }

    Index add_variable(
        const std::string& name,
        Float64 lb,
        Float64 ub,
        Float64 obj_coeff,
        VariableType type = VariableType::Continuous
    );

    Index add_constraint(
        const std::string& name,
        lp::ConstraintSense sense,
        Float64 rhs,
        std::span<const Index> col_indices,
        std::span<const Float64> values
    );

    void set_variable_type(Index var_idx, VariableType type);
    VariableType variable_type(Index var_idx) const;

    bool is_integer(Index var_idx) const;
    bool is_binary(Index var_idx) const;
    bool is_continuous(Index var_idx) const;

    std::vector<Index> integer_variable_indices() const;
    Index num_integer_variables() const;
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_MILP_MODEL_HPP
