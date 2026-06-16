// ═══════════════════════════════════════════════════════════════════════════════
// PML Animation Timeline — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "timeline.h"

#include "error.h"
#include "types.h"
#include "evaluator.h"
#include "transform.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Global timeline singleton
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<Timeline> g_timeline = nullptr;

std::shared_ptr<Timeline> get_or_create_timeline() {
    if (!g_timeline) {
        g_timeline = std::make_shared<Timeline>();
    }
    return g_timeline;
}

void Timeline::reset() {
    g_timeline = nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TimelineState → string
// ═══════════════════════════════════════════════════════════════════════════════

std::string timeline_state_to_string(TimelineState s) {
    switch (s) {
        case TimelineState::Idle:     return "idle";
        case TimelineState::Playing:  return "playing";
        case TimelineState::Paused:   return "paused";
        case TimelineState::Finished: return "finished";
    }
    return "idle";  // unreachable
}

// ═══════════════════════════════════════════════════════════════════════════════
// Animation constructor
// ═══════════════════════════════════════════════════════════════════════════════

Animation::Animation(
    uint64_t target_id_,
    std::string property_name_,
    double from_value_,
    double to_value_,
    float duration_,
    EasingFn easing_)
    : target_id(target_id_)
    , property_name(std::move(property_name_))
    , from_value(from_value_)
    , to_value(to_value_)
    , duration(duration_)
    , easing(easing_ ? std::move(easing_) : easing_linear)
{
}

// ═══════════════════════════════════════════════════════════════════════════════
// Timeline — mutators
// ═══════════════════════════════════════════════════════════════════════════════

void Timeline::add(Animation anim) {
    animations.push_back(std::move(anim));
}

void Timeline::add_frame_hook(FrameHook hook) {
    frame_hooks.push_back(std::move(hook));
}

void Timeline::play() {
    state = TimelineState::Playing;
}

void Timeline::stop() {
    state = TimelineState::Finished;
    current_time = 0.0;
}

void Timeline::pause() {
    if (state == TimelineState::Playing) {
        state = TimelineState::Paused;
    }
}

void Timeline::seek(double t) {
    current_time = std::max(0.0, t);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Timeline — queries
// ═══════════════════════════════════════════════════════════════════════════════

double Timeline::get_total_duration() const {
    if (animations.empty()) return 0.0;
    double max_dur = 0.0;
    for (const auto& anim : animations) {
        if (anim.duration > max_dur) {
            max_dur = static_cast<double>(anim.duration);
        }
    }
    // Default to 1 second if only zero-duration animations
    if (max_dur <= 0.0) return 1.0;
    return max_dur;
}

std::vector<FrameResult> Timeline::evaluate_at(double t) const {
    std::vector<FrameResult> result;
    result.reserve(animations.size());

    for (const auto& anim : animations) {
        if (anim.duration <= 0.0f) continue;

        double progress = t / static_cast<double>(anim.duration);
        progress = std::clamp(progress, 0.0, 1.0);

        double eased = anim.easing(progress);
        double value = anim.from_value + (anim.to_value - anim.from_value) * eased;

        result.emplace_back(anim.target_id, anim.property_name, value);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Timeline — frame rendering
// ═══════════════════════════════════════════════════════════════════════════════

void Timeline::render_frames(int fps) {
    if (fps <= 0) fps = 30;  // sane default

    double total_duration = get_total_duration();
    if (total_duration <= 0.0) total_duration = 1.0;

    int num_frames = std::max(1, static_cast<int>(std::ceil(total_duration * fps)));

    for (int i = 0; i < num_frames; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(fps);

        // Call frame hooks with current time
        for (const auto& hook : frame_hooks) {
            hook(t);
        }

        // Evaluate all animations at time t
        auto modifications = evaluate_at(t);

        // Build mapping: target_id → {property → value}
        std::unordered_map<uint64_t, std::unordered_map<std::string, double>> obj_mods;
        for (const auto& [target_id, prop, value] : modifications) {
            obj_mods[target_id][prop] = value;
        }

        // Apply modifications to canvas objects
        if (g_current_canvas) {
            for (auto& obj : g_current_canvas->objects) {
                auto it = obj_mods.find(obj.id);
                if (it != obj_mods.end()) {
                    obj = _apply_modifications(obj, it->second);
                }
            }
        }
    }

    current_time = total_duration;
    state = TimelineState::Finished;
}

// ═══════════════════════════════════════════════════════════════════════════════
// _apply_modifications
// ═══════════════════════════════════════════════════════════════════════════════

GraphicObject _apply_modifications(
    const GraphicObject& obj,
    const std::unordered_map<std::string, double>& mods)
{
    auto new_params = obj.params;
    auto new_fill = obj.fill;
    auto new_stroke = obj.stroke;
    double new_stroke_width = obj.stroke_width;
    auto new_transform = obj.transform;

    for (const auto& [prop, value] : mods) {
        if (prop == "fill") {
            // For numeric tweening, store the interpolated value as a param;
            // actual fill animation would need string interpolation (future).
            new_params[prop] = Value(value);
        } else if (prop == "stroke") {
            new_params[prop] = Value(value);
        } else if (prop == "stroke-width") {
            new_stroke_width = value;
        } else if (prop == "transform.tx") {
            new_transform = ::AffineTransform{
                new_transform.a, new_transform.b,
                new_transform.c, new_transform.d,
                value, new_transform.f
            };
        } else if (prop == "transform.ty") {
            new_transform = ::AffineTransform{
                new_transform.a, new_transform.b,
                new_transform.c, new_transform.d,
                new_transform.e, value
            };
        } else {
            // Assume it's a param key (x, y, r, w, h, cx, cy, etc.)
            new_params[prop] = Value(static_cast<int64_t>(value));
        }
    }

    return GraphicObject(
        obj.shape_type,
        std::move(new_params),
        std::move(new_fill),
        std::move(new_stroke),
        new_stroke_width,
        new_transform,
        obj.children,
        obj.metadata
    );
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin implementations (anonymous namespace)
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

/// Extract keyword arguments from the tail of the args vector.
///
/// The evaluator's apply_function merges keyword args as a flat sequence
/// appended to the positional args:
///   [pos1, pos2, ..., Keyword("key"), val1, Keyword("key2"), val2, ...]
///
/// This helper walks from the end, extracting Keyword+Value pairs into
/// a map and popping them from the vector.
std::unordered_map<std::string, Value> extract_kwargs(std::vector<Value>& args) {
    std::unordered_map<std::string, Value> kwargs;

    while (args.size() >= 2) {
        const Value& last = args.back();
        const auto* kw = std::get_if<Keyword>(&last);
        if (!kw) break;  // No more keyword args

        kwargs[kw->name] = args[args.size() - 2];
        args.pop_back();  // remove value
        args.pop_back();  // remove keyword
    }

    // Reverse the remaining positional args (they were reversed by pop_back)
    // Actually, since we popped from the end of the original merged list,
    // the positional args at the front are still in the right order.
    // No reversal needed.

    return kwargs;
}

/// _animate builtin: (_animate target property from to duration [:ease <name>])
///
/// Creates an Animation and registers it on the global timeline.
/// Keyword args:
///   :ease <symbol|string> — easing function name (default "linear")
///
/// Returns the Animation object as a Value.
Result<Value> builtin_animate(const std::vector<Value>& cxx_args, Environment&) {
    // Copy since we need to mutate (extract kwargs)
    std::vector<Value> args = cxx_args;

    // Extract keyword arguments from tail
    auto kwargs = extract_kwargs(args);

    // Need at least 5 positional: target, property, from, to, duration
    if (args.size() < 5) {
        return std::unexpected(arity_error(
            SourceLocation{}, 5, static_cast<int>(args.size())));
    }

    // Target must be a GraphicObject
    const auto* target_ptr = std::get_if<std::shared_ptr<GraphicObject>>(&args[0]);
    if (!target_ptr || !*target_ptr) {
        return std::unexpected(type_error(
            "_animate: first argument must be a GraphicObject"));
    }
    uint64_t target_id = (*target_ptr)->id;

    // Property name (from Symbol or string)
    std::string prop_name;
    if (const auto* sym = std::get_if<Symbol>(&args[1])) {
        prop_name = sym->name;
    } else if (const auto* s = std::get_if<std::string>(&args[1])) {
        prop_name = *s;
    } else {
        return std::unexpected(type_error(
            "_animate: property must be a symbol or string"));
    }

    // From value
    if (!is_number(args[2])) {
        return std::unexpected(type_error(
            "_animate: from value must be numeric"));
    }
    double from_val = (std::holds_alternative<double>(args[2]))
        ? std::get<double>(args[2])
        : static_cast<double>(std::get<int64_t>(args[2]));

    // To value
    if (!is_number(args[3])) {
        return std::unexpected(type_error(
            "_animate: to value must be numeric"));
    }
    double to_val = (std::holds_alternative<double>(args[3]))
        ? std::get<double>(args[3])
        : static_cast<double>(std::get<int64_t>(args[3]));

    // Duration
    if (!is_number(args[4])) {
        return std::unexpected(type_error(
            "_animate: duration must be numeric"));
    }
    float duration = static_cast<float>(
        (std::holds_alternative<double>(args[4]))
            ? std::get<double>(args[4])
            : static_cast<double>(std::get<int64_t>(args[4])));

    // Easing (from :ease keyword or default "linear")
    std::string ease_name = "linear";
    auto ease_it = kwargs.find("ease");
    if (ease_it != kwargs.end()) {
        if (const auto* sym = std::get_if<Symbol>(&ease_it->second)) {
            ease_name = sym->name;
        } else if (const auto* s = std::get_if<std::string>(&ease_it->second)) {
            ease_name = *s;
        }
    }

    EasingFn ease_fn = get_easing(ease_name);

    // Create animation
    auto anim = std::make_shared<Animation>(
        target_id, prop_name, from_val, to_val, duration, ease_fn);

    // Register on global timeline
    auto timeline = get_or_create_timeline();
    timeline->add(*anim);

    return Value(std::move(anim));
}

/// _play builtin: (_play) — start playback
Result<Value> builtin_play(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->play();
    return make_nil_value();
}

/// _stop builtin: (_stop) — stop and reset
Result<Value> builtin_stop(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->stop();
    return make_nil_value();
}

/// _pause builtin: (_pause) — pause playback
Result<Value> builtin_pause(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->pause();
    return make_nil_value();
}

/// _seek builtin: (_seek t) — seek to time t
Result<Value> builtin_seek(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(
            SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    if (!is_number(args[0])) {
        return std::unexpected(type_error(
            "_seek: argument must be numeric"));
    }
    double t = (std::holds_alternative<double>(args[0]))
        ? std::get<double>(args[0])
        : static_cast<double>(std::get<int64_t>(args[0]));

    auto timeline = get_or_create_timeline();
    timeline->seek(t);
    return make_nil_value();
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// register_timeline_builtins
// ═══════════════════════════════════════════════════════════════════════════════

void register_timeline_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn,
                   bool accepts_kwargs = false) {
        auto proc = std::make_shared<BuiltinProcedure>(
            name, std::move(fn), accepts_kwargs);
        env->define(name, Value(proc));
    };

    def("_animate", builtin_animate, true);   // accepts :ease keyword
    def("_play",    builtin_play);
    def("_stop",    builtin_stop);
    def("_pause",   builtin_pause);
    def("_seek",    builtin_seek);
}

}  // namespace pml
