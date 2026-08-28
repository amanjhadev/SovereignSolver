#include "sovereign/industrial/stress_test_generator.hpp"
#include <cmath>

namespace sovereign::industrial {

lp::LPModel StressTestGenerator::generate_sparse_lp(const std::string& name, std::size_t num_vars, std::size_t num_constraints, Float64 density) {
    lp::LPModel model(name, ObjectiveSense::Minimize);

    for (std::size_t j = 0; j < num_vars; ++j) {
        Float64 cost = 1.0 + static_cast<Float64>(j % 10);
        model.add_variable("x_" + std::to_string(j), 0.0, 100.0, cost);
    }

    std::size_t nonzeros_per_row = std::max<std::size_t>(2, static_cast<std::size_t>(num_vars * density));

    for (std::size_t i = 0; i < num_constraints; ++i) {
        std::vector<Index> cols;
        std::vector<Float64> vals;

        for (std::size_t k = 0; k < nonzeros_per_row; ++k) {
            Index col_idx = static_cast<Index>((i * 7 + k * 13 + (i + k * 3)) % num_vars);
            Float64 coeff = 1.0 + static_cast<Float64>((i + k) % 5);
            cols.push_back(col_idx);
            vals.push_back(coeff);
        }

        Float64 rhs = 50.0 + static_cast<Float64>(i % 20);
        model.add_constraint("c_" + std::to_string(i), lp::ConstraintSense::LessEqual, rhs, cols, vals);
    }

    return model;
}

milp::MILPModel StressTestGenerator::generate_sparse_milp(const std::string& name, std::size_t num_vars, std::size_t num_constraints, Float64 binary_ratio, Float64 density) {
    milp::MILPModel model(name, ObjectiveSense::Minimize);

    std::size_t num_binaries = static_cast<std::size_t>(num_vars * binary_ratio);

    for (std::size_t j = 0; j < num_vars; ++j) {
        Float64 cost = 1.0 + static_cast<Float64>(j % 10);
        VariableType vtype = (j < num_binaries) ? VariableType::Binary : VariableType::Continuous;
        Float64 ub = (vtype == VariableType::Binary) ? 1.0 : 100.0;
        model.add_variable("x_" + std::to_string(j), 0.0, ub, cost, vtype);
    }

    std::size_t nonzeros_per_row = std::max<std::size_t>(2, static_cast<std::size_t>(num_vars * density));

    for (std::size_t i = 0; i < num_constraints; ++i) {
        std::vector<Index> cols;
        std::vector<Float64> vals;

        for (std::size_t k = 0; k < nonzeros_per_row; ++k) {
            Index col_idx = static_cast<Index>((i * 7 + k * 13 + (i + k * 3)) % num_vars);
            Float64 coeff = 1.0 + static_cast<Float64>((i + k) % 5);
            cols.push_back(col_idx);
            vals.push_back(coeff);
        }

        Float64 rhs = 50.0 + static_cast<Float64>(i % 20);
        model.add_constraint("c_" + std::to_string(i), lp::ConstraintSense::LessEqual, rhs, cols, vals);
    }

    return model;
}

} // namespace sovereign::industrial
