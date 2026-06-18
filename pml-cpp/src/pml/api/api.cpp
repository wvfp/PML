// ═══════════════════════════════════════════════════════════════════════════════
// PML API — PMLRuntime facade implementation.
// Ported from pml/api.py.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/api/api.h"

// ── Frontend (lex → parse → expand) ──────────────────────────────────────
#include "pml/frontend/lexer.h"
#include "pml/frontend/parser.h"
#include "pml/frontend/expander.h"

// ── Evaluator & builtins ─────────────────────────────────────────────────
#include "pml/evaluator/evaluator.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/builtins.h"
#include "pml/evaluator/backend_builtins.h"
#include "pml/evaluator/shader_builtins.h"

// ── Graphics ─────────────────────────────────────────────────────────────
#include "pml/graphics/render.h"

// ── Sprites ──────────────────────────────────────────────────────────────
#include "pml/sprites/style.h"
#include "pml/sprites/palette.h"
#include "pml/sprites/registry.h"

// ── Skeleton & IK ────────────────────────────────────────────────────────
#include "pml/skeleton/skeleton.h"
#include "pml/skeleton/ik_ccd.h"

// ── Animation (timeline) ─────────────────────────────────────────────
#include "pml/animation/timeline.h"

// ── Skin binding ─────────────────────────────────────────────────────
#include "pml/skeleton/skin_binding.h"

// ── Module system ────────────────────────────────────────────────────────
// register_module_builtins is not yet implemented.  Module loading is
// handled via special forms in the evaluator (eval_import / eval_provide).

// ── Transform / Canvas builtins ──────────────────────────────────────────
#include "pml/evaluator/transform_builtins.h"
#include "pml/evaluator/canvas_builtins.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════
// RenderResult
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::json RenderResult::to_json() const {
    nlohmann::json j;
    j["success"] = success;
    j["value"] = value;
    j["files"] = files;
    j["error"] = error.has_value() ? *error : nlohmann::json(nullptr);
    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::json serialize_value(const Value& val) {
    // nil → null
    if (is_nil(val)) {
        return nullptr;
    }
    // bool → bool
    if (is_bool(val)) {
        return std::get<bool>(val);
    }
    // int → number
    if (is_integer(val)) {
        return std::get<int64_t>(val);
    }
    // float → number
    if (is_float(val)) {
        return std::get<double>(val);
    }
    // string → string
    if (is_string(val)) {
        return std::get<std::string>(val);
    }
    // Symbol → "<symbol:name>"
    if (is_symbol(val)) {
        return std::format("<symbol:{}>", std::get<Symbol>(val).name);
    }
    // Keyword → "<keyword:name>"
    if (is_keyword(val)) {
        return std::format("<keyword:{}>", std::get<Keyword>(val).name);
    }
    // List → array (recursive)
    if (is_list(val)) {
        const auto& elements = (*std::get<std::shared_ptr<ValueList>>(val)).elements;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& elem : elements) {
            arr.push_back(serialize_value(elem));
        }
        return arr;
    }
    // BuiltinProcedure → "<builtin:name>"
    if (is_builtin(val)) {
        return std::format("<builtin:{}>",
                           std::get<std::shared_ptr<BuiltinProcedure>>(val)->name);
    }
    // Procedure → "<procedure>"
    if (is_procedure(val)) {
        const auto& proc = *std::get<std::shared_ptr<Procedure>>(val);
        if (proc.name.has_value()) {
            return std::format("<procedure:{}>", *proc.name);
        }
        return "<procedure>";
    }
    // Macro → "<macro:name>"
    if (is_macro(val)) {
        return std::format("<macro:{}>",
                           std::get<std::shared_ptr<Macro>>(val)->name);
    }
    // GraphicObject → "<graphic-object:type>"
    if (std::holds_alternative<std::shared_ptr<GraphicObject>>(val)) {
        return std::format("<graphic-object:{}>",
                           std::get<std::shared_ptr<GraphicObject>>(val)->shape_type);
    }
    // Module → "<module>"
    // (Module is only forward-declared in types.h, so we can't access its members)
    if (std::holds_alternative<std::shared_ptr<Module>>(val)) {
        return "<module>";
    }
    // Canvas → "<canvas>"
    if (std::holds_alternative<std::shared_ptr<Canvas>>(val)) {
        return "<canvas>";
    }
    // Fallback: use value_to_string
    return value_to_string(val);
}

std::string generate_hint(ErrorCode code) {
    using enum ErrorCode;
    switch (code) {
        case UnboundVariableError:
            return "The variable is not defined in any scope. "
                   "Use (define name value) to define it, or (import \"path\" as prefix) "
                   "to import from a module.";
        case ArityError:
            return "Check the number of arguments passed to this function or special form.";
        case PMLSyntaxError:
            return "Check for unmatched parentheses, unterminated strings, or invalid tokens.";
        case PMLTypeError:
            return "Check the types of arguments. Ensure symbols, strings, and numbers are used correctly.";
        case CircularImportError:
            return "Circular dependency detected. Break the cycle by removing one of the "
                   "mutual imports, or restructure into separate modules.";
        case ImportError:
            return "Check the file path. Use forward slashes (/) in paths, not backslashes. "
                   "Paths are resolved relative to the importing file.";
        case MacroExpansionDepthError:
            return "A macro is expanding recursively without a base case. "
                   "Add a termination condition or restructure to avoid infinite expansion.";
        case AccessError:
            return "This symbol is not exported from the module. "
                   "Add it to (provide ...) in the module file.";
        case ResourceError:
            return "Check that the referenced resource file exists and is accessible.";
        case IKNoSolutionError:
            return "The IK solver could not reach the target. Try increasing :iterations, "
                   "relaxing joint constraints (:min/:max), or moving the target closer.";
        default:
            return "";
    }
}

