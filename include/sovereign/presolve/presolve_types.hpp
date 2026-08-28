#ifndef SOVEREIGN_PRESOLVE_PRESOLVE_TYPES_HPP
#define SOVEREIGN_PRESOLVE_PRESOLVE_TYPES_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::presolve {

enum class PresolveStatus : std::uint8_t {
    Clean            = 0,
    Infeasible       = 1,
    Unbounded        = 2,
    IterationLimit   = 3,
    NumericalFailure = 4
};

enum class ReductionType : std::uint8_t {
    FixedVariable        = 0,
    EmptyRow             = 1,
    EmptyColumn          = 2,
    RowSingleton         = 3,
    FreeColumnSingleton  = 4,
    BoundTightening      = 5,
    RedundantRow         = 6
};

struct PresolveReduction {
    ReductionType type{ReductionType::FixedVariable};
    Index row_index{-1};
    Index col_index{-1};
    std::string var_name;
    std::string constraint_name;
    
    Float64 fixed_value{0.0};
    Float64 old_lower_bound{-INFINITY_VAL};
    Float64 old_upper_bound{INFINITY_VAL};
    Float64 old_rhs{0.0};
    Float64 old_lhs{-INFINITY_VAL};
    lp::ConstraintSense old_sense{lp::ConstraintSense::LessEqual};
    
    std::vector<Index> col_indices;
    std::vector<Float64> coefficients;
    
    Float64 objective_coeff{0.0};
    Float64 objective_shift{0.0};
    VariableType var_type{VariableType::Continuous};
};

struct PresolveConfig {
    Int32 max_passes{20};
    Float64 tolerance{1e-6};
    Float64 pivot_tolerance{1e-9};
    bool enable_free_col_singleton{true};
    bool enable_empty_cols{true};
    bool enable_row_singleton{true};
};

} // namespace sovereign::presolve

#endif // SOVEREIGN_PRESOLVE_PRESOLVE_TYPES_HPP
