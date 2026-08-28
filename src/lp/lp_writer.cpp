#include "sovereign/lp/lp_writer.hpp"
#include <fstream>
#include <iomanip>

namespace sovereign::lp {

bool LPWriter::write_mps(const LPModel& model, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "NAME          " << model.name() << "\n";
    out << "ROWS\n";
    out << " N  OBJ\n";

    for (Index i = 0; i < model.num_constraints(); ++i) {
        const auto& constr = model.get_constraint(i);
        char sense_char = 'E';
        if (constr.sense == ConstraintSense::LessEqual) sense_char = 'L';
        else if (constr.sense == ConstraintSense::GreaterEqual) sense_char = 'G';
        out << " " << sense_char << "  " << constr.name << "\n";
    }

    out << "COLUMNS\n";
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        if (var.obj_coeff != 0.0) {
            out << "    " << std::left << std::setw(8) << var.name
                << "  OBJ       " << std::right << std::setw(12) << std::setprecision(6) << var.obj_coeff << "\n";
        }
    }

    const auto& matrix = model.get_csc_matrix();
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        auto rows = matrix.get_col_row_indices(j);
        auto vals = matrix.get_col_values(j);
        for (std::size_t p = 0; p < rows.size(); ++p) {
            Index row_idx = rows[p];
            Float64 val = vals[p];
            const auto& constr = model.get_constraint(row_idx);
            out << "    " << std::left << std::setw(8) << var.name
                << "  " << std::left << std::setw(8) << constr.name
                << "  " << std::right << std::setw(12) << std::setprecision(6) << val << "\n";
        }
    }

    out << "RHS\n";
    for (Index i = 0; i < model.num_constraints(); ++i) {
        const auto& constr = model.get_constraint(i);
        if (constr.rhs != 0.0) {
            out << "    RHS1      " << std::left << std::setw(8) << constr.name
                << "  " << std::right << std::setw(12) << std::setprecision(6) << constr.rhs << "\n";
        }
    }

    out << "BOUNDS\n";
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        if (var.lower_bound != 0.0) {
            out << " LO BND1      " << std::left << std::setw(8) << var.name
                << "  " << std::right << std::setw(12) << std::setprecision(6) << var.lower_bound << "\n";
        }
        if (var.upper_bound < 1e20) {
            out << " UP BND1      " << std::left << std::setw(8) << var.name
                << "  " << std::right << std::setw(12) << std::setprecision(6) << var.upper_bound << "\n";
        }
    }

    out << "ENDATA\n";
    return true;
}

bool LPWriter::write_mps(const milp::MILPModel& model, const std::string& filepath) {
    return write_mps(model.lp_model(), filepath);
}

bool LPWriter::write_lp(const LPModel& model, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "\\ Problem name: " << model.name() << "\n";
    if (model.objective_sense() == ObjectiveSense::Minimize) {
        out << "Minimize\n obj: ";
    } else {
        out << "Maximize\n obj: ";
    }

    bool first = true;
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        if (var.obj_coeff == 0.0) continue;
        if (!first && var.obj_coeff > 0.0) out << " + ";
        else if (var.obj_coeff < 0.0) out << " - ";
        out << std::abs(var.obj_coeff) << " " << var.name;
        first = false;
    }
    if (first) out << "0";
    out << "\n\nSubject To\n";

    const auto& matrix = model.get_csc_matrix();
    for (Index i = 0; i < model.num_constraints(); ++i) {
        const auto& constr = model.get_constraint(i);
        out << " " << constr.name << ": ";
        bool first_term = true;
        for (Index j = 0; j < model.num_variables(); ++j) {
            auto rows = matrix.get_col_row_indices(j);
            auto vals = matrix.get_col_values(j);
            Float64 val = 0.0;
            for (std::size_t p = 0; p < rows.size(); ++p) {
                if (rows[p] == i) {
                    val = vals[p];
                    break;
                }
            }
            if (val == 0.0) continue;
            const auto& var = model.get_variable(j);
            if (!first_term && val > 0.0) out << " + ";
            else if (val < 0.0) out << " - ";
            out << std::abs(val) << " " << var.name;
            first_term = false;
        }
        if (first_term) out << "0";
        if (constr.sense == ConstraintSense::LessEqual) out << " <= ";
        else if (constr.sense == ConstraintSense::GreaterEqual) out << " >= ";
        else out << " = ";
        out << constr.rhs << "\n";
    }

    out << "\nBounds\n";
    for (Index j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(j);
        out << " " << var.lower_bound << " <= " << var.name << " <= " << var.upper_bound << "\n";
    }

    out << "End\n";
    return true;
}

bool LPWriter::write_lp(const milp::MILPModel& model, const std::string& filepath) {
    return write_lp(model.lp_model(), filepath);
}

} // namespace sovereign::lp
