#ifndef SOVEREIGN_PRESOLVE_PRESOLVE_STACK_HPP
#define SOVEREIGN_PRESOLVE_PRESOLVE_STACK_HPP

#include "sovereign/presolve/presolve_types.hpp"
#include <vector>

namespace sovereign::presolve {

class PresolveStack {
private:
    std::vector<PresolveReduction> reductions_;

public:
    PresolveStack() = default;

    void push_reduction(PresolveReduction reduction) {
        reductions_.push_back(std::move(reduction));
    }

    bool empty() const noexcept {
        return reductions_.empty();
    }

    std::size_t size() const noexcept {
        return reductions_.size();
    }

    const std::vector<PresolveReduction>& reductions() const noexcept {
        return reductions_;
    }

    void clear() noexcept {
        reductions_.clear();
    }
};

} // namespace sovereign::presolve

#endif // SOVEREIGN_PRESOLVE_PRESOLVE_STACK_HPP
