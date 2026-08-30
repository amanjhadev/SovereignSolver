#include "sovereign/public_solver_api.hpp"
#include "sovereign/lp/dual_simplex.hpp"
#include "sovereign/milp/milp_solver.hpp"
#include "sovereign/lp/mps_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <cmath>

using namespace sovereign;

std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

int main(int argc, char* argv[]) {
    std::string mps_path = "";
    double timeout_sec = 30.0;
    int num_threads = 1;
    bool validate_only = false;
    bool stream_telemetry = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mps" && i + 1 < argc) {
            mps_path = argv[++i];
        } else if (arg == "--timeout" && i + 1 < argc) {
            timeout_sec = std::stod(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "--validate-only") {
            validate_only = true;
        } else if (arg == "--stream-telemetry") {
            stream_telemetry = true;
        }
    }

    if (mps_path.empty()) {
        std::cout << "{\"success\": false, \"error\": \"Missing --mps filepath parameter\"}\n";
        return 1;
    }

    lp::MPSParser parser;
    lp::LPModel parsed_lp;
    Status parse_st = parser.parse_file(mps_path, parsed_lp);
    if (!is_ok(parse_st) || (parsed_lp.num_variables() == 0 && parsed_lp.num_constraints() == 0)) {
        std::string err_msg = parser.last_error();
        if (err_msg.empty()) err_msg = "MPS parsing produced an empty model (0 variables, 0 constraints)";
        std::cout << "{\"success\": false, \"error\": \"" << escape_json(err_msg) << "\"}\n";
        return 1;
    }

    milp::MILPModel milp_model(parsed_lp);

    Index n_vars = milp_model.num_variables();
    Index n_bin = milp_model.num_integer_variables();
    Index n_cont = n_vars - n_bin;
    Index n_cons = milp_model.num_constraints();
    Index n_nnz = parsed_lp.get_csc_matrix().num_nonzeros();
    std::string sense_str = (milp_model.objective_sense() == ObjectiveSense::Minimize) ? "MINIMIZE" : "MAXIMIZE";

    if (validate_only) {
        std::cout << "{\n";
        std::cout << "  \"success\": true,\n";
        std::cout << "  \"model_name\": \"" << escape_json(parsed_lp.name()) << "\",\n";
        std::cout << "  \"variables\": " << n_vars << ",\n";
        std::cout << "  \"binaries\": " << n_bin << ",\n";
        std::cout << "  \"continuous\": " << n_cont << ",\n";
        std::cout << "  \"constraints\": " << n_cons << ",\n";
        std::cout << "  \"nnz\": " << n_nnz << ",\n";
        std::cout << "  \"sense\": \"" << sense_str << "\"\n";
        std::cout << "}\n";
        return 0;
    }

    // Full Solve
    milp::MILPConfig milp_cfg;
    milp_cfg.num_threads = num_threads;
    milp_cfg.parallel_mode = (num_threads > 1) ? milp::ParallelMode::Deterministic : milp::ParallelMode::Serial;
    milp_cfg.max_time_sec = timeout_sec;
    milp_cfg.enable_cuts = true;
    milp_cfg.enable_diving = true;
    milp_cfg.enable_feasibility_pump = true;

    if (stream_telemetry) {
        milp_cfg.telemetry_callback = [](const std::string& line) {
            std::cout << line << "\n" << std::flush;
        };
    }

    // Profile Root LP
    SolverConfig lp_cfg;
    lp_cfg.max_iterations = 20000;
    lp::DualSimplexSolver root_lp_solver(lp_cfg);
    auto root_res = root_lp_solver.solve(milp_model.lp_model());

    Index frac_bin_cnt = 0;
    for (Index j = 0; j < milp_model.num_variables(); ++j) {
        if (milp_model.is_integer(j) && !root_res.primal_solution.empty()) {
            Float64 val = root_res.primal_solution[j];
            if (std::abs(val - std::round(val)) > 1e-6) frac_bin_cnt++;
        }
    }

    milp::MILPSolver solver(milp_cfg);
    auto start_t = std::chrono::high_resolution_clock::now();
    auto res = solver.solve(milp_model);
    auto end_t = std::chrono::high_resolution_clock::now();
    double solve_time_ms = std::chrono::duration<double, std::milli>(end_t - start_t).count();

    std::string status_str = "Unknown";
    switch (res.status) {
        case milp::MILPStatus::Optimal: status_str = "Optimal"; break;
        case milp::MILPStatus::Infeasible: status_str = "Infeasible"; break;
        case milp::MILPStatus::Unbounded: status_str = "Unbounded"; break;
        case milp::MILPStatus::TimeLimit: status_str = "TimeLimit"; break;
        case milp::MILPStatus::NodeLimit: status_str = "NodeLimit"; break;
        case milp::MILPStatus::NumericalFailure: status_str = "NumericalFailure"; break;
        case milp::MILPStatus::Unresolved: status_str = "Unresolved"; break;
    }

    std::cout << "{\n";
    std::cout << "  \"success\": true,\n";
    std::cout << "  \"model_name\": \"" << escape_json(parsed_lp.name()) << "\",\n";
    std::cout << "  \"status_code\": " << static_cast<int>(res.status) << ",\n";
    std::cout << "  \"status_name\": \"" << status_str << "\",\n";
    std::cout << "  \"message\": \"" << escape_json(res.message) << "\",\n";
    std::cout << "  \"objective_value\": " << std::fixed << std::setprecision(6) << res.objective_value << ",\n";
    std::cout << "  \"best_bound\": " << std::fixed << std::setprecision(6) << res.best_bound << ",\n";
    std::cout << "  \"mip_gap\": " << std::fixed << std::setprecision(6) << res.mip_gap << ",\n";
    std::cout << "  \"nodes_explored\": " << res.nodes_explored << ",\n";
    std::cout << "  \"nodes_created\": " << res.nodes_created << ",\n";
    std::cout << "  \"lp_solves\": " << res.lp_solves << ",\n";
    std::cout << "  \"solve_time_ms\": " << std::fixed << std::setprecision(3) << solve_time_ms << ",\n";

    // Telemetry section
    std::cout << "  \"telemetry\": {\n";
    std::cout << "    \"variables\": " << n_vars << ",\n";
    std::cout << "    \"binaries\": " << n_bin << ",\n";
    std::cout << "    \"continuous\": " << n_cont << ",\n";
    std::cout << "    \"constraints\": " << n_cons << ",\n";
    std::cout << "    \"nnz\": " << n_nnz << ",\n";
    std::cout << "    \"sense\": \"" << sense_str << "\",\n";
    std::cout << "    \"root_lp_status\": " << static_cast<int>(root_res.status) << ",\n";
    std::cout << "    \"root_lp_objective\": " << std::fixed << std::setprecision(6) << root_res.objective_value << ",\n";
    std::cout << "    \"root_lp_iterations\": " << root_res.iterations << ",\n";
    std::cout << "    \"fractional_binaries\": " << frac_bin_cnt << ",\n";
    std::cout << "    \"cuts_generated\": " << res.cuts_generated << ",\n";
    std::cout << "    \"cuts_accepted\": " << res.cuts_accepted << ",\n";
    std::cout << "    \"heuristic_incumbents\": " << res.heuristic_incumbents_found << "\n";
    std::cout << "  },\n";

    // Solution map
    std::cout << "  \"solution\": {\n";
    bool first = true;
    if (!res.primal_solution.empty()) {
        for (Index j = 0; j < milp_model.num_variables(); ++j) {
            const auto& var = milp_model.lp_model().get_variable(j);
            Float64 val = res.primal_solution[j];
            if (std::abs(val) > 1e-9 || milp_model.is_integer(j)) {
                if (!first) std::cout << ",\n";
                first = false;
                std::cout << "    \"" << escape_json(var.name) << "\": " << std::fixed << std::setprecision(6) << val;
            }
        }
    }
    std::cout << "\n  }\n";
    std::cout << "}\n";

    return 0;
}
