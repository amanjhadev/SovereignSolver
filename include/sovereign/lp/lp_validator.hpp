#ifndef SOVEREIGN_LP_LP_VALIDATOR_HPP
#define SOVEREIGN_LP_LP_VALIDATOR_HPP

#include "sovereign/lp/lp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::lp {

struct ValidationResult {
    bool is_valid{true};
    bool is_infeasible{false};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class LPValidator {
public:
    // Performs comprehensive numerical, dimensional, and structural validation of an LPModel
    static ValidationResult validate(const LPModel& model);
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_LP_VALIDATOR_HPP
