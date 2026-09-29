#include "sovereign/lp/mps_parser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace sovereign::lp {

namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split_tokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace

Status MPSParser::parse_file(const std::string& filepath, LPModel& model) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        last_error_ = "Could not open file: " + filepath;
        error_line_ = 0;
        return Status::InvalidInput;
    }
    return parse_stream(file, model, filepath);
}

Status MPSParser::parse_stream(std::istream& is, LPModel& model, const std::string& stream_name) {
    last_error_.clear();
    error_line_ = 0;

    enum class Section {
        None,
        Name,
        ObjSense,
        Rows,
        Columns,
        Rhs,
        Ranges,
        Bounds,
        EndData
    };

    Section section = Section::None;
    std::string line;
    Int32 line_num = 0;

    std::string obj_row_name;
    std::unordered_map<std::string, Index> row_name_to_idx;
    std::unordered_map<std::string, Index> col_name_to_idx;
    bool in_intorg = false;

    while (std::getline(is, line)) {
        line_num++;
        if (line.empty()) continue;

        std::string trimmed_line = trim(line);
        if (trimmed_line.empty()) continue;

        // Skip comment lines (even if preceded by whitespace)
        if (trimmed_line[0] == '*' || trimmed_line[0] == '$') continue;

        // Check for section headers (unindented or keyword match)
        if (line[0] != ' ' && line[0] != '\t') {
            auto tokens = split_tokens(line);
            if (!tokens.empty()) {
                std::string header = tokens[0];
                std::transform(header.begin(), header.end(), header.begin(), ::toupper);

                if (header == "NAME") {
                    section = Section::Name;
                    std::string model_name = (tokens.size() > 1) ? tokens[1] : "mps_model";
                    model.set_name(model_name);
                    continue;
                } else if (header == "OBJSENSE") {
                    section = Section::ObjSense;
                    if (tokens.size() > 1) {
                        std::string sense_str = tokens[1];
                        std::transform(sense_str.begin(), sense_str.end(), sense_str.begin(), ::toupper);
                        if (sense_str == "MAX" || sense_str == "MAXIMIZE") {
                            model.set_objective_sense(ObjectiveSense::Maximize);
                        } else if (sense_str == "MIN" || sense_str == "MINIMIZE") {
                            model.set_objective_sense(ObjectiveSense::Minimize);
                        } else {
                            last_error_ = "Unknown OBJSENSE value '" + sense_str + "' in " + stream_name;
                            error_line_ = line_num;
                            return Status::InvalidInput;
                        }
                    }
                    continue;
                } else if (header == "ROWS") {
                    section = Section::Rows;
                    continue;
                } else if (header == "COLUMNS") {
                    section = Section::Columns;
                    continue;
                } else if (header == "RHS") {
                    section = Section::Rhs;
                    continue;
                } else if (header == "RANGES") {
                    section = Section::Ranges;
                    continue;
                } else if (header == "BOUNDS") {
                    section = Section::Bounds;
                    continue;
                } else if (header == "ENDATA") {
                    section = Section::EndData;
                    break;
                } else {
                    last_error_ = "Unsupported or unknown MPS section header '" + header + "' in " + stream_name;
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }
            }
        }

        auto tokens = split_tokens(line);
        if (tokens.empty()) continue;

        switch (section) {
            case Section::ObjSense: {
                if (!tokens.empty()) {
                    std::string sense_str = tokens[0];
                    std::transform(sense_str.begin(), sense_str.end(), sense_str.begin(), ::toupper);
                    if (sense_str == "MAX" || sense_str == "MAXIMIZE") {
                        model.set_objective_sense(ObjectiveSense::Maximize);
                    } else if (sense_str == "MIN" || sense_str == "MINIMIZE") {
                        model.set_objective_sense(ObjectiveSense::Minimize);
                    } else {
                        last_error_ = "Unknown OBJSENSE value '" + sense_str + "' in " + stream_name;
                        error_line_ = line_num;
                        return Status::InvalidInput;
                    }
                }
                break;
            }

            case Section::Rows: {
                // Format: TYPE ROW_NAME
                if (tokens.size() < 2) {
                    last_error_ = "Invalid ROWS entry format in " + stream_name;
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }
                char type_char = std::toupper(tokens[0][0]);
                std::string rname = tokens[1];

                if (type_char == 'N') {
                    if (obj_row_name.empty()) {
                        obj_row_name = rname;
                    }
                } else {
                    ConstraintSense sense;
                    Float64 rhs_val = 0.0;
                    if (type_char == 'L') {
                        sense = ConstraintSense::LessEqual;
                        rhs_val = 0.0;
                    } else if (type_char == 'G') {
                        sense = ConstraintSense::GreaterEqual;
                        rhs_val = 0.0;
                    } else if (type_char == 'E') {
                        sense = ConstraintSense::Equal;
                        rhs_val = 0.0;
                    } else {
                        last_error_ = "Unknown constraint sense '" + std::string(1, type_char) + "' in " + stream_name;
                        error_line_ = line_num;
                        return Status::InvalidInput;
                    }

                    Index r_idx = model.add_constraint(rname, sense, rhs_val, {}, {});
                    row_name_to_idx[rname] = r_idx;
                }
                break;
            }

            case Section::Columns: {
                // Check for integer marker lines: 'MARKER' 'INTORG' or 'INTEND'
                if (line.find("MARKER") != std::string::npos) {
                    if (line.find("INTORG") != std::string::npos) {
                        in_intorg = true;
                        continue;
                    }
                    if (line.find("INTEND") != std::string::npos) {
                        in_intorg = false;
                        continue;
                    }
                }

                // Format: COL_NAME ROW_NAME VAL [ROW_NAME2 VAL2]
                if (tokens.size() < 3) {
                    last_error_ = "Invalid COLUMNS entry format in " + stream_name;
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }

                std::string cname = tokens[0];
                Index col_idx;

                auto it = col_name_to_idx.find(cname);
                if (it == col_name_to_idx.end()) {
                    VariableType vtype = in_intorg ? VariableType::Binary : VariableType::Continuous;
                    Float64 ub = in_intorg ? 1.0 : INFINITY_VAL;
                    col_idx = model.add_variable(cname, 0.0, ub, 0.0, vtype);
                    col_name_to_idx[cname] = col_idx;
                } else {
                    col_idx = it->second;
                    if (in_intorg) {
                        auto& var = model.get_variable(col_idx);
                        var.type = VariableType::Binary;
                        if (var.upper_bound == INFINITY_VAL) {
                            var.upper_bound = 1.0;
                        }
                    }
                }

                for (std::size_t k = 1; k + 1 < tokens.size(); k += 2) {
                    std::string rname = tokens[k];
                    Float64 val = std::stod(tokens[k + 1]);

                    if (rname == obj_row_name) {
                        model.get_variable(col_idx).obj_coeff = val;
                    } else {
                        auto rit = row_name_to_idx.find(rname);
                        if (rit != row_name_to_idx.end()) {
                            model.add_matrix_entry(rit->second, col_idx, val);
                        } else {
                            last_error_ = "Referenced row '" + rname + "' not found in ROWS section";
                            error_line_ = line_num;
                            return Status::InvalidInput;
                        }
                    }
                }
                break;
            }

            case Section::Rhs: {
                // Format: RHS_NAME ROW_NAME VAL [ROW_NAME2 VAL2]
                std::size_t start_idx = (tokens.size() % 2 == 1) ? 1 : 0;
                for (std::size_t k = start_idx; k + 1 < tokens.size(); k += 2) {
                    std::string rname = tokens[k];
                    Float64 val = std::stod(tokens[k + 1]);

                    if (rname == obj_row_name) {
                        model.set_objective_offset(-val);
                    } else {
                        auto rit = row_name_to_idx.find(rname);
                        if (rit != row_name_to_idx.end()) {
                            auto& c = model.get_constraint(rit->second);
                            if (c.sense == ConstraintSense::LessEqual) {
                                c.rhs = val;
                            } else if (c.sense == ConstraintSense::GreaterEqual) {
                                c.lhs = val;
                                c.rhs = INFINITY_VAL;
                            } else if (c.sense == ConstraintSense::Equal) {
                                c.lhs = val;
                                c.rhs = val;
                            }
                        }
                    }
                }
                break;
            }

            case Section::Ranges: {
                // Format: [RANGE_NAME] ROW_NAME VAL [ROW_NAME2 VAL2]
                std::size_t start_idx = (tokens.size() % 2 == 1) ? 1 : 0;
                for (std::size_t k = start_idx; k + 1 < tokens.size(); k += 2) {
                    std::string rname = tokens[k];
                    Float64 r_val = 0.0;
                    try {
                        r_val = std::stod(tokens[k + 1]);
                    } catch (...) {
                        last_error_ = "Invalid numeric range value '" + tokens[k + 1] + "' in RANGES section";
                        error_line_ = line_num;
                        return Status::InvalidInput;
                    }

                    auto rit = row_name_to_idx.find(rname);
                    if (rit == row_name_to_idx.end()) {
                        last_error_ = "Referenced row '" + rname + "' in RANGES section not found in ROWS";
                        error_line_ = line_num;
                        return Status::InvalidInput;
                    }

                    auto& c = model.get_constraint(rit->second);
                    if (c.sense == ConstraintSense::LessEqual) {
                        Float64 b = c.rhs;
                        c.lhs = b - std::abs(r_val);
                        c.rhs = b;
                        c.sense = ConstraintSense::Range;
                    } else if (c.sense == ConstraintSense::GreaterEqual) {
                        Float64 b = c.lhs;
                        c.lhs = b;
                        c.rhs = b + std::abs(r_val);
                        c.sense = ConstraintSense::Range;
                    } else if (c.sense == ConstraintSense::Equal) {
                        Float64 b = c.rhs;
                        if (r_val >= 0.0) {
                            c.lhs = b;
                            c.rhs = b + r_val;
                        } else {
                            c.lhs = b + r_val;
                            c.rhs = b;
                        }
                        c.sense = ConstraintSense::Range;
                    }
                }
                break;
            }

            case Section::Bounds: {
                // Format: TYPE BOUND_NAME COL_NAME [VAL] or TYPE COL_NAME [VAL]
                if (tokens.size() < 2) {
                    last_error_ = "Invalid BOUNDS entry format in " + stream_name;
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }

                std::string btype = tokens[0];
                std::transform(btype.begin(), btype.end(), btype.begin(), ::toupper);

                std::string cname;
                Float64 val = 0.0;

                if (tokens.size() >= 4) {
                    cname = tokens[2];
                    try { val = std::stod(tokens[3]); } catch (...) { val = 0.0; }
                } else if (tokens.size() == 3) {
                    if (col_name_to_idx.find(tokens[2]) != col_name_to_idx.end()) {
                        cname = tokens[2];
                        val = 0.0;
                    } else if (col_name_to_idx.find(tokens[1]) != col_name_to_idx.end()) {
                        cname = tokens[1];
                        try { val = std::stod(tokens[2]); } catch (...) { val = 0.0; }
                    } else {
                        cname = tokens[2];
                    }
                } else if (tokens.size() == 2) {
                    cname = tokens[1];
                }

                auto cit = col_name_to_idx.find(cname);
                if (cit == col_name_to_idx.end()) {
                    last_error_ = "Referenced column '" + cname + "' in BOUNDS not found in COLUMNS";
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }

                auto& var = model.get_variable(cit->second);

                if (btype == "LO") {
                    var.lower_bound = val;
                } else if (btype == "UP") {
                    var.upper_bound = val;
                } else if (btype == "FX") {
                    var.lower_bound = val;
                    var.upper_bound = val;
                } else if (btype == "FR") {
                    var.lower_bound = -INFINITY_VAL;
                    var.upper_bound = INFINITY_VAL;
                } else if (btype == "MI") {
                    var.lower_bound = -INFINITY_VAL;
                } else if (btype == "PL") {
                    var.upper_bound = INFINITY_VAL;
                } else if (btype == "BV") {
                    var.lower_bound = 0.0;
                    var.upper_bound = 1.0;
                    var.type = VariableType::Binary;
                } else if (btype == "UI") {
                    var.type = VariableType::Integer;
                    var.upper_bound = val;
                } else {
                    last_error_ = "Unsupported MPS bound type '" + btype + "' in " + stream_name;
                    error_line_ = line_num;
                    return Status::InvalidInput;
                }
                break;
            }

            default:
                break;
        }
    }

    if (model.num_variables() == 0 && model.num_constraints() == 0) {
        last_error_ = "MPS parsing produced an empty model (0 variables, 0 constraints) in " + stream_name;
        return Status::InvalidInput;
    }

    return Status::Ok;
}

} // namespace sovereign::lp
