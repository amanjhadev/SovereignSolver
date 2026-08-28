#ifndef SOVEREIGN_CORE_VECTOR_OPS_HPP
#define SOVEREIGN_CORE_VECTOR_OPS_HPP

#include "sovereign/core/types.hpp"
#include <span>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace sovereign {

inline Float64 dot_product(std::span<const Float64> x, std::span<const Float64> y) {
    if (x.size() != y.size()) {
        throw std::invalid_argument("Vector dimension mismatch in dot_product");
    }
    Float64 sum = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sum += x[i] * y[i];
    }
    return sum;
}

inline void axpy(Float64 alpha, std::span<const Float64> x, std::span<Float64> y) {
    if (x.size() != y.size()) {
        throw std::invalid_argument("Vector dimension mismatch in axpy");
    }
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] += alpha * x[i];
    }
}

inline void scale_vector(Float64 alpha, std::span<Float64> x) {
    for (std::size_t i = 0; i < x.size(); ++i) {
        x[i] *= alpha;
    }
}

inline Float64 infinity_norm(std::span<const Float64> x) {
    Float64 max_val = 0.0;
    for (Float64 val : x) {
        max_val = std::max(max_val, std::abs(val));
    }
    return max_val;
}

inline Float64 l1_norm(std::span<const Float64> x) {
    Float64 sum = 0.0;
    for (Float64 val : x) {
        sum += std::abs(val);
    }
    return sum;
}

inline Float64 l2_norm(std::span<const Float64> x) {
    Float64 sum = 0.0;
    for (Float64 val : x) {
        sum += val * val;
    }
    return std::sqrt(sum);
}

inline void fill_vector(std::span<Float64> x, Float64 value) {
    std::fill(x.begin(), x.end(), value);
}

inline void copy_vector(std::span<const Float64> src, std::span<Float64> dst) {
    if (src.size() != dst.size()) {
        throw std::invalid_argument("Vector dimension mismatch in copy_vector");
    }
    std::copy(src.begin(), src.end(), dst.begin());
}

} // namespace sovereign

#endif // SOVEREIGN_CORE_VECTOR_OPS_HPP
