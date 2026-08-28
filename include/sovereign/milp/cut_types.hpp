#ifndef SOVEREIGN_MILP_CUT_TYPES_HPP
#define SOVEREIGN_MILP_CUT_TYPES_HPP

#include "sovereign/core/types.hpp"
#include "sovereign/lp/lp_model.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace sovereign::milp {

enum class CutScope : std::uint8_t {
    Global       = 0, // Valid across all B&B nodes
    LocalSubtree = 1  // Valid only within origin node and its descendants
};

enum class CutType : std::uint8_t {
    GomoryFractional     = 0,
    CanonicalMIR         = 1,
    MinimalKnapsackCover = 2
};

struct Cut {
    std::string name;
    lp::ConstraintSense sense{lp::ConstraintSense::LessEqual};
    Float64 rhs{0.0};

    std::vector<Index> col_indices;
    std::vector<Float64> coefficients;

    CutScope scope{CutScope::Global};
    Int64 origin_node_id{-1}; // -1 for global root cuts

    CutType type{CutType::GomoryFractional};

    Float64 efficacy{0.0};    // Norm-scaled violation: (a^T x* - b) / ||a||_2
    Float64 parallelism{0.0}; // Cosine similarity with existing active rows
};

} // namespace sovereign::milp

#endif // SOVEREIGN_MILP_CUT_TYPES_HPP
