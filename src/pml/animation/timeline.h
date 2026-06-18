#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Animation Timeline — Animation struct, Timeline class, and builtins
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/animation/__init__.py + pml/animation/timeline.py.
// Provides numeric property tweening with easing, global timeline singleton,
// and PML builtins (_animate, _play, _stop, _pause, _seek).
// ═══════════════════════════════════════════════════════════════════════════════

#include "easing.h"
#include "objects.h"
#include "canvas.h"
#include "environment.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Animation — single property tween
// ═══════════════════════════════════════════════════════════════════════════════

/// A single property animation (tween) that interpolates a property
/// on a target GraphicObject from @p from_value to @p to_value over
/// @p duration seconds using an easing function.
/// Supports numeric properties and color strings (hex RGB/RGBA).
struct Animation {
    uint64_t target_id;           ///< GraphicObject::id to animate.
    std::string property_name;    ///< Property key ("x", "y", "r", "fill", etc.).
    Value from_value;             ///< Start value (number or color string).
    Value to_value;               ///< End value (number or color string).
    float duration;               ///< Duration of one cycle in seconds.
    EasingFn easing;              ///< Easing function (from easing.h).
    float delay{0.0f};            ///< Delay before the animation starts.
    int repeat{1};                ///< Number of cycles (1 = play once).

    Animation(
        uint64_t target_id_,
        std::string property_name_,
        Value from_value_,
        Value to_value_,
        float duration_,
        EasingFn easing_ = easing_linear,
        float delay_ = 0.0f,
        int repeat_ = 1
    );

    /// Total duration including delay and all repeats.
    [[nodiscard]] double get_duration() const noexcept {
        return static_cast<double>(delay) + static_cast<double>(duration) * repeat;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// TimelineState — playback state machine
// ═══════════════════════════════════════════════════════════════════════════════

/// Runtime playback state of the timeline.
/// Matches Python's { "idle", "playing", "paused", "stopped", "finished" }.
enum class TimelineState {
    Idle,
    Playing,
    Paused,
    Stopped,
    Finished
};

/// Convert TimelineState to string (for debugging / PML introspection).
std::string timeline_state_to_string(TimelineState s);

// ═══════════════════════════════════════════════════════════════════════════════
// Frame types
// ═══════════════════════════════════════════════════════════════════════════════

/// A per-frame callback invoked with the current time.
using FrameHook = std::function<void(double)>;

/// A single evaluation result: (target_id, property_name, interpolated_value).
/// The value is a runtime Value (number or color string).
using FrameResult = std::tuple<uint64_t, std::string, Value>;

// ═══════════════════════════════════════════════════════════════════════════════
// Timeline — global animation manager
// ═══════════════════════════════════════════════════════════════════════════════

/// Global animation timeline singleton.
///
/// Tracks registered animations, playback state, and per-frame hooks.
/// Accessed via the @p g_timeline global (lazily created on first use).
///
/// Usage:
///   - Create Animation objects via _animate() builtin
///   - Control playback with play()/stop()/pause()/seek()
///   - Call render_frames() to step through all frames
class Timeline {
public:
    /// Registered animations (reference semantics via shared_ptr).
    std::vector<std::shared_ptr<Animation>> animations;

    /// Per-frame hooks called before each evaluated frame.
    std::vector<FrameHook> frame_hooks;

    /// Current playback state.
    TimelineState state{TimelineState::Idle};

    /// Current playback time in seconds.
    double current_time{0.0};

    Timeline() = default;

    // ── Mutators ────────────────────────────────────────────────────────

    /// Register an animation on the timeline.
    void add(std::shared_ptr<Animation> anim);

    /// Register a function to call before each evaluated frame.
    /// The hook receives the current time (in seconds).
    void add_frame_hook(FrameHook hook);

    /// Start playback — sets state to Playing.
    void play();

    /// Stop playback and reset current_time to 0.
    void stop();

    /// Pause playback (only when state is Playing).
    void pause();

    /// Seek to a specific time (clamped to >= 0).
    void seek(double t);

    // ── Queries ─────────────────────────────────────────────────────────

    /// Total duration across all registered animations.
    /// Returns the maximum finite duration. Returns 0.0 if no animations.
    double get_total_duration() const;

    /// Evaluate all animations at time @p t.
    /// Returns a vector of (target_id, property_name, interpolated_value)
    /// tuples for every active animation at this time.
    [[nodiscard]] std::vector<FrameResult> evaluate_at(double t) const;

    // ── Frame rendering ─────────────────────────────────────────────────

    /// Step through all frames at @p fps, applying modifications to canvas.
    ///
    /// For each time step:
    ///   1. Calls all frame_hooks with the current time.
    ///   2. Evaluates all animations at that time.
    ///   3. Builds a (target_id → {property → value}) map.
    ///   4. Applies modifications to matching GraphicObjects on
    ///      g_current_canvas via _apply_modifications().
    ///
    /// After rendering, sets state to Finished and current_time to duration.
    void render_frames(int fps);

    /// Reset the global timeline singleton (for testing).
    static void reset();
};

// ═══════════════════════════════════════════════════════════════════════════════
// Global singleton
// ═══════════════════════════════════════════════════════════════════════════════

/// The global timeline instance.
/// Lazily created on first access (via _animate builtin or direct use).
extern std::shared_ptr<Timeline> g_timeline;

/// Ensure the global timeline exists (create if null).
/// Returns the current g_timeline.
std::shared_ptr<Timeline> get_or_create_timeline();

// ═══════════════════════════════════════════════════════════════════════════════
// _apply_modifications — apply animation frame to GraphicObject
// ═══════════════════════════════════════════════════════════════════════════════

/// Apply a set of property modifications to a GraphicObject.
///
/// Creates and returns a NEW GraphicObject with the modifications applied
/// (the original is unchanged — immutable pattern).
///
/// Supported properties:
///   - param keys (x, y, r, w, h, cx, cy, etc.) → stored in params map
///   - "fill"            → set fill color string
///   - "stroke"          → set stroke color string
///   - "stroke-width"    → set stroke_width field
///   - "transform.tx"    → modify transform.e (translation x)
///   - "transform.ty"    → modify transform.f (translation y)
GraphicObject _apply_modifications(
    const GraphicObject& obj,
    const std::unordered_map<std::string, Value>& mods);

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Register timeline-related builtins into the environment.
///
/// Registers:
///   (_animate target property from to duration [:ease <name>]) → Animation
///   (_play)                          → start playback
///   (_stop)                          → stop and reset
///   (_pause)                         → pause playback
///   (_seek t)                        → seek to time t
void register_timeline_builtins(std::shared_ptr<Environment> env);

} // namespace pml
