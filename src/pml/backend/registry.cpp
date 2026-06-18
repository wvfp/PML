// ═══════════════════════════════════════════════════════════════════════════════
// BackendRegistry — Singleton Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/backend/registry.h"

#include <algorithm>
#include <cstdint>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Singleton
// ═══════════════════════════════════════════════════════════════════════════════

BackendRegistry& BackendRegistry::instance()
{
    static BackendRegistry reg;
    return reg;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void BackendRegistry::add(
    std::string name,
    std::string description,
    BackendCap capabilities,
    BackendFactory factory)
{
    // Avoid duplicates — replace existing entry with the same name
    for (auto& entry : backends_) {
        if (entry.name == name) {
            entry.description = std::move(description);
            entry.capabilities = capabilities;
            entry.factory = std::move(factory);
            return;
        }
    }

    backends_.push_back(BackendEntry{
        .name = std::move(name),
        .description = std::move(description),
        .capabilities = capabilities,
        .factory = std::move(factory),
    });
}

bool BackendRegistry::register_backend(
    std::string name,
    std::string description,
    BackendCap capabilities,
    BackendFactory factory)
{
    instance().add(std::move(name), std::move(description),
                   capabilities, std::move(factory));
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Instance creation
// ═══════════════════════════════════════════════════════════════════════════════

auto BackendRegistry::create(const std::string& name)
    -> std::unique_ptr<RenderBackend>
{
    for (const auto& entry : backends_) {
        if (entry.name == name) {
            return entry.factory();
        }
    }
    return nullptr;
}

auto BackendRegistry::create_best()
    -> std::unique_ptr<RenderBackend>
{
    if (backends_.empty()) {
        return nullptr;
    }

    // Pick the backend with the highest active capability bits
    const auto* best = &backends_[0];
    uint8_t best_score = static_cast<uint8_t>(best->capabilities);

    for (const auto& entry : backends_) {
        const uint8_t score = static_cast<uint8_t>(entry.capabilities);
        if (score > best_score) {
            best = &entry;
            best_score = score;
        }
    }

    return best->factory();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Queries
// ═══════════════════════════════════════════════════════════════════════════════

auto BackendRegistry::available() const -> std::vector<BackendInfo>
{
    std::vector<BackendInfo> result;
    result.reserve(backends_.size());
    for (const auto& entry : backends_) {
        result.push_back(BackendInfo{
            .name = entry.name,
            .description = entry.description,
            .capabilities = entry.capabilities,
        });
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Active backend
// ═══════════════════════════════════════════════════════════════════════════════

bool BackendRegistry::set_active(const std::string& name)
{
    for (const auto& entry : backends_) {
        if (entry.name == name) {
            active_instance_ = entry.factory();
            active_ = active_instance_.get();
            return true;
        }
    }
    return false;
}

auto BackendRegistry::active() -> RenderBackend&
{
    if (!active_) {
        // Lazily create the best available backend
        active_instance_ = create_best();
        active_ = active_instance_.get();
    }
    return *active_;
}

}  // namespace pml
