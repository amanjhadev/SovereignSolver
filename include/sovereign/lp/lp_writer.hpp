#ifndef SOVEREIGN_LP_LP_WRITER_HPP
#define SOVEREIGN_LP_LP_WRITER_HPP

#include "sovereign/lp/lp_model.hpp"
#include "sovereign/milp/milp_model.hpp"
#include <string>

namespace sovereign::lp {

class LPWriter {
public:
    // Write LPModel to standard MPS format file
    static bool write_mps(const LPModel& model, const std::string& filepath);

    // Write MILPModel to standard MPS format file
    static bool write_mps(const milp::MILPModel& model, const std::string& filepath);

    // Write LPModel to standard LP format file
    static bool write_lp(const LPModel& model, const std::string& filepath);

    // Write MILPModel to standard LP format file
    static bool write_lp(const milp::MILPModel& model, const std::string& filepath);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_LP_WRITER_HPP
