#ifndef SOVEREIGN_INDUSTRIAL_RUN_REPORT_HPP
#define SOVEREIGN_INDUSTRIAL_RUN_REPORT_HPP

#include "sovereign/core/types.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct RunTelemetry {
    std::string model_name;
    std::string solver_engine;
    std::string dataset_category; // "Standard Benchmark", "Open-Literature Case Study", "Synthetic Stress Test"
    std::string scalability_tier;  // "Tier 1", "Tier 2", "Tier 3", "Tier 4 Candidate"
    
    std::size_t variables{0};
    std::size_t constraints{0};
    std::size_t nonzeros{0};
    Float64 memory_mb{0.0};
    Float64 presolve_reduction_pct{0.0};
    
    Float64 runtime_ms{0.0};
    std::size_t iterations{0};
    std::size_t lp_solves{0};
    Int64 bb_nodes{0};
    
    std::size_t cuts_generated{0};
    std::size_t heuristic_incumbents{0};
    std::size_t threads_used{1};
    
    Float64 objective_value{0.0};
    Float64 mip_gap{0.0};
    Float64 primal_residual{0.0};
    Float64 dual_residual{0.0};
    std::string status_string;
};

class RunReport {
public:
    static std::string to_json(const RunTelemetry& telemetry);
    static std::string to_markdown(const RunTelemetry& telemetry);
    static bool save_json(const RunTelemetry& telemetry, const std::string& filepath);
    static bool save_markdown(const RunTelemetry& telemetry, const std::string& filepath);
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_RUN_REPORT_HPP
