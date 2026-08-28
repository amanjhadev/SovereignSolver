#include "sovereign/industrial/industrial_diagnostics.hpp"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace sovereign::industrial {

DiagnosticReport IndustrialDiagnostics::diagnose(const lp::LPModel& model) {
    DiagnosticReport rep;
    rep.model_name = model.name();
    rep.num_variables = model.num_variables();
    rep.num_constraints = model.num_constraints();
    
    const auto& matrix = model.get_csc_matrix();
    rep.num_nonzeros = matrix.num_nonzeros();

    Float64 min_coeff = 1e20;
    Float64 max_coeff = 0.0;

    for (Float64 val : matrix.values()) {
        Float64 abs_val = std::abs(val);
        if (abs_val > 0.0) {
            min_coeff = std::min(min_coeff, abs_val);
            max_coeff = std::max(max_coeff, abs_val);
        }
    }

    if (rep.num_nonzeros > 0 && min_coeff < 1e19) {
        rep.min_abs_coeff = min_coeff;
        rep.max_abs_coeff = max_coeff;
        rep.coeff_dynamic_range = (min_coeff > 0.0) ? (max_coeff / min_coeff) : 0.0;
    }

    if (rep.coeff_dynamic_range > 1e8) {
        rep.warnings.push_back("Extreme matrix coefficient dynamic range (" + std::to_string(rep.coeff_dynamic_range) + " > 1e8)");
        rep.is_numerically_well_conditioned = false;
    }

    // Check bound clashes and unbounded variables
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        if (var.lower_bound > var.upper_bound + 1e-9) {
            rep.bound_clashes++;
            rep.warnings.push_back("Bound clash on variable " + var.name + ": lower (" + std::to_string(var.lower_bound) + ") > upper (" + std::to_string(var.upper_bound) + ")");
        }
        if (var.lower_bound <= -1e19 && var.upper_bound >= 1e19) {
            rep.unbounded_vars++;
        }
    }

    // Check zero rows and columns
    std::vector<std::size_t> row_nz(model.num_constraints(), 0);
    for (Index j = 0; j < model.num_variables(); ++j) {
        auto rows = matrix.get_col_row_indices(j);
        if (rows.empty()) {
            rep.zero_cols++;
        }
        for (Index r : rows) {
            row_nz[r]++;
        }
    }

    for (Index i = 0; i < model.num_constraints(); ++i) {
        if (row_nz[i] == 0) {
            rep.zero_rows++;
            rep.warnings.push_back("Zero constraint row detected: " + model.get_constraint(i).name);
        }
    }

    return rep;
}

DiagnosticReport IndustrialDiagnostics::diagnose(const milp::MILPModel& model) {
    return diagnose(model.lp_model());
}

std::string IndustrialDiagnostics::format_report(const DiagnosticReport& rep) {
    std::ostringstream ss;
    ss << "=== INDUSTRIAL DIAGNOSTIC REPORT: " << rep.model_name << " ===\n"
       << "Variables: " << rep.num_variables << ", Constraints: " << rep.num_constraints << ", Nonzeros: " << rep.num_nonzeros << "\n"
       << "Min Abs Coeff: " << rep.min_abs_coeff << ", Max Abs Coeff: " << rep.max_abs_coeff << "\n"
       << "Dynamic Range: " << rep.coeff_dynamic_range << " (" << (rep.is_numerically_well_conditioned ? "WELL-CONDITIONED" : "HIGH-RANGE WARNING") << ")\n"
       << "Zero Rows: " << rep.zero_rows << ", Zero Cols: " << rep.zero_cols << ", Bound Clashes: " << rep.bound_clashes << "\n";
    if (!rep.warnings.empty()) {
        ss << "Warnings (" << rep.warnings.size() << "):\n";
        for (const auto& w : rep.warnings) {
            ss << "  - " << w << "\n";
        }
    }
    return ss.str();
}

} // namespace sovereign::industrial
