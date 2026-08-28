#ifndef SOVEREIGN_LP_BASIS_HPP
#define SOVEREIGN_LP_BASIS_HPP

#include "sovereign/core/types.hpp"
#include <span>
#include <vector>

namespace sovereign::lp {

enum class BasisStatus : std::uint8_t {
    Basic   = 0,
    AtLower = 1,
    AtUpper = 2,
    Free    = 3,
    Fixed   = 4
};

class Basis {
private:
    Index num_rows_{0};
    Index num_cols_{0};

    std::vector<Index> basic_vars_;      // Size: num_rows_ (Maps basis slot i -> col j)
    std::vector<Index> nonbasic_vars_;   // Size: num_cols_ - num_rows_
    std::vector<BasisStatus> var_status_; // Size: num_cols_

    std::vector<Index> basic_pos_;       // Size: num_cols_ (Maps col j -> basis slot i, or -1)
    std::vector<Index> nonbasic_pos_;    // Size: num_cols_ (Maps col j -> nonbasic slot k, or -1)

public:
    Basis() = default;
    Basis(Index num_rows, Index num_cols);

    Index num_rows() const noexcept { return num_rows_; }
    Index num_cols() const noexcept { return num_cols_; }

    std::span<const Index> basic_vars() const noexcept { return basic_vars_; }
    std::span<const Index> nonbasic_vars() const noexcept { return nonbasic_vars_; }
    std::span<const BasisStatus> var_status() const noexcept { return var_status_; }

    BasisStatus get_status(Index col) const;
    Index get_basic_pos(Index col) const;
    Index get_basic_var(Index slot) const;

    void set_basic(Index col, Index slot);
    void set_nonbasic(Index col, BasisStatus status);

    // Pivot Step: Replaces variable at basis slot `out_slot` with entering variable `in_col`
    void pivot(Index entering_col, Index leaving_slot, BasisStatus leaving_status);

    bool is_valid() const noexcept;
    void rebuild_position_maps();
};

} // namespace sovereign::lp

#endif // SOVEREIGN_LP_BASIS_HPP
