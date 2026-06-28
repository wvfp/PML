// ==========================================================================================================================================================================================================================================═
// PML Animation Timeline — Implementation
// ==========================================================================================================================================================================================================================================═

#include "timeline.h"

#include "error.h"
#include "types.h"
#include "evaluator.h"
#include "transform.h"
#include "pml/skeleton/skin_binding.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

namespace {

// ==========================================================================================================================================================================================================================================═
// Local helpers
// ==========================================================================================================================================================================================================================================═

/// Convert a numeric Value to double.
[[nodiscard]] double value_to_double(const Value& v) {
    return v.to_double();
}

/// Parse a hex color string (#RGB, #RRGGBB, #RRGGBBAA) to 0xRRGGBB.
[[nodiscard]] std::optional<uint32_t> parse_hex_color(const std::string& s) {
    if (s.empty() || s[0] != '#')
        return std::nullopt;

    std::string hex = s.substr(1);
    if (hex.size() == 3) {
        // #RGB → #RRGGBB
        hex = std::string{hex[0], hex[0], hex[1], hex[1], hex[2], hex[2]};
    }
    if (hex.size() != 6 && hex.size() != 8) {
        return std::nullopt;
    }

    try {
        size_t pos = 0;
        unsigned long value = std::stoul(hex, &pos, 16);
        if (pos != hex.size())
            return std::nullopt;
        // Drop alpha if present; keep 24-bit RGB.
        return static_cast<uint32_t>(value >> (hex.size() == 8 ? 8 : 0)) & 0xFFFFFFu;
    } catch (...) {
        return std::nullopt;
    }
}

/// Format an RGB triplet as #RRGGBB.
[[nodiscard]] std::string format_hex_color(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("#{:02x}{:02x}{:02x}", r, g, b);
}

/// Interpolate between two RGB hex strings.
[[nodiscard]] Value interpolate_color(const std::string& from, const std::string& to, double t) {
    auto c1 = parse_hex_color(from);
    auto c2 = parse_hex_color(to);
    if (!c1 || !c2)
        return Value(from);

    uint8_t r1 = static_cast<uint8_t>((*c1 >> 16) & 0xFFu);
    uint8_t g1 = static_cast<uint8_t>((*c1 >> 8) & 0xFFu);
    uint8_t b1 = static_cast<uint8_t>(*c1 & 0xFFu);

    uint8_t r2 = static_cast<uint8_t>((*c2 >> 16) & 0xFFu);
    uint8_t g2 = static_cast<uint8_t>((*c2 >> 8) & 0xFFu);
    uint8_t b2 = static_cast<uint8_t>(*c2 & 0xFFu);

    auto lerp = [](uint8_t a, uint8_t b, double t) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(static_cast<int>(a + (b - a) * t + 0.5), 0, 255));
    };

    return Value(format_hex_color(lerp(r1, r2, t), lerp(g1, g2, t), lerp(b1, b2, t)));
}

} // namespace

// ==========================================================================================================================================================================================================================================═
// Context-scoped timeline
// ==========================================================================================================================================================================================================================================═

void Timeline::reset() {
    PMLContext::current().timeline.reset();
}

// ==========================================================================================================================================================================================================================================═
// TimelineState → string
// ==========================================================================================================================================================================================================================================═

std::string timeline_state_to_string(TimelineState s) {
    switch (s) {
    case TimelineState::Idle:
        return "idle";
    case TimelineState::Playing:
        return "playing";
    case TimelineState::Paused:
        return "paused";
    case TimelineState::Stopped:
        return "stopped";
    case TimelineState::Finished:
        return "finished";
    }
    return "idle"; // unreachable
}

// ==========================================================================================================================================================================================================================================═
// Animation constructor
// ==========================================================================================================================================================================================================================================═

