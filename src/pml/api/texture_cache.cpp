// ═══════════════════════════════════════════════════════════════════════════════
// PML Texture Cache — LRU singleton for baked SkImages
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/texture_cache.h"

#include "pml/api/context.h"

#include "include/core/SkImage.h"

#include <algorithm>
#include <cstdint>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Singleton accessor (delegates to PMLContext)
// ═══════════════════════════════════════════════════════════════════════════════

TextureCache& TextureCache::instance() {
    auto& ctx = PMLContext::current();
    if (!ctx.texture_cache) {
        ctx.texture_cache = std::make_unique<TextureCache>();
    }
    return *ctx.texture_cache;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Lookup
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<SkImage> TextureCache::lookup(size_t stable_id) const {
    auto it = m_entries.find(stable_id);
    if (it == m_entries.end()) {
        return nullptr;
    }
    // Update LRU ordering — const-cast because lookup is logically const
    // but updates internal bookkeeping.
    const_cast<TextureCache*>(this)->touch(stable_id);
    return it->second.image;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Insert
// ═══════════════════════════════════════════════════════════════════════════════

void TextureCache::insert(size_t stable_id, std::shared_ptr<SkImage> image) {
    if (!image) {
        return;
    }

    // Approximate byte size: width × height × 4 bytes/pixel (RGBA).
    const auto dims = image->dimensions();
    const size_t byte_size =
        static_cast<size_t>(dims.width()) * static_cast<size_t>(dims.height()) * 4;

    // If the new image alone exceeds the budget, don't cache it at all.
    if (byte_size > m_max_bytes) {
        return;
    }

    // If the entry already exists, subtract its old size first.
    auto it = m_entries.find(stable_id);
    if (it != m_entries.end()) {
        m_current_bytes -= it->second.byte_size;
        // Remove stale LRU position — will re-insert below.
        m_lru_order.remove(stable_id);
    }

    // Evict until we have room for the new entry.
    m_current_bytes += byte_size;
    while (m_current_bytes > m_max_bytes) {
        evict_lru();
    }

    // Store / update entry.
    m_entries[stable_id] = CacheEntry{
        .image = std::move(image),
        .byte_size = byte_size,
        .last_access = Clock::now(),
    };
    // New (or refreshed) entry goes to the back = most recently used.
    m_lru_order.push_back(stable_id);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Eviction
// ═══════════════════════════════════════════════════════════════════════════════

void TextureCache::evict(size_t stable_id) {
    auto it = m_entries.find(stable_id);
    if (it == m_entries.end()) {
        return;
    }
    m_current_bytes -= it->second.byte_size;
    m_entries.erase(it);
    m_lru_order.remove(stable_id);
}

void TextureCache::evict_lru() {
    // Evict the oldest entry (front of the list) until under budget.
    while (m_current_bytes > m_max_bytes && !m_lru_order.empty()) {
        size_t oldest_id = m_lru_order.front();
        m_lru_order.pop_front();
        auto it = m_entries.find(oldest_id);
        if (it != m_entries.end()) {
            m_current_bytes -= it->second.byte_size;
            m_entries.erase(it);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Clear & reset
// ═══════════════════════════════════════════════════════════════════════════════

void TextureCache::clear() {
    m_entries.clear();
    m_lru_order.clear();
    m_current_bytes = 0;
}

void TextureCache::reset() {
    clear();
    m_max_bytes = kDefaultMaxBytes;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════════════════════════

void TextureCache::set_max_size(size_t max_bytes) {
    m_max_bytes = max_bytes;
    // Evict immediately if the new limit is smaller than current usage.
    evict_lru();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Internal — touch (promote to most recently used)
// ═══════════════════════════════════════════════════════════════════════════════

void TextureCache::touch(size_t stable_id) {
    auto it = m_entries.find(stable_id);
    if (it == m_entries.end()) {
        return;
    }
    it->second.last_access = Clock::now();
    // Move to back (most recently used).
    m_lru_order.remove(stable_id);
    m_lru_order.push_back(stable_id);
}

} // namespace pml
