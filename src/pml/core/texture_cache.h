#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Texture Cache — LRU singleton for baked SkImages
// ==========================================================================================================================================================================================================================================═

#include <chrono>
#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>

// Forward declaration of Skia types — these are real C++ classes in the
// global namespace (see include/core/SkImage.h), NOT pml::SkImage.
class SkImage;

namespace pml {

/// Per-runtime LRU cache for baked SkImages keyed by texture stable_id.
///
/// Thread-safety is not required (PML is single-threaded).
///
/// Singleton accessor delegates to PMLContext::current() so the cache is
/// scoped per PMLRuntime instance, matching the pattern used by StyleRegistry,
/// PaletteManager, etc.
class TextureCache {
  public:
    /// Global singleton accessor (lazily created via PMLContext).
    [[nodiscard]] static TextureCache& instance();

    /// Look up a cached SkImage by its stable_id.
    /// Returns nullptr if not found.
    [[nodiscard]] std::shared_ptr<SkImage> lookup(size_t stable_id) const;

    /// Insert or update a baked SkImage keyed by stable_id.
    /// May trigger LRU eviction if the cache exceeds max_bytes.
    void insert(size_t stable_id, std::shared_ptr<SkImage> image);

    /// Explicitly remove a single entry.
    void evict(size_t stable_id);

    /// Remove all entries and reset byte count.
    void clear();

    /// Set the maximum cache budget in bytes (default: 64 MB).
    void set_max_size(size_t max_bytes);

    /// Return the current approximate total byte usage.
    [[nodiscard]] size_t current_bytes() const noexcept { return m_current_bytes; }

    /// Return the number of cached entries.
    [[nodiscard]] size_t size() const noexcept { return m_entries.size(); }

    /// Full reset — clears all entries and resets max to the default.
    /// Intended for test isolation between smoke-test cases.
    void reset();

    /// Default maximum cache size (64 MiB).
    static constexpr size_t kDefaultMaxBytes = 64ULL * 1024 * 1024;

  public:
    TextureCache() = default;
    ~TextureCache() = default;

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

private:

    /// One cached texture entry.
    struct CacheEntry {
        std::shared_ptr<SkImage> image; ///< The baked SkImage.
        size_t byte_size = 0;           ///< Approximate size (w × h × 4).
        std::chrono::steady_clock::time_point last_access; ///< LRU timestamp.
    };

    using Clock = std::chrono::steady_clock;

    /// Evict least-recently-used entries until total bytes <= m_max_bytes.
    void evict_lru();

    /// Move an entry's last_access to now (called by lookup/insert).
    void touch(size_t stable_id);

    // ---- Data ------------------------------------------------------------------------------------------------------------------------

    size_t m_max_bytes{kDefaultMaxBytes};
    size_t m_current_bytes{0};

    // Stable-id → entry
    std::unordered_map<size_t, CacheEntry> m_entries;

    // LRU ordering: order of insertion/access, front = oldest.
    mutable std::list<size_t> m_lru_order;
};

} // namespace pml
