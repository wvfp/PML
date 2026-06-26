#pragma once

// ==========================================================================================================================================================================================================================================═
// BackendRegistry — compile-time registration, runtime switching
// ==========================================================================================================================================================================================================================================═
//
// Backends register themselves at static init time via
// `BackendRegistry::register_backend()`.  At runtime, `set_active("name")`
// switches the current backend; `active()` returns it.
//
// The NullBackend is registered first so that `active()` always succeeds.
// ==========================================================================================================================================================================================================================================═

#include "backend.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pml {

/// Factory function type: creates a new backend instance on demand.
using BackendFactory = std::function<std::unique_ptr<RenderBackend>()>;

/// Singleton registry for render backends.
///
/// Backends are registered by name at static initialization time using the
/// `register_backend()` static method.  At runtime, call `set_active()` to
/// switch the active backend, or `create(name)` to get an instance without
/// activating it.
class BackendRegistry {
public:
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;

    // ---- Singleton access --------------------------------------------------------------------------------------------─

    /// Get the singleton instance.
    static auto instance() -> BackendRegistry&;

    // ---- Registration ----------------------------------------------------------------------------------------------------─

    /// Register a backend factory under the given name.
    void add(std::string name, std::string description,
             BackendCap capabilities, BackendFactory factory);

    /// Convenience: register a backend with static initialization.
    /// Returns true (ignored — intended for `[[maybe_unused]]` static vars).
    [[nodiscard]] static bool register_backend(
        std::string name, std::string description,
        BackendCap capabilities, BackendFactory factory);

    // ---- Instance creation --------------------------------------------------------------------------------------------─

    /// Create a backend instance by name. Returns nullptr if unknown.
    [[nodiscard]] auto create(const std::string& name)
        -> std::unique_ptr<RenderBackend>;

    /// Create the "best" available backend (highest capabilities bitmask).
    [[nodiscard]] auto create_best()
        -> std::unique_ptr<RenderBackend>;

    // ---- Queries ----------------------------------------------------------------------------------------------------------------─

    /// Return metadata for all registered backends.
    [[nodiscard]] auto available() const -> std::vector<BackendInfo>;

    // ---- Active backend management ----------------------------------------------------------------------------─

    /// Set the active backend by name. Returns false if name is unknown.
    [[nodiscard]] bool set_active(const std::string& name);

    /// Return a reference to the active backend.
    /// Creates the best available backend if none has been set yet.
    [[nodiscard]] auto active() -> RenderBackend&;

private:
    BackendRegistry() = default;

    struct BackendEntry {
        std::string name;
        std::string description;
        BackendCap capabilities{static_cast<BackendCap>(0)};
        BackendFactory factory;
    };

    std::vector<BackendEntry> backends_;
    std::unique_ptr<RenderBackend> active_instance_;
    RenderBackend* active_{nullptr};
};

}  // namespace pml
