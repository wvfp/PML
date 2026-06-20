#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// GraphicObject params — compact enum-keyed storage
// ═══════════════════════════════════════════════════════════════════════════════
// Replaces the previous std::unordered_map<std::string, Value> with a dense
// array indexed by ParamKey.  This removes per-object hash-table overhead and
// avoids string hashing on every render lookup.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/types.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace pml {

/// Known shape parameter keys.  New primitive parameters can be added here;
/// older renders simply won't use unknown slots.
enum class ParamKey : uint8_t {
    unknown,
    x, y, w, h,
    cx, cy, r,
    rx, ry,
    x1, y1, x2, y2,
    text,
    font_size,
    d,
    src,
    points,
    shader,
    crop,
    mesh,
    transform,
    count
};

/// Convert a string key to its ParamKey.  Unknown strings map to ParamKey::unknown.
[[nodiscard]] ParamKey param_key_from_string(std::string_view sv) noexcept;

/// Convert a ParamKey back to its canonical string.  Returns "" for unknown.
[[nodiscard]] const char* param_key_to_string(ParamKey key) noexcept;

/// Compact, array-backed parameter map for GraphicObject.
/// Provides map-like mutation/lookup while storing values in a fixed-size array.
class Params {
public:
    using Mask = uint32_t;
    static_assert(static_cast<size_t>(ParamKey::count) <= sizeof(Mask) * 8,
                  "Params mask is too small for the number of keys");

    Params() = default;

    /// Construct from string-keyed initializer list.  Keeps call sites readable.
    Params(std::initializer_list<std::pair<const char*, Value>> init);
    /// Construct from enum-keyed initializer list (preferred, zero ambiguity).
    Params(std::initializer_list<std::pair<ParamKey, Value>> init);

    /// Set a value by enum key.
    void set(ParamKey key, Value value);
    /// Set a value by string key (for compatibility / runtime strings).
    void set(const std::string& key, Value value);

    /// Check presence without retrieving.
    [[nodiscard]] bool contains(ParamKey key) const noexcept;
    [[nodiscard]] bool contains(const std::string& key) const noexcept;

    /// Retrieve a value if present.
    [[nodiscard]] const Value* find(ParamKey key) const noexcept;
    [[nodiscard]] const Value* find(const std::string& key) const noexcept;

    /// Map-like accessors.  Insert a new entry if absent.
    Value& operator[](ParamKey key);
    Value& operator[](const std::string& key);

    /// Number of stored parameters.
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    /// Iterator over present (key, value) pairs.  Order follows ParamKey enum.
    class iterator;
    [[nodiscard]] iterator begin() const noexcept;
    [[nodiscard]] iterator end() const noexcept;

    /// Equality is used by tests and debugging output.
    [[nodiscard]] bool operator==(const Params& other) const noexcept;
    [[nodiscard]] bool operator!=(const Params& other) const noexcept = default;

private:
    static constexpr size_t N = static_cast<size_t>(ParamKey::count);
    std::array<Value, N> values_{};
    Mask present_ = 0;

    [[nodiscard]] static size_t index(ParamKey key) noexcept {
        auto i = static_cast<size_t>(key);
        return i < N ? i : 0;
    }
    [[nodiscard]] bool bit(ParamKey key) const noexcept {
        auto i = static_cast<size_t>(key);
        return i < N ? ((present_ >> i) & 1) : false;
    }
    void set_bit(ParamKey key) noexcept {
        auto i = static_cast<size_t>(key);
        if (i < N) present_ |= (Mask{1} << i);
    }
};

class Params::iterator {
public:
    using value_type = std::pair<ParamKey, const Value&>;

    iterator(const Params* p, size_t i) : params_(p), idx_(i) {
        advance_to_present();
    }

    iterator& operator++() {
        ++idx_;
        advance_to_present();
        return *this;
    }

    [[nodiscard]] value_type operator*() const {
        return {static_cast<ParamKey>(idx_), params_->values_[idx_]};
    }

    [[nodiscard]] bool operator==(const iterator& other) const noexcept {
        return params_ == other.params_ && idx_ == other.idx_;
    }
    [[nodiscard]] bool operator!=(const iterator& other) const noexcept {
        return !(*this == other);
    }

private:
    const Params* params_;
    size_t idx_;

    void advance_to_present() {
        while (idx_ < N && !params_->bit(static_cast<ParamKey>(idx_))) {
            ++idx_;
        }
    }
};

inline Params::iterator Params::begin() const noexcept {
    return iterator(this, 0);
}
inline Params::iterator Params::end() const noexcept {
    return iterator(this, N);
}

}  // namespace pml
