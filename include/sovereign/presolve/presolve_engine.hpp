#ifndef SOVEREIGN_PRESOLVE_PRESOLVE_ENGINE_HPP
#define SOVEREIGN_PRESOLVE_PRESOLVE_ENGINE_HPP

#include "sovereign/presolve/presolve_types.hpp"
#include "sovereign/presolve/presolve_stack.hpp"
#include "sovereign/lp/lp_model.hpp"

namespace sovereign::presolve {

struct PresolveResult {
    PresolveStatus status{PresolveStatus::Clean};
    lp::LPModel presolved_model;
    PresolveStack stack;
    Int32 passes_executed{0};
    Int64 total_reductions{0};
    std::string message;
};

class PresolveEngine {
private:
    PresolveConfig config_;

public:
    PresolveEngine() = default;
    explicit PresolveEngine(const PresolveConfig& config) : config_(config) {}

    // Runs iterative presolve passes on orig_model and returns PresolveResult
    PresolveResult presolve(const lp::LPModel& orig_model) const;
};

} // namespace sovereign::presolve

#endif // SOVEREIGN_PRESOLVE_PRESOLVE_ENGINE_HPP
