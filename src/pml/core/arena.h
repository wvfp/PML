#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Arena Allocator — bump allocator for short-lived interpreter objects
// ═══════════════════════════════════════════════════════════════════════════════
//
// A simple bump allocator that hands out memory from pre-allocated blocks.
// Intended for objects whose lifetime is bounded by a single `execute()` call
// (AST nodes, temporary lists, etc.).  Deallocations are batched: the whole
// arena is reset/freed at once.
//
// Use `Arena::Allocator<T>` with `std::allocate_shared` or STL containers to
// route allocations through an arena.
//
// IMPORTANT: objects allocated from an arena must NOT outlive the arena.  The
// default shared_ptr deleter is replaced by a no-op when arena ownership is
// desired.
// ═══════════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Arena
// ═══════════════════════════════════════════════════════════════════════════════

class Arena {
public:
    explicit Arena(size_t block_size = 64 * 1024)
        : block_size_(block_size) {
        allocate_block();
    }

    ~Arena() = default;

    // Non-copyable, non-movable (addresses must remain stable within a call).
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    /// Allocate `size` bytes with the given alignment.
    [[nodiscard]] void* allocate(size_t size, size_t alignment) {
        if (size == 0) {
            return nullptr;
        }
        // Ensure we have a current block.
        if (blocks_.empty()) {
            allocate_block();
        }

        Block* block = blocks_.back().get();
        char* base = block->data.get();
        size_t aligned = align_up(block->offset, alignment);

        if (aligned + size > block->size) {
            // Not enough room: allocate a new block large enough for this request.
            size_t new_block_size = std::max(block_size_, aligned + size);
            allocate_block(new_block_size);
            block = blocks_.back().get();
            base = block->data.get();
            aligned = align_up(block->offset, alignment);
        }

        block->offset = aligned + size;
        return base + aligned;
    }

    /// Construct a single object of type T in arena memory.
    template<typename T, typename... Args>
    [[nodiscard]] T* construct(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    /// Destroy an object (does NOT reclaim memory).
    template<typename T>
    void destroy(T* obj) {
        if (obj) {
            obj->~T();
        }
    }

    /// Reset the arena, discarding all allocations.
    void reset() noexcept {
        blocks_.clear();
        allocate_block();
    }

    /// Total bytes currently reserved by the arena.
    [[nodiscard]] size_t total_reserved() const noexcept {
        size_t total = 0;
        for (const auto& block : blocks_) {
            total += block->size;
        }
        return total;
    }

    /// Bytes currently in use across all blocks.
    [[nodiscard]] size_t total_used() const noexcept {
        size_t total = 0;
        for (const auto& block : blocks_) {
            total += block->offset;
        }
        return total;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // STL-compatible allocator
    // ═══════════════════════════════════════════════════════════════════════════

    template<typename T>
    class Allocator {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using propagate_on_container_swap = std::false_type;
        using is_always_equal = std::false_type;

        /// Default constructor: falls back to the global heap.
        Allocator() noexcept
            : arena_(nullptr) {}

        explicit Allocator(Arena* arena) noexcept
            : arena_(arena) {}

        template<typename U>
        Allocator(const Allocator<U>& other) noexcept
            : arena_(other.arena_) {}

        [[nodiscard]] T* allocate(size_type n) {
            if (n == 0) {
                return nullptr;
            }
            if (arena_) {
                return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
            }
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void deallocate(T* ptr, size_type /*n*/) noexcept {
            if (!arena_) {
                ::operator delete(ptr);
            }
            // No-op for arena-owned memory: reclaimed when the arena resets.
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) {
            p->~U();
        }

        bool operator==(const Allocator& other) const noexcept {
            return arena_ == other.arena_;
        }

        bool operator!=(const Allocator& other) const noexcept {
            return !(*this == other);
        }

        Arena* arena() const noexcept { return arena_; }

    private:
        Arena* arena_;

        template<typename U>
        friend class Allocator;
    };

private:
    struct Block {
        std::unique_ptr<char[]> data;
        size_t size = 0;
        size_t offset = 0;
    };

    std::vector<std::unique_ptr<Block>> blocks_;
    size_t block_size_;

    void allocate_block(size_t size = 0) {
        size_t actual_size = size == 0 ? block_size_ : size;
        auto block = std::make_unique<Block>();
        block->data = std::make_unique<char[]>(actual_size);
        block->size = actual_size;
        block->offset = 0;
        blocks_.push_back(std::move(block));
    }

    [[nodiscard]] static size_t align_up(size_t value, size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

}  // namespace pml
