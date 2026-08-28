#ifndef SOVEREIGN_CORE_CONFIG_HPP
#define SOVEREIGN_CORE_CONFIG_HPP

#include "sovereign/core/types.hpp"
#include <string>

namespace sovereign {

struct SolverConfig {
    // Numerical Tolerances
    Float64 zero_tolerance              {1e-13};
    Float64 drop_tolerance              {1e-14};
    Float64 pivot_threshold             {0.1};   // Markowitz threshold partial pivoting (u in [0.01, 1.0])
    Float64 pivot_tolerance             {1e-10}; // Minimum absolute pivot magnitude allowed
    Float64 feasibility_tolerance       {1e-6};
    Float64 optimality_tolerance        {1e-6};
    Float64 max_condition_number        {1e13};

    // Refactorization & Iteration controls
    Int32 max_refactorization_interval  {50};
    Int64 max_iterations                {10000};

    // Logging & Diagnostics
    Int32 verbosity_level               {1};     // 0 = Silent, 1 = Info, 2 = Debug
    std::string log_file_path           {""};    // Empty = stdout

    // Factory method
    static SolverConfig default_config();
};

} // namespace sovereign

#endif // SOVEREIGN_CORE_CONFIG_HPP
