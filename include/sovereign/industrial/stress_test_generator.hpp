#ifndef SOVEREIGN_INDUSTRIAL_STRESS_TEST_GENERATOR_HPP
#define SOVEREIGN_INDUSTRIAL_STRESS_TEST_GENERATOR_HPP

#include "sovereign/lp/lp_model.hpp"
#include "sovereign/milp/milp_model.hpp"
#include <string>

namespace sovereign::industrial {

class StressTestGenerator {
public:
    // Generate large sparse deterministic continuous LP model
    static lp::LPModel generate_sparse_lp(const std::string& name, std::size_t num_vars, std::size_t num_constraints, Float64 density = 0.005);

    // Generate large sparse deterministic MILP model
    static milp::MILPModel generate_sparse_milp(const std::string& name, std::size_t num_vars, std::size_t num_constraints, Float64 binary_ratio = 0.2, Float64 density = 0.005);
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_STRESS_TEST_GENERATOR_HPP
