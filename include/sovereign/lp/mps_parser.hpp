#ifndef SOVEREIGN_LP_MPS_PARSER_HPP
#define SOVEREIGN_LP_MPS_PARSER_HPP

#include "sovereign/core/status.hpp"
#include "sovereign/lp/lp_model.hpp"
#include <string>
#include <istream>

namespace sovereign::lp {

class MPSParser {
private:
    std::string last_error_{""};
    Int32 error_line_{0};

public:
    MPSParser() = default;

    // Parses an MPS file into an LPModel
    Status parse_file(const std::string& filepath, LPModel& model);

    // Parses an MPS stream into an LPModel
    Status parse_stream(std::istream& is, LPModel& model, const std::string& stream_name = "stream");

    const std::string& last_error() const noexcept { return last_error_; }
    Int32 error_line() const noexcept { return error_line_; }
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_MPS_PARSER_HPP
