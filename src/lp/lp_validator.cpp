#include "sovereign/lp/lp_validator.hpp"
#include <cmath>
#include <sstream>

namespace sovereign::lp {

ValidationResult LPValidator::validate(const LPModel& model) {
    ValidationResult res;

    Index n = model.num_variables();
    Index m = model.num_constraints();

    if (n <= 0) {
        res.is_valid = false;
        res.errors.push_back("Model has 0 variables");
        return res;
    }

    // 1. Variable Validation
    for (Index j = 0; j < n; ++j) {
        const auto& var = model.get_variable(j);
        if (std::isnan(var.lower_bound) || std::isnan(var.upper_bound)) {
            res.is_valid = false;
            res.errors.push_back("Variable '" + var.name + "' has NaN lower or upper bound");
        }
        if (var.lower_bound > var.upper_bound + 1e-12) {
            res.is_valid = false;
            res.is_infeasible = true;
            res.errors.push_back("Variable '" + var.name + "' has lower bound (" +
                                 std::to_string(var.lower_bound) + ") > upper bound (" +
                                 std::to_string(var.upper_bound) + ")");
        }
        if (std::isnan(var.obj_coeff) || std::isinf(var.obj_coeff)) {
            res.is_valid = false;
            res.errors.push_back("Variable '" + var.name + "' has NaN or Inf objective coefficient");
        }
    }

    // 2. Constraint Validation
    for (Index i = 0; i < m; ++i) {
        const auto& c = model.get_constraint(i);
        if (std::isnan(c.lhs) || std::isnan(c.rhs)) {
            res.is_valid = false;
            res.errors.push_back("Constraint '" + c.name + "' has NaN bounds");
        }
        if (c.lhs > c.rhs + 1e-12) {
            res.is_valid = false;
            res.is_infeasible = true;
            res.errors.push_back("Constraint '" + c.name + "' has lhs (" +
                                 std::to_string(c.lhs) + ") > rhs (" +
                                 std::to_string(c.rhs) + ")");
        }
    }

    // 3. Matrix Coefficients Validation
    const auto& csc = model.get_csc_matrix();
    if (!csc.is_valid()) {
        res.is_valid = false;
        res.errors.push_back("Constraint matrix CSC structure is invalid");
    }

    auto vals = csc.values();
    for (std::size_t k = 0; k < vals.size(); ++k) {
        if (std::isnan(vals[k]) || std::isinf(vals[k])) {
            res.is_valid = false;
            res.errors.push_back("Constraint matrix contains NaN or Inf value at index " + std::to_string(k));
        }
    }

    return res;
}

} // namespace sovereign::lp
