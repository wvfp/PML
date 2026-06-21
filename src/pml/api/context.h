#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Runtime Context — Aggregates per-runtime mutable global state
// ───────────────────────────────────────────────────────────────────────────────
// Replaces the loose global singletons (_current_canvas, g_timeline,
// StyleRegistry::instance(), PaletteManager::instance()) with an explicit
// context object owned by PMLRuntime.
//
// Design notes:
//   * Each PMLRuntime instance owns one PMLContext.
//   * A thread-local pointer selects the "current" context; singleton-style
//     accessors in graphics/animation/sprites delegate to PMLContext::current().
//   * A default static context is used when no runtime has been activated,
//     preserving backward compatibility for code that uses the accessors outside
//     of a PMLRuntime::execute() call.
// ═══════════════════════════════════════════════════════════════════════════════

#include <memory>
#include <string>
#include <vector>

namespace pml {

// Forward declarations keep the header lightweight and avoid circular
// include dependencies between core/graphics/animation/sprites.
class AssetCache;
class Canvas;
class Composition;
class Timeline;
class StyleRegistry;
class PaletteManager;
class TilemapManager;
class TilesetManager;

/// Runtime context that holds all per-instance mutable global state.
class PMLContext {
  public:
    PMLContext();
    ~PMLContext();

    PMLContext(const PMLContext&) = delete;
    PMLContext& operator=(const PMLContext&) = delete;

    // ── Subsystem state ─────────────────────────────────────────────────

    std::shared_ptr<Canvas> current_canvas;   ///< Active canvas set by (canvas ...)
    std::shared_ptr<Timeline> timeline;       ///< Active animation timeline
    std::unique_ptr<StyleRegistry> styles;    ///< Named style registry
    std::unique_ptr<PaletteManager> palettes; ///< Named palette registry + active palette
    std::unique_ptr<TilemapManager> tilemaps; ///< Named tilemap registry
    std::unique_ptr<TilesetManager> tilesets; ///< Named tileset registry
    std::unique_ptr<AssetCache> assets;       ///< Loaded image asset cache

    std::vector<std::shared_ptr<Composition>> compositions; ///< Registered compositions
    std::vector<std::string> output_files;                  ///< Paths written by this runtime

    // ── Lifecycle ───────────────────────────────────────────────────────

    /// Reset all state to a fresh runtime state (predefined styles/palettes
    /// are re-registered).
    void reset();

    /// Read-only access to registered compositions.
    [[nodiscard]] const std::vector<std::shared_ptr<Composition>>& registered_compositions() const noexcept {
        return compositions;
    }

    /// Read-only access to output file paths.
    [[nodiscard]] const std::vector<std::string>& output_paths() const noexcept {
        return output_files;
    }

    // ── Current-context access ──────────────────────────────────────────

    /// Return the context currently active on this thread. If none has been
    /// activated, returns a process-wide default context.
    [[nodiscard]] static PMLContext& current();

    /// Return the currently active context pointer, or nullptr if none.
    [[nodiscard]] static PMLContext* current_ptr() noexcept;

    /// Activate @p ctx for the current thread. Pass nullptr to deactivate.
    static void set_current(PMLContext* ctx) noexcept;

  private:
    static thread_local PMLContext* s_current;
};

/// RAII helper that activates a PMLContext for the current scope and restores
/// the previous context on destruction.
class PMLContextScope {
  public:
    explicit PMLContextScope(PMLContext& ctx)
        : prev_(PMLContext::current_ptr()) {
        PMLContext::set_current(&ctx);
    }

    ~PMLContextScope() {
        PMLContext::set_current(prev_);
    }

    PMLContextScope(const PMLContextScope&) = delete;
    PMLContextScope& operator=(const PMLContextScope&) = delete;

  private:
    PMLContext* prev_;
};

} // namespace pml