Animation::Animation(uint64_t target_id_,
                     std::string property_name_,
                     Value from_value_,
                     Value to_value_,
                     float duration_,
                     EasingFn easing_,
                     float delay_,
                     int repeat_)
    : target_id(target_id_)
    , property_name(std::move(property_name_))
    , from_value(std::move(from_value_))
    , to_value(std::move(to_value_))
    , duration(duration_)
    , easing(easing_ ? std::move(easing_) : easing_linear)
    , delay(delay_)
    , repeat(repeat_ > 0 ? repeat_ : 1) {}

// ==========================================================================================================================================================================================================================================═
// Timeline — mutators
// ==========================================================================================================================================================================================================================================═

void Timeline::add(std::shared_ptr<Animation> anim) {
    if (anim) {
        animations.push_back(std::move(anim));
    }
}

void Timeline::add_frame_hook(FrameHook hook) {
    frame_hooks.push_back(std::move(hook));
}

void Timeline::play() {
    state = TimelineState::Playing;
}

void Timeline::stop() {
    state = TimelineState::Stopped;
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

// ==========================================================================================================================================================================================================================================═
// Timeline — queries
// ==========================================================================================================================================================================================================================================═

double Timeline::get_total_duration() const {
    if (animations.empty())
        return 0.0;
    double max_dur = 0.0;
    for (const auto& anim_ptr : animations) {
        if (!anim_ptr)
            continue;
        double dur = anim_ptr->get_duration();
        if (dur > max_dur) {
            max_dur = dur;
        }
    }
    // Default to 1 second if only zero-duration animations
    if (max_dur <= 0.0)
        return 1.0;
    return max_dur;
}

std::vector<FrameResult> Timeline::evaluate_at(double t) const {
    std::vector<FrameResult> result;
    result.reserve(animations.size());

    for (const auto& anim_ptr : animations) {
        if (!anim_ptr)
            continue;
        const auto& anim = *anim_ptr;
        if (anim.duration <= 0.0f)
            continue;

        double local_t = t - static_cast<double>(anim.delay);
        if (local_t < 0.0) {
            local_t = 0.0;
        }

        double cycle_duration = static_cast<double>(anim.duration) * anim.repeat;
        if (local_t >= cycle_duration) {
            local_t = cycle_duration;
        }

        double cycle_pos =
            (anim.repeat > 1) ? std::fmod(local_t, static_cast<double>(anim.duration)) : local_t;
        double progress = cycle_pos / static_cast<double>(anim.duration);
        progress = std::clamp(progress, 0.0, 1.0);

        double eased = anim.easing(progress);
        Value value;

        if (is_string(anim.from_value) && is_string(anim.to_value)) {
            // Interpolate color strings.
            value =
                interpolate_color(*anim.from_value.as_string(), *anim.to_value.as_string(), eased);
        } else if (is_number(anim.from_value) && is_number(anim.to_value)) {
            // Interpolate numeric values.
            double from = value_to_double(anim.from_value);
            double to = value_to_double(anim.to_value);
            value = Value(from + (to - from) * eased);
        } else {
            // Mismatched types: hold at start value.
            value = anim.from_value;
        }

        result.emplace_back(anim.target_id, anim.property_name, value);
    }

    return result;
}

// ==========================================================================================================================================================================================================================================═
// Timeline — frame rendering
// ==========================================================================================================================================================================================================================================═

void Timeline::render_frames(int fps) {
    if (fps <= 0)
        fps = 30; // sane default

    double total_duration = get_total_duration();
    if (total_duration <= 0.0)
        total_duration = 1.0;

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
        std::unordered_map<uint64_t, std::unordered_map<std::string, Value>> obj_mods;
        for (const auto& [target_id, prop, value] : modifications) {
            obj_mods[target_id][prop] = value;
        }

        // Merge skeleton skin bindings into obj_mods
        merge_skin_bindings(obj_mods);

        // Apply modifications to canvas objects
        auto current_canvas = get_current_canvas();
        if (current_canvas) {
            for (auto& obj : current_canvas->objects) {
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

// ==========================================================================================================================================================================================================================================═
// _apply_modifications
// ==========================================================================================================================================================================================================================================═

GraphicObject _apply_modifications(const GraphicObject& obj,
                                   const std::unordered_map<std::string, Value>& mods) {
    auto new_params = obj.params;
    auto new_fill = obj.fill;
    auto new_stroke = obj.stroke;
    double new_stroke_width = obj.stroke_width;
    auto new_transform = obj.transform;

    for (const auto& [prop, value] : mods) {
        if (prop == "fill") {
            if (is_string(value)) {
                new_fill = *value.as_string();
            } else {
                new_params[prop] = value;
            }
        } else if (prop == "stroke") {
            if (is_string(value)) {
                new_stroke = *value.as_string();
            } else {
                new_params[prop] = value;
            }
        } else if (prop == "stroke-width") {
            new_stroke_width = value_to_double(value);
        } else if (prop == "transform.tx") {
            new_transform = AffineTransform{new_transform.a,
                                            new_transform.b,
                                            new_transform.c,
                                            new_transform.d,
                                            value_to_double(value),
                                            new_transform.f};
        } else if (prop == "transform.ty") {
            new_transform = AffineTransform{new_transform.a,
                                            new_transform.b,
                                            new_transform.c,
                                            new_transform.d,
                                            new_transform.e,
                                            value_to_double(value)};
        } else if (prop == "transform.a") {
            new_transform = AffineTransform{value_to_double(value),
                                            new_transform.b,
                                            new_transform.c,
                                            new_transform.d,
                                            new_transform.e,
                                            new_transform.f};
        } else if (prop == "transform.b") {
            new_transform = AffineTransform{new_transform.a,
                                            value_to_double(value),
                                            new_transform.c,
                                            new_transform.d,
                                            new_transform.e,
                                            new_transform.f};
        } else if (prop == "transform.c") {
            new_transform = AffineTransform{new_transform.a,
                                            new_transform.b,
                                            value_to_double(value),
                                            new_transform.d,
                                            new_transform.e,
                                            new_transform.f};
        } else if (prop == "transform.d") {
            new_transform = AffineTransform{new_transform.a,
                                            new_transform.b,
                                            new_transform.c,
                                            value_to_double(value),
                                            new_transform.e,
                                            new_transform.f};
        } else {
            // Assume it's a param key (x, y, r, w, h, cx, cy, etc.)
            new_params[prop] = value;
        }
    }

    GraphicObject result(obj.shape_type,
                         std::move(new_params),
                         std::move(new_fill),
                         std::move(new_stroke),
                         new_stroke_width,
                         new_transform,
                         obj.children,
                         obj.metadata);
    // Preserve identity so the timeline can keep targeting this object.
    result.id = obj.id;
    return result;
}

// ==========================================================================================================================================================================================================================================═
// Builtin implementations (anonymous namespace)
// ==========================================================================================================================================================================================================================================═

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
        const auto* kw = last.as_keyword();
        if (!kw)
            break; // No more keyword args

        kwargs[kw->name] = args[args.size() - 2];
        args.pop_back(); // remove value
        args.pop_back(); // remove keyword
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
        return std::unexpected(arity_error(SourceLocation{}, 5, static_cast<int>(args.size())));
    }

    // Target must be a GraphicObject
    const auto* target_ptr = args[0].as_graphic_object();
    if (!target_ptr || !*target_ptr) {
        return std::unexpected(type_error("_animate: first argument must be a GraphicObject"));
    }
    uint64_t target_id = (*target_ptr)->id;

    // Property name (from Symbol or string)
    std::string prop_name;
    if (const auto* sym = args[1].as_symbol()) {
        prop_name = sym->name;
    } else if (const auto* s = args[1].as_string()) {
        prop_name = *s;
    } else {
        return std::unexpected(type_error("_animate: property must be a symbol or string"));
    }

    // From value (number or color string)
    Value from_value;
    if (is_number(args[2])) {
        from_value = args[2];
    } else if (is_string(args[2])) {
        from_value = args[2];
    } else if (const auto* sym = args[2].as_symbol()) {
        from_value = Value(sym->name);
    } else {
        return std::unexpected(
            type_error("_animate: from value must be numeric or a color string"));
    }

    // To value (number or color string)
    Value to_value;
    if (is_number(args[3])) {
        to_value = args[3];
    } else if (is_string(args[3])) {
        to_value = args[3];
    } else if (const auto* sym = args[3].as_symbol()) {
        to_value = Value(sym->name);
    } else {
        return std::unexpected(type_error("_animate: to value must be numeric or a color string"));
    }

    // Duration
    if (!is_number(args[4])) {
        return std::unexpected(type_error("_animate: duration must be numeric"));
    }
    float duration = static_cast<float>(
        (args[4].is_double()) ? args[4].double_val() : static_cast<double>(args[4].int_val()));

    // Easing (from :ease keyword or default "linear")
    std::string ease_name = "linear";
    auto ease_it = kwargs.find("ease");
    if (ease_it != kwargs.end()) {
        if (const auto* sym = ease_it->second.as_symbol()) {
            ease_name = sym->name;
        } else if (const auto* s = ease_it->second.as_string()) {
            ease_name = *s;
        }
    }

    EasingFn ease_fn = get_easing(ease_name);

    // Delay (default 0)
    float delay = 0.0f;
    auto delay_it = kwargs.find("delay");
    if (delay_it != kwargs.end()) {
        if (is_number(delay_it->second)) {
            delay = static_cast<float>(delay_it->second.is_double()
                                           ? delay_it->second.double_val()
                                           : static_cast<double>(delay_it->second.int_val()));
        }
    }

    // Repeat (default 1, or "infinite" symbol)
    int repeat = 1;
    auto repeat_it = kwargs.find("repeat");
    if (repeat_it != kwargs.end()) {
        if (const auto* sym = repeat_it->second.as_symbol()) {
            if (sym->name == "infinite") {
                repeat = 1; // infinite not yet supported in C++ backend
            } else {
                repeat = std::max(1, static_cast<int>(std::stoi(sym->name)));
            }
        } else if (repeat_it->second.is_int()) {
            repeat = std::max(1, static_cast<int>(repeat_it->second.int_val()));
        } else if (repeat_it->second.is_double()) {
            repeat = std::max(1, static_cast<int>(repeat_it->second.double_val()));
        }
    }

    // Create animation
    auto anim = std::make_shared<Animation>(target_id,
                                            prop_name,
                                            std::move(from_value),
                                            std::move(to_value),
                                            duration,
                                            ease_fn,
                                            delay,
                                            repeat);

    // Register on global timeline
    auto timeline = get_or_create_timeline();
    timeline->add(anim);

    return Value(std::move(anim));
}

/// play builtin: (play) — start playback
Result<Value> builtin_play(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->play();
    return make_nil_value();
}

/// stop builtin: (stop) — stop and reset
Result<Value> builtin_stop(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->stop();
    return make_nil_value();
}

/// pause builtin: (pause) — pause playback
Result<Value> builtin_pause(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    timeline->pause();
    return make_nil_value();
}

/// seek builtin: (seek t) or (seek anim t) — seek to time t
Result<Value> builtin_seek(const std::vector<Value>& args, Environment&) {
    if (args.empty() || args.size() > 2) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const Value& time_arg = (args.size() == 2) ? args[1] : args[0];
    if (!is_number(time_arg)) {
        return std::unexpected(type_error("seek: time argument must be numeric"));
    }
    double t =
        (time_arg.is_double()) ? time_arg.double_val() : static_cast<double>(time_arg.int_val());

    auto timeline = get_or_create_timeline();
    timeline->seek(t);
    return make_nil_value();
}

/// animation-state builtin: return current playback state as a symbol
Result<Value> builtin_animation_state(const std::vector<Value>&, Environment&) {
    auto timeline = get_or_create_timeline();
    return Value(Symbol(timeline_state_to_string(timeline->state)));
}

/// every-frame builtin: register a thunk to be called before each frame
Result<Value> builtin_every_frame(const std::vector<Value>& args, Environment& env) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    const Value& thunk = args[0];
    bool callable = thunk.is_procedure() || thunk.is_builtin();
    if (!callable) {
        return std::unexpected(type_error("every-frame: argument must be a callable procedure"));
    }

    std::shared_ptr<Environment> env_ptr = env.shared_from_this();
    auto hook = [thunk, env_ptr](double /*t*/) {
        auto result = apply_function(thunk, {}, {}, env_ptr);
        if (!result) {
            return;
        }
        // apply_function returns a TailCall for user-defined procedures.
        // Use trampoline to actually execute the body.
        (void)trampoline(std::move(*result));
    };

    auto timeline = get_or_create_timeline();
    timeline->add_frame_hook(std::move(hook));
    return make_nil_value();
}

/// parallel builtin: (parallel anim ...) — play all animations simultaneously.
Result<Value> builtin_parallel(const std::vector<Value>& args, Environment&) {
    std::vector<std::shared_ptr<Animation>> children;
    children.reserve(args.size());
    for (const auto& arg : args) {
        if (const auto* anim_ptr = arg.as_animation()) {
            if (*anim_ptr)
                children.push_back(*anim_ptr);
        }
    }

    auto timeline = get_or_create_timeline();
    // Remove any duplicates of the child animations from the timeline.
    auto& anims = timeline->animations;
    for (const auto& child : children) {
        auto it = std::remove(anims.begin(), anims.end(), child);
        anims.erase(it, anims.end());
    }
    // Re-add all children so they play together.
    for (const auto& child : children) {
        timeline->add(child);
    }

    return make_list_value(std::vector<Value>(children.begin(), children.end()));
}

/// sequence builtin: (sequence anim ...) — play animations one after another.
Result<Value> builtin_sequence(const std::vector<Value>& args, Environment&) {
    std::vector<std::shared_ptr<Animation>> children;
    children.reserve(args.size());
    for (const auto& arg : args) {
        if (const auto* anim_ptr = arg.as_animation()) {
            if (*anim_ptr)
                children.push_back(*anim_ptr);
        }
    }

    auto timeline = get_or_create_timeline();
    // Remove originals from the timeline.
    auto& anims = timeline->animations;
    for (const auto& child : children) {
        auto it = std::remove(anims.begin(), anims.end(), child);
        anims.erase(it, anims.end());
    }

    // Re-add with cumulative delays so they run back-to-back.
    float cumulative_delay = 0.0f;
    for (const auto& child : children) {
        auto shifted = std::make_shared<Animation>(child->target_id,
                                                   child->property_name,
                                                   child->from_value,
                                                   child->to_value,
                                                   child->duration,
                                                   child->easing,
                                                   child->delay + cumulative_delay,
                                                   child->repeat);
        timeline->add(shifted);
        cumulative_delay += child->duration;
    }

    return make_list_value(std::vector<Value>(children.begin(), children.end()));
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// register_timeline_builtins
// ==========================================================================================================================================================================================================================================═

void register_timeline_builtins(std::shared_ptr<Environment> env) {
    auto def =
        [&](const std::string& name, BuiltinProcedure::BuiltinFn fn, bool accepts_kwargs = false) {
            auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
            env->define(name, Value(proc));
        };

    def("animate", builtin_animate, true); // accepts :ease :delay :repeat
    def("play", builtin_play);
    def("stop", builtin_stop);
    def("pause", builtin_pause);
    def("seek", builtin_seek);
    def("animation-state", builtin_animation_state);
    def("every-frame", builtin_every_frame);
    def("parallel", builtin_parallel);
    def("sequence", builtin_sequence);
}

} // namespace pml
