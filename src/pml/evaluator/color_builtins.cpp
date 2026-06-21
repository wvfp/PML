// ═══════════════════════════════════════════════════════════════════════════════
// PML Color Helper Builtins
//
// Extracted from canvas_builtins.cpp.  Implements:
//   color/rgb, color/rgba
// ═══════════════════════════════════════════════════════════════════════════════

#include "color_builtins.h"

#include "builtins_helpers.h"
#include "error.h"
#include "types.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <vector>

namespace pml {

namespace {

// ── Registration helper ─────────────────────────────────────────────────

void def(std::shared_ptr<Environment> env,
         const std::string& name,
         BuiltinProcedure::BuiltinFn fn) {
    auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
    env->define(name, Value(proc));
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Color helper builtins
// ═══════════════════════════════════════════════════════════════════════════════

// ── (color/rgb r g b) → "#RRGGBB" ───────────────────────────────────────────
//
// r, g, b are integers 0-255.

static Result<Value> builtin_color_rgb(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(3, args, "color/rgb");
    if (!r)
        return std::unexpected(r.error());

    int red = std::clamp(value_to_int(args[0]), 0, 255);
    int green = std::clamp(value_to_int(args[1]), 0, 255);
    int blue = std::clamp(value_to_int(args[2]), 0, 255);

    return Value(std::format("#{:02x}{:02x}{:02x}", red, green, blue));
}

// ── (color/rgba r g b a) → "#RRGGBBAA" ──────────────────────────────────────
//
// r, g, b are integers 0-255. a is a float 0.0-1.0 (or integer 0-255).

static Result<Value> builtin_color_rgba(const std::vector<Value>& args, Environment& /*env*/) {
    auto r = expect_arity(4, args, "color/rgba");
    if (!r)
        return std::unexpected(r.error());

    int red = std::clamp(value_to_int(args[0]), 0, 255);
    int green = std::clamp(value_to_int(args[1]), 0, 255);
    int blue = std::clamp(value_to_int(args[2]), 0, 255);

    int alpha;
    if (args[3].is_double()) {
        // Float 0.0-1.0
        double a = args[3].double_val();
        alpha = std::clamp(static_cast<int>(std::round(a * 255.0)), 0, 255);
    } else {
        alpha = std::clamp(value_to_int(args[3]), 0, 255);
    }

    return Value(std::format("#{:02x}{:02x}{:02x}{:02x}", red, green, blue, alpha));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_color_builtins(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ── Color helper builtins ───────────────────────────────────────────
    def(env, "color/rgb", builtin_color_rgb);
    def(env, "color/rgba", builtin_color_rgba);
}

} // namespace pml
