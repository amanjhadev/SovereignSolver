#include "sovereign/industrial/run_report.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace sovereign::industrial {

std::string RunReport::to_json(const RunTelemetry& t) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"model_name\": \"" << t.model_name << "\",\n"
       << "  \"solver_engine\": \"" << t.solver_engine << "\",\n"
       << "  \"dataset_category\": \"" << t.dataset_category << "\",\n"
       << "  \"scalability_tier\": \"" << t.scalability_tier << "\",\n"
       << "  \"problem_metrics\": {\n"
       << "    \"variables\": " << t.variables << ",\n"
       << "    \"constraints\": " << t.constraints << ",\n"
       << "    \"nonzeros\": " << t.nonzeros << ",\n"
       << "    \"memory_mb\": " << std::fixed << std::setprecision(2) << t.memory_mb << ",\n"
       << "    \"presolve_reduction_pct\": " << std::fixed << std::setprecision(2) << t.presolve_reduction_pct << "\n"
       << "  },\n"
       << "  \"performance_metrics\": {\n"
       << "    \"runtime_ms\": " << std::fixed << std::setprecision(3) << t.runtime_ms << ",\n"
       << "    \"iterations\": " << t.iterations << ",\n"
       << "    \"lp_solves\": " << t.lp_solves << ",\n"
       << "    \"bb_nodes\": " << t.bb_nodes << ",\n"
       << "    \"cuts_generated\": " << t.cuts_generated << ",\n"
       << "    \"heuristic_incumbents\": " << t.heuristic_incumbents << ",\n"
       << "    \"threads_used\": " << t.threads_used << "\n"
       << "  },\n"
       << "  \"numerical_quality\": {\n"
       << "    \"status\": \"" << t.status_string << "\",\n"
       << "    \"objective_value\": " << std::setprecision(6) << t.objective_value << ",\n"
       << "    \"mip_gap\": " << std::setprecision(6) << t.mip_gap << ",\n"
       << "    \"primal_residual\": " << std::scientific << std::setprecision(3) << t.primal_residual << ",\n"
       << "    \"dual_residual\": " << std::scientific << std::setprecision(3) << t.dual_residual << "\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

std::string RunReport::to_markdown(const RunTelemetry& t) {
    std::ostringstream ss;
    ss << "# Sovereign Solver Industrial Run Report: " << t.model_name << "\n\n"
       << "- **Engine**: `" << t.solver_engine << "`\n"
       << "- **Category**: `" << t.dataset_category << "` (" << t.scalability_tier << ")\n"
       << "- **Status**: `" << t.status_string << "` | **Objective**: `" << std::fixed << std::setprecision(4) << t.objective_value << "`\n\n"
       << "### Quality & Telemetry Metrics\n\n"
       << "| Metric | Measured Value |\n"
       << "| :--- | :--- |\n"
       << "| Variables / Constraints / Nonzeros | " << t.variables << " / " << t.constraints << " / " << t.nonzeros << " |\n"
       << "| Memory Allocation | " << std::fixed << std::setprecision(2) << t.memory_mb << " MB |\n"
       << "| Presolve Reduction | " << std::fixed << std::setprecision(1) << t.presolve_reduction_pct << "% |\n"
       << "| Total Runtime | " << std::fixed << std::setprecision(3) << t.runtime_ms << " ms |\n"
       << "| Simplex/Barrier Iterations | " << t.iterations << " |\n"
       << "| LP Solves / B&B Nodes | " << t.lp_solves << " / " << t.bb_nodes << " |\n"
       << "| Cuts / Heuristics / Threads | " << t.cuts_generated << " / " << t.heuristic_incumbents << " / " << t.threads_used << " |\n"
       << "| MIP Gap | " << std::fixed << std::setprecision(6) << t.mip_gap << " |\n"
       << "| Primal Residual | " << std::scientific << std::setprecision(2) << t.primal_residual << " |\n"
       << "| Dual Residual | " << std::scientific << std::setprecision(2) << t.dual_residual << " |\n";
    return ss.str();
}

bool RunReport::save_json(const RunTelemetry& t, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << to_json(t);
    return true;
}

bool RunReport::save_markdown(const RunTelemetry& t, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << to_markdown(t);
    return true;
}

} // namespace sovereign::industrial
