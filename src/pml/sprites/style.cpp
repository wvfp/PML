// ==========================================================================================================================================================================================================================================═
// PML Style Engine — StyleDescriptor + StyleRegistry + Builtins
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Ported from pml/sprites/style.py.
// ==========================================================================================================================================================================================================================================═

#include "style.h"

#include "error.h"
#include "evaluator.h"
#include "pml/core/kwargs.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Internal helpers (anonymous namespace)
// ==========================================================================================================================================================================================================================================═

namespace {

using pml::kwargs::kw_double;
using pml::kwargs::kw_int;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;

/// Extract an optional boolean from kwargs.
[[nodiscard]] std::optional<bool> kw_bool(const std::unordered_map<std::string, Value>& kwargs,
                                          const std::string& key) {
    auto it = kwargs.find(key);
    if (it == kwargs.end())
        return std::nullopt;
    if (it->second.is_bool()) {
        return it->second.bool_val();
    }
    return std::nullopt;
}

// ---- Predefined style factories ----------------------------------------------------------------------------------------

StyleDescriptor make_cel_style() {
    StyleDescriptor s;
    s.outline_width = 2.5f;
    s.outline_color = "#1a1a1a";
    s.shading = "cel";
    s.highlight = true;
    s.anti_alias = true;
    return s;
}

StyleDescriptor make_pixel_style() {
    StyleDescriptor s;
    s.outline_width = 1.0f;
    s.outline_color = "#000000";
    s.shading = "pixel";
    s.highlight = false;
    s.pixel_size = 2;
    s.anti_alias = false;
    return s;
}

StyleDescriptor make_flat_style() {
    StyleDescriptor s;
    s.outline_width = 0.0f;
    s.outline_style = "none";
    s.shading = "flat";
    s.highlight = false;
    s.anti_alias = true;
    return s;
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// StyleDescriptor
// ==========================================================================================================================================================================================================================================═

std::unordered_map<std::string, Value> StyleDescriptor::to_kwargs() const {
    return {
        {"outline-width", Value(static_cast<double>(outline_width))},
        {"outline-color", Value(outline_color)},
        {"outline-style", Value(outline_style)},
        {"shading", Value(shading)},
        {"shadow", Value(shadow)},
        {"highlight", Value(highlight)},
        {"pixel-size", Value(static_cast<int64_t>(pixel_size))},
        {"anti-alias", Value(anti_alias)},
        {"corner-radius", Value(static_cast<double>(corner_radius))},
        {"roughness", Value(static_cast<double>(roughness))},
        {"bowing", Value(static_cast<double>(bowing))},
        {"seed", Value(static_cast<int64_t>(seed))},
        {"fill-style", Value(fill_style)},
    };
}

StyleDescriptor
StyleDescriptor::merge(const std::unordered_map<std::string, Value>& overrides) const {
    StyleDescriptor result = *this; // copy all fields

    if (overrides.contains("outline-width")) {
        result.outline_width = static_cast<float>(kw_double(overrides, "outline-width", 0.0));
    }

    if (overrides.contains("outline-color")) {
        result.outline_color = kw_string(overrides, "outline-color", "");
    }

    if (overrides.contains("outline-style")) {
        result.outline_style = kw_string(overrides, "outline-style", "");
    }

    if (overrides.contains("shading")) {
        result.shading = kw_string(overrides, "shading", "");
    }

    auto b = kw_bool(overrides, "shadow");
    if (b)
        result.shadow = *b;

    b = kw_bool(overrides, "highlight");
    if (b)
        result.highlight = *b;

    if (overrides.contains("pixel-size")) {
        result.pixel_size = static_cast<int>(kw_int(overrides, "pixel-size", 0));
    }

    b = kw_bool(overrides, "anti-alias");
    if (b)
        result.anti_alias = *b;

    if (overrides.contains("corner-radius")) {
        result.corner_radius = static_cast<float>(kw_double(overrides, "corner-radius", 0.0));
    }

    // ---- Rough-style fields --------------------------------------------------------------------------------─
    if (overrides.contains("roughness")) {
        result.roughness = static_cast<float>(kw_double(overrides, "roughness", 0.0));
    }
    if (overrides.contains("bowing")) {
        result.bowing = static_cast<float>(kw_double(overrides, "bowing", 1.0));
    }
    if (overrides.contains("seed")) {
        result.seed = static_cast<int>(kw_int(overrides, "seed", 0));
    }
    if (overrides.contains("fill-style")) {
        result.fill_style = kw_string(overrides, "fill-style", "solid");
    }

    return result;
}

StyleDescriptor StyleDescriptor::merge(const StyleDescriptor& overrides) const {
    StyleDescriptor result = *this; // copy all fields

    result.outline_width = overrides.outline_width;
    result.outline_color = overrides.outline_color;
    result.outline_style = overrides.outline_style;
    result.shading = overrides.shading;
    result.shadow = overrides.shadow;
    result.highlight = overrides.highlight;
    result.pixel_size = overrides.pixel_size;
    result.anti_alias = overrides.anti_alias;
    result.corner_radius = overrides.corner_radius;

    result.roughness = overrides.roughness;
    result.bowing = overrides.bowing;
    result.seed = overrides.seed;
    result.fill_style = overrides.fill_style;

    return result;
}

// ==========================================================================================================================================================================================================================================═
// StyleRegistry
// ==========================================================================================================================================================================================================================================═

StyleRegistry::StyleRegistry() {
    // Predefined styles matching Python pml/sprites/style.py exactly
    m_styles["cel"] = make_cel_style();
    m_styles["pixel"] = make_pixel_style();
    m_styles["flat"] = make_flat_style();
}

StyleRegistry& StyleRegistry::instance() {
    auto& ctx = PMLContext::current();
    if (!ctx.styles) {
        ctx.styles = std::make_unique<StyleRegistry>();
    }
    return *ctx.styles;
}

void StyleRegistry::define(const std::string& name, const StyleDescriptor& style) {
    m_styles[name] = style;
}

StyleDescriptor StyleRegistry::get(const std::string& name) const {
    auto it = m_styles.find(name);
    if (it != m_styles.end()) {
        return it->second;
    }
    // Fallback to "cel" with a warning (matching Python behavior)
    std::cerr << "Warning: unknown style '" << name << "', falling back to 'cel'" << std::endl;
    auto cel_it = m_styles.find("cel");
    if (cel_it != m_styles.end()) {
        return cel_it->second;
    }
    // Absolute fallback — default StyleDescriptor (defensive)
    return StyleDescriptor{};
}

bool StyleRegistry::has(const std::string& name) const {
    return m_styles.find(name) != m_styles.end();
}

// ==========================================================================================================================================================================================================================================═
// resolve_style
// ==========================================================================================================================================================================================================================================═

StyleDescriptor resolve_style(const Value& style_val) {
    // shared_ptr<StyleDescriptor> — return a copy
    if (const auto* sp = style_val.as_style()) {
        if (*sp)
            return **sp;
    }

    // Symbol — look up by name
    if (const auto* sym = style_val.as_symbol()) {
        return StyleRegistry::instance().get(sym->name);
    }

    // String — look up by name
    if (const auto* s = style_val.as_string()) {
        return StyleRegistry::instance().get(*s);
    }

    // Fallback — return "cel"
    return StyleRegistry::instance().get("cel");
}

// ==========================================================================================================================================================================================================================================═
// register_style — builtins
// ==========================================================================================================================================================================================================================================═

void register_style(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ---- (define-style name :outline-width ... :shading ...) ------------------------─
    //
    // Define or update a named style.  Name is a Symbol or string (first
    // positional arg).  All remaining args are keyword-value pairs for the
    // StyleDescriptor fields.
    {
        auto define_style_fn = [](const std::vector<Value>& args,
                                  Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(arity_error(SourceLocation{}, 1, 0));
            }

            // Extract style name from arg[0]
            std::string style_name;
            if (const auto* sym = args[0].as_symbol()) {
                style_name = sym->name;
            } else if (const auto* s = args[0].as_string()) {
                style_name = *s;
            } else {
                return std::unexpected(
                    type_error("define-style: expected a Symbol or string as the style name"));
            }

            // Parse keyword arguments from args[1:] (flat-list pattern)
            auto kwargs = parse_kwargs(args, 1);

            // Build StyleDescriptor: start with defaults, apply overrides
            StyleDescriptor style;
            style = style.merge(kwargs);

            StyleRegistry::instance().define(style_name, style);
            return make_nil_value();
        };

        env->define("define-style",
                    Value(std::make_shared<BuiltinProcedure>(
                        "define-style", std::move(define_style_fn), true)));
    }

    // ---- (use-style name) → StyleDescriptor ------------------------------------------------------------
    //
    // Look up a named style and return it as a StyleDescriptor value.
    {
        auto use_style_fn = [](const std::vector<Value>& args,
                               Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(arity_error(SourceLocation{}, 1, 0));
            }

            std::string style_name;
            if (const auto* sym = args[0].as_symbol()) {
                style_name = sym->name;
            } else if (const auto* s = args[0].as_string()) {
                style_name = *s;
            } else {
                return std::unexpected(
                    type_error("use-style: expected a Symbol or string as the style name"));
            }

            auto descriptor = StyleRegistry::instance().get(style_name);
            return Value(std::make_shared<StyleDescriptor>(std::move(descriptor)));
        };

        env->define(
            "use-style",
            Value(std::make_shared<BuiltinProcedure>("use-style", std::move(use_style_fn), false)));
    }
}

} // namespace pml
