// ═══════════════════════════════════════════════════════════════════════════════
// PML Style Engine — StyleDescriptor + StyleRegistry + Builtins
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/style.py.
// ═══════════════════════════════════════════════════════════════════════════════

#include "style.h"

#include "error.h"
#include "evaluator.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers (anonymous namespace)
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// ── String extraction from Value ──────────────────────────────────────────

/// Extract a string from a Value (string, Symbol, or Keyword).
[[nodiscard]] std::optional<std::string> extract_string(const Value& v) {
    return std::visit([](const auto& arg) -> std::optional<std::string> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, Symbol>) {
            return arg.name;
        } else if constexpr (std::is_same_v<T, Keyword>) {
            return arg.name;
        }
        return std::nullopt;
    }, v);
}

// ── Kwarg extraction helpers ──────────────────────────────────────────────

[[nodiscard]] std::optional<std::string> kw_string(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    return extract_string(it->second);
}

[[nodiscard]] std::optional<double> kw_float(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    return std::visit([](const auto& arg) -> std::optional<double> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return static_cast<double>(arg);
        }
        return std::nullopt;
    }, it->second);
}

[[nodiscard]] std::optional<int64_t> kw_int(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    if (const auto* i = std::get_if<int64_t>(&it->second)) {
        return *i;
    }
    if (const auto* d = std::get_if<double>(&it->second)) {
        return static_cast<int64_t>(*d);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<bool> kw_bool(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    if (const auto* b = std::get_if<bool>(&it->second)) {
        return *b;
    }
    return std::nullopt;
}

// ── Flat-list kwargs parser ───────────────────────────────────────────────

/// Parse keyword-value pairs from a flat vector starting at `start`.
/// Handles both Keyword and Symbol keys.  Stops at the first non-key value.
[[nodiscard]] std::unordered_map<std::string, Value> parse_kwargs(
    const std::vector<Value>& args, size_t start)
{
    std::unordered_map<std::string, Value> result;
    for (size_t i = start; i + 1 < args.size(); i += 2) {
        if (const auto* kw = std::get_if<Keyword>(&args[i])) {
            result[kw->name] = args[i + 1];
        } else if (const auto* sym = std::get_if<Symbol>(&args[i])) {
            result[sym->name] = args[i + 1];
        } else {
            break;
        }
    }
    return result;
}

// ── Predefined style factories ────────────────────────────────────────────

StyleDescriptor make_cel_style()
{
    StyleDescriptor s;
    s.outline_width = 2.5f;
    s.outline_color = "#1a1a1a";
    s.shading = "cel";
    s.highlight = true;
    s.anti_alias = true;
    return s;
}

StyleDescriptor make_pixel_style()
{
    StyleDescriptor s;
    s.outline_width = 1.0f;
    s.outline_color = "#000000";
    s.shading = "pixel";
    s.highlight = false;
    s.pixel_size = 2;
    s.anti_alias = false;
    return s;
}

StyleDescriptor make_flat_style()
{
    StyleDescriptor s;
    s.outline_width = 0.0f;
    s.outline_style = "none";
    s.shading = "flat";
    s.highlight = false;
    s.anti_alias = true;
    return s;
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// StyleDescriptor
// ═══════════════════════════════════════════════════════════════════════════════

std::unordered_map<std::string, Value> StyleDescriptor::to_kwargs() const
{
    return {
        {"outline-width",  Value(static_cast<double>(outline_width))},
        {"outline-color",  Value(outline_color)},
        {"outline-style",  Value(outline_style)},
        {"shading",        Value(shading)},
        {"shadow",         Value(shadow)},
        {"highlight",      Value(highlight)},
        {"pixel-size",     Value(static_cast<int64_t>(pixel_size))},
        {"anti-alias",     Value(anti_alias)},
        {"corner-radius",  Value(static_cast<double>(corner_radius))},
    };
}

StyleDescriptor StyleDescriptor::merge(
    const std::unordered_map<std::string, Value>& overrides) const
{
    StyleDescriptor result = *this;  // copy all fields

    auto val = kw_float(overrides, "outline-width");
    if (val) result.outline_width = static_cast<float>(*val);

    auto s = kw_string(overrides, "outline-color");
    if (s) result.outline_color = std::move(*s);

    s = kw_string(overrides, "outline-style");
    if (s) result.outline_style = std::move(*s);

    s = kw_string(overrides, "shading");
    if (s) result.shading = std::move(*s);

    auto b = kw_bool(overrides, "shadow");
    if (b) result.shadow = *b;

    b = kw_bool(overrides, "highlight");
    if (b) result.highlight = *b;

    auto i = kw_int(overrides, "pixel-size");
    if (i) result.pixel_size = static_cast<int>(*i);

    b = kw_bool(overrides, "anti-alias");
    if (b) result.anti_alias = *b;

    val = kw_float(overrides, "corner-radius");
    if (val) result.corner_radius = static_cast<float>(*val);

    return result;
}

StyleDescriptor StyleDescriptor::merge(const StyleDescriptor& overrides) const
{
    StyleDescriptor result = *this;  // copy all fields

    result.outline_width  = overrides.outline_width;
    result.outline_color  = overrides.outline_color;
    result.outline_style  = overrides.outline_style;
    result.shading        = overrides.shading;
    result.shadow         = overrides.shadow;
    result.highlight      = overrides.highlight;
    result.pixel_size     = overrides.pixel_size;
    result.anti_alias     = overrides.anti_alias;
    result.corner_radius  = overrides.corner_radius;

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// StyleRegistry
// ═══════════════════════════════════════════════════════════════════════════════

StyleRegistry::StyleRegistry()
{
    // Predefined styles matching Python pml/sprites/style.py exactly
    m_styles["cel"]   = make_cel_style();
    m_styles["pixel"] = make_pixel_style();
    m_styles["flat"]  = make_flat_style();
}

StyleRegistry& StyleRegistry::instance()
{
    static StyleRegistry registry;
    return registry;
}

void StyleRegistry::define(const std::string& name, const StyleDescriptor& style)
{
    m_styles[name] = style;
}

StyleDescriptor StyleRegistry::get(const std::string& name) const
{
    auto it = m_styles.find(name);
    if (it != m_styles.end()) {
        return it->second;
    }
    // Fallback to "cel" with a warning (matching Python behavior)
    std::cerr << "Warning: unknown style '" << name
              << "', falling back to 'cel'" << std::endl;
    auto cel_it = m_styles.find("cel");
    if (cel_it != m_styles.end()) {
        return cel_it->second;
    }
    // Absolute fallback — default StyleDescriptor (defensive)
    return StyleDescriptor{};
}

bool StyleRegistry::has(const std::string& name) const
{
    return m_styles.find(name) != m_styles.end();
}

// ═══════════════════════════════════════════════════════════════════════════════
// resolve_style
// ═══════════════════════════════════════════════════════════════════════════════

StyleDescriptor resolve_style(const Value& style_val)
{
    // shared_ptr<StyleDescriptor> — return a copy
    if (const auto* sp = std::get_if<std::shared_ptr<StyleDescriptor>>(&style_val)) {
        if (*sp) return **sp;
    }

    // Symbol — look up by name
    if (const auto* sym = std::get_if<Symbol>(&style_val)) {
        return StyleRegistry::instance().get(sym->name);
    }

    // String — look up by name
    if (const auto* s = std::get_if<std::string>(&style_val)) {
        return StyleRegistry::instance().get(*s);
    }

    // Fallback — return "cel"
    return StyleRegistry::instance().get("cel");
}

// ═══════════════════════════════════════════════════════════════════════════════
// register_style — builtins
// ═══════════════════════════════════════════════════════════════════════════════

void register_style(std::shared_ptr<Environment> env)
{
    if (!env) return;

    // ── (define-style name :outline-width ... :shading ...) ─────────────
    //
    // Define or update a named style.  Name is a Symbol or string (first
    // positional arg).  All remaining args are keyword-value pairs for the
    // StyleDescriptor fields.
    {
        auto define_style_fn = [](const std::vector<Value>& args,
                                  Environment& /*env*/) -> Result<Value>
        {
            if (args.empty()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, 0));
            }

            // Extract style name from arg[0]
            std::string style_name;
            if (const auto* sym = std::get_if<Symbol>(&args[0])) {
                style_name = sym->name;
            } else if (const auto* s = std::get_if<std::string>(&args[0])) {
                style_name = *s;
            } else {
                return std::unexpected(type_error(
                    "define-style: expected a Symbol or string as the style name"));
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

    // ── (use-style name) → StyleDescriptor ──────────────────────────────
    //
    // Look up a named style and return it as a StyleDescriptor value.
    {
        auto use_style_fn = [](const std::vector<Value>& args,
                               Environment& /*env*/) -> Result<Value>
        {
            if (args.empty()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, 0));
            }

            std::string style_name;
            if (const auto* sym = std::get_if<Symbol>(&args[0])) {
                style_name = sym->name;
            } else if (const auto* s = std::get_if<std::string>(&args[0])) {
                style_name = *s;
            } else {
                return std::unexpected(type_error(
                    "use-style: expected a Symbol or string as the style name"));
            }

            auto descriptor = StyleRegistry::instance().get(style_name);
            return Value(std::make_shared<StyleDescriptor>(std::move(descriptor)));
        };

        env->define("use-style",
            Value(std::make_shared<BuiltinProcedure>(
                "use-style", std::move(use_style_fn), false)));
    }
}

}  // namespace pml
