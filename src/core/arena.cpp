#include "sovereign/core/arena.hpp"
#include <new>

namespace sovereign {

MemoryArena::MemoryArena(std::size_t initial_capacity_bytes) {
    buffer_.resize(initial_capacity_bytes);
}

void* MemoryArena::allocate(std::size_t bytes, std::size_t alignment) {
    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(buffer_.data() + offset_);
    std::size_t padding = (alignment - (current_ptr % alignment)) % alignment;

    if (offset_ + padding + bytes > buffer_.size()) {
        // Expand buffer exponentially if needed
        std::size_t new_size = std::max(buffer_.size() * 2, offset_ + padding + bytes + 4096);
        buffer_.resize(new_size);
        current_ptr = reinterpret_cast<std::uintptr_t>(buffer_.data() + offset_);
        padding = (alignment - (current_ptr % alignment)) % alignment;
    }

    std::size_t alloc_offset = offset_ + padding;
    offset_ = alloc_offset + bytes;
    return static_cast<void*>(buffer_.data() + alloc_offset);
}

void MemoryArena::reset() noexcept {
    offset_ = 0;
}

std::size_t MemoryArena::capacity() const noexcept {
    return buffer_.size();
}

std::size_t MemoryArena::bytes_allocated() const noexcept {
    return offset_;
}

} // namespace sovereign
