#ifndef SOVEREIGN_CORE_STATUS_HPP
#define SOVEREIGN_CORE_STATUS_HPP

#include <string_view>
#include <cstdint>

namespace sovereign {

enum class Status : std::int32_t {
    Ok                     = 0,
    InvalidInput           = 1,
    DimensionMismatch      = 2,
    SingularMatrix         = 3,
    NumericalInstability   = 4,
    MaxIterationsReached   = 5,
    Infeasible             = 6,
    Unbounded              = 7,
    OutOfMemory            = 8,
    NotImplemented         = 9,
    Error                  = -1
};

constexpr bool is_ok(Status status) noexcept {
    return status == Status::Ok;
}

std::string_view status_to_string(Status status) noexcept;

} // namespace sovereign

#endif // SOVEREIGN_CORE_STATUS_HPP
