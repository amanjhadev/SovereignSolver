#ifndef SOVEREIGN_CORE_TYPES_HPP
#define SOVEREIGN_CORE_TYPES_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <cmath>

namespace sovereign {

// Fundamental Primitive Types
using Float64 = double;
using Int32   = std::int32_t;
using Int64   = std::int64_t;
using Index   = std::int32_t; // Standard 32-bit index for sparse pointers/indices

// Numerical Constants
constexpr Float64 INFINITY_VAL    = 1e30;
constexpr Float64 EPSILON_DEFAULT = 1e-12;
constexpr Float64 ZERO_TOLERANCE  = 1e-14;
constexpr Float64 PIVOT_TOLERANCE = 1e-10;

// Optimization Objective Direction
enum class ObjectiveSense : std::uint8_t {
    Minimize = 0,
    Maximize = 1
};

// Variable Bound Types
enum class BoundType : std::uint8_t {
    Free       = 0,
    Lower      = 1,
    Upper      = 2,
    Bounded    = 3,
    Fixed      = 4
};

// Variable Types
enum class VariableType : std::uint8_t {
    Continuous = 0,
    Integer    = 1,
    Binary     = 2,
    SemiContinuous = 3,
    SemiInteger    = 4
};

} // namespace sovereign

#endif // SOVEREIGN_CORE_TYPES_HPP
