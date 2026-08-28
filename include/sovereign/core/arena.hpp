#ifndef SOVEREIGN_CORE_ARENA_HPP
#define SOVEREIGN_CORE_ARENA_HPP

#include "sovereign/core/types.hpp"
#include <vector>
#include <cstddef>
#include <memory>
#include <concepts>

namespace sovereign {

class MemoryArena {
private:
    std::vector<std::byte> buffer_;
    std::size_t offset_{0};

public:
    explicit MemoryArena(std::size_t initial_capacity_bytes = 1024 * 1024);
    ~MemoryArena() = default;

    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    MemoryArena(MemoryArena&&) noexcept = default;
    MemoryArena& operator=(MemoryArena&&) noexcept = default;

    void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t));

    template <typename T>
    T* allocate_array(std::size_t count) {
        return static_cast<T*>(allocate(count * sizeof(T), alignof(T)));
    }

    void reset() noexcept;
    std::size_t capacity() const noexcept;
    std::size_t bytes_allocated() const noexcept;
};

} // namespace sovereign

#endif // SOVEREIGN_CORE_ARENA_HPP
