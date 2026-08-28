#ifndef SOVEREIGN_INDUSTRIAL_INDUSTRIAL_DIAGNOSTICS_HPP
#define SOVEREIGN_INDUSTRIAL_INDUSTRIAL_DIAGNOSTICS_HPP

#include "sovereign/lp/lp_model.hpp"
#include "sovereign/milp/milp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct DiagnosticReport {
    std::string model_name;
    std::size_t num_variables{0};
    std::size_t num_constraints{0};
    std::size_t num_nonzeros{0};
    Float64 min_abs_coeff{0.0};
    Float64 max_abs_coeff{0.0};
    Float64 coeff_dynamic_range{0.0}; // max / min ratio
    
    std::size_t zero_rows{0};
    std::size_t zero_cols{0};
    std::size_t bound_clashes{0};
    std::size_t unbounded_vars{0};
    
    std::vector<std::string> warnings;
    bool is_numerically_well_conditioned{true};
};

class IndustrialDiagnostics {
public:
    static DiagnosticReport diagnose(const lp::LPModel& model);
    static DiagnosticReport diagnose(const milp::MILPModel& model);
    static std::string format_report(const DiagnosticReport& report);
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_INDUSTRIAL_DIAGNOSTICS_HPP