nlohmann::json error_to_dict(const PMLException& e) {
    nlohmann::json d;
    d["type"]    = std::string(to_string(e.code));
    d["message"] = e.message;
    if (e.location.has_value()) {
        const auto& loc = *e.location;
        if (loc.line > 0)   d["line"]   = loc.line;
        if (loc.column > 0) d["column"] = loc.column;
        if (!loc.filename.empty()) d["filename"] = loc.filename;
    }
    d["hint"] = e.repair_hint.has_value() ? *e.repair_hint : generate_hint(e.code);
    return d;
}

// ═══════════════════════════════════════════════════════════════════════════
// PMLRuntime implementation
// ═══════════════════════════════════════════════════════════════════════════

PMLRuntime::PMLRuntime()
    : m_env(std::make_shared<Environment>())
{
    init_global_env();
}

void PMLRuntime::init_global_env() {
    // 1. Core builtins (arithmetic, comparison, IO, list/string ops, predicates)
    register_builtins(m_env);

    // 2. Graphics render builtins (render, render-set, render-spritesheet)
    register_render(m_env);

    // 3. Skeleton system (defskeleton, instantiate-skeleton, joint-position)
    register_skeleton(m_env);

    // 4. IK solver (ik-solve — dispatches to CCD or FABRIK)
    register_ik(m_env);

    // 5. Style engine (define-style, use-style)
    register_style(m_env);

    // 6. Palette system (define-palette, palette-ref)
    register_palette(m_env);

    // 6.5 Sprite semantic components (body, head, eyes, hair, outfit, items, UI, scene)
    register_components(m_env);

    // 7. Animation timeline (_animate, _play, _stop, _pause, _seek)
    register_timeline_builtins(m_env);

    // 8. Skin binding (bind-skin)
    register_skin_binding(m_env);

    // 9. Backend builtins (list-backends, set-backend!, current-backend,
    //    backend-capabilities, backend-available?)
    register_backend_builtins(m_env);

    // 10. Shader builtins (shader, apply-shader!) — stubs, Skia not available
    register_shader_builtins(m_env);

    // 11. Transform builtins (translate, rotate, scale, shear, identity-matrix,
    //     compose, matrix-inverse, matrix-apply)
    register_transform_builtins(m_env);

    // 12. Canvas / shape / style / group / color builtins
    //     (canvas, sprite-canvas, add, circle, rect, ellipse, line, polygon,
    //      text, fill, stroke, no-fill, no-stroke, stroke-width, group,
    //      with-transform, translate-object, rotate-object, scale-object,
    //      color/rgb, color/rgba)
    register_canvas_builtins(m_env);

    // ── Module loading is handled via the evaluator's eval_import/eval_provide
    // special forms — no separate module registration call needed.
}

RenderResult PMLRuntime::execute(
    const std::string& source,
    const std::string& filename)
{
    // Lexer
    auto tokenResult = Lexer(source, filename).tokenize();
    if (!tokenResult.has_value()) {
        RenderResult r;
        r.success = false;
        r.error   = error_to_dict(tokenResult.error());
        return r;
    }
    auto& tokens = *tokenResult;

    // Parser
    auto parseResult = Parser(std::move(tokens), filename).parse();
    if (!parseResult.has_value()) {
        RenderResult r;
        r.success = false;
        r.error   = error_to_dict(parseResult.error());
        return r;
    }
    auto ast = std::move(*parseResult);

    // Expander (macro expansion)
    Expander expander(m_env);
    auto expandResult = expander.expand_all(ast);
    if (!expandResult.has_value()) {
        RenderResult r;
        r.success = false;
        r.error   = error_to_dict(expandResult.error());
        return r;
    }
    auto expanded = std::move(*expandResult);

    // Evaluate each expression sequentially
    Value last_value;
    bool has_value = false;
    for (const auto& expr : expanded) {
        auto evalResult = evaluate(expr, m_env);
        if (!evalResult.has_value()) {
            RenderResult r;
            r.success = false;
            r.error   = error_to_dict(evalResult.error());
            return r;
        }
        last_value = std::move(*evalResult);
        has_value = true;
    }

    // Build successful result
    RenderResult r;
    r.success = true;
    r.files   = {};  // Output files are produced by render builtins, not tracked here
    r.error   = std::nullopt;
    if (has_value) {
        r.value = serialize_value(last_value);
    } else {
        r.value = nullptr;
    }
    return r;
}

RenderResult PMLRuntime::execute_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        RenderResult r;
        r.success = false;
        r.error = nlohmann::json{
            {"type", "ResourceError"},
            {"message", std::format("Cannot open file: {}", path)},
            {"hint", "Check that the file exists and is readable."},
        };
        return r;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return execute(ss.str(), path);
}

nlohmann::json PMLRuntime::execute_pml(
    const std::string& source,
    const nlohmann::json& /*options*/)
{
    auto result = execute(source);
    return result.to_json();
}

std::shared_ptr<Environment> PMLRuntime::env() const noexcept {
    return m_env;
}

}  // namespace pml
