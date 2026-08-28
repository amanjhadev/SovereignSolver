#include "sovereign/lp/basis.hpp"
#include <stdexcept>
#include <algorithm>

namespace sovereign::lp {

Basis::Basis(Index num_rows, Index num_cols)
    : num_rows_(num_rows), num_cols_(num_cols) {
    if (num_rows_ < 0 || num_cols_ < 0 || num_rows_ > num_cols_) {
        throw std::invalid_argument("Invalid Basis dimensions");
    }

    basic_vars_.assign(num_rows_, -1);
    nonbasic_vars_.reserve(num_cols_ - num_rows_);
    var_status_.assign(num_cols_, BasisStatus::AtLower);

    basic_pos_.assign(num_cols_, -1);
    nonbasic_pos_.assign(num_cols_, -1);
}

BasisStatus Basis::get_status(Index col) const {
    if (col < 0 || col >= num_cols_) {
        throw std::out_of_range("Column index out of range in get_status");
    }
    return var_status_[col];
}

Index Basis::get_basic_pos(Index col) const {
    if (col < 0 || col >= num_cols_) {
        throw std::out_of_range("Column index out of range in get_basic_pos");
    }
    return basic_pos_[col];
}

Index Basis::get_basic_var(Index slot) const {
    if (slot < 0 || slot >= num_rows_) {
        throw std::out_of_range("Basis slot out of range in get_basic_var");
    }
    return basic_vars_[slot];
}

void Basis::set_basic(Index col, Index slot) {
    if (col < 0 || col >= num_cols_ || slot < 0 || slot >= num_rows_) {
        throw std::out_of_range("Index out of range in set_basic");
    }
    basic_vars_[slot] = col;
    var_status_[col] = BasisStatus::Basic;
    basic_pos_[col] = slot;
    nonbasic_pos_[col] = -1;
}

void Basis::set_nonbasic(Index col, BasisStatus status) {
    if (col < 0 || col >= num_cols_) {
        throw std::out_of_range("Column index out of range in set_nonbasic");
    }
    if (status == BasisStatus::Basic) {
        throw std::invalid_argument("Cannot set nonbasic variable to Basic status");
    }
    var_status_[col] = status;
    basic_pos_[col] = -1;
}

void Basis::pivot(Index entering_col, Index leaving_slot, BasisStatus leaving_status) {
    if (entering_col < 0 || entering_col >= num_cols_ || leaving_slot < 0 || leaving_slot >= num_rows_) {
        throw std::out_of_range("Index out of range in pivot");
    }

    Index leaving_col = basic_vars_[leaving_slot];

    // Update leaving variable to non-basic
    var_status_[leaving_col] = leaving_status;
    basic_pos_[leaving_col] = -1;

    // Update entering variable to basic at leaving_slot
    basic_vars_[leaving_slot] = entering_col;
    var_status_[entering_col] = BasisStatus::Basic;
    basic_pos_[entering_col] = leaving_slot;

    rebuild_position_maps();
}

void Basis::rebuild_position_maps() {
    std::fill(basic_pos_.begin(), basic_pos_.end(), -1);
    std::fill(nonbasic_pos_.begin(), nonbasic_pos_.end(), -1);
    nonbasic_vars_.clear();

    for (Index i = 0; i < num_rows_; ++i) {
        Index col = basic_vars_[i];
        if (col >= 0 && col < num_cols_) {
            basic_pos_[col] = i;
        }
    }

    for (Index j = 0; j < num_cols_; ++j) {
        if (var_status_[j] != BasisStatus::Basic) {
            nonbasic_pos_[j] = static_cast<Index>(nonbasic_vars_.size());
            nonbasic_vars_.push_back(j);
        }
    }
}

bool Basis::is_valid() const noexcept {
    if (num_rows_ < 0 || num_cols_ < 0 || num_rows_ > num_cols_) return false;
    if (static_cast<Index>(basic_vars_.size()) != num_rows_) return false;
    if (static_cast<Index>(var_status_.size()) != num_cols_) return false;

    Index basic_count = 0;
    std::vector<bool> seen(num_cols_, false);

    for (Index i = 0; i < num_rows_; ++i) {
        Index col = basic_vars_[i];
        if (col < 0 || col >= num_cols_) return false;
        if (seen[col]) return false; // Duplicate basic variable
        if (var_status_[col] != BasisStatus::Basic) return false;
        seen[col] = true;
        basic_count++;
    }

    if (basic_count != num_rows_) return false;

    for (Index j = 0; j < num_cols_; ++j) {
        if (var_status_[j] == BasisStatus::Basic && !seen[j]) return false;
    }

    return true;
}

} // namespace sovereign::lp
