#include "sovereign/core/status.hpp"

namespace sovereign {

std::string_view status_to_string(Status status) noexcept {
    switch (status) {
        case Status::Ok:                   return "Ok";
        case Status::InvalidInput:         return "InvalidInput";
        case Status::DimensionMismatch:    return "DimensionMismatch";
        case Status::SingularMatrix:       return "SingularMatrix";
        case Status::NumericalInstability: return "NumericalInstability";
        case Status::MaxIterationsReached: return "MaxIterationsReached";
        case Status::Infeasible:           return "Infeasible";
        case Status::Unbounded:            return "Unbounded";
        case Status::OutOfMemory:          return "OutOfMemory";
        case Status::NotImplemented:       return "NotImplemented";
        case Status::Error:                return "Error";
        default:                           return "UnknownError";
    }
}

} // namespace sovereign
