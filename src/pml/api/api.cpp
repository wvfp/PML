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
#include "pml/core/call_stack.h"
#include "pml/evaluator/evaluator.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/builtins.h"
#include "pml/evaluator/backend_builtins.h"
#include "pml/evaluator/shader_builtins.h"
#include "pml/evaluator/tilemap_builtins.h"
#include "pml/evaluator/render_channels_builtins.h"
#include "pml/evaluator/multi_texture_builtins.h"
#include "pml/evaluator/perturb_builtins.h"
#include "pml/evaluator/builtins_threading.h"
#include "pml/evaluator/builtins_polymorphic.h"
#include "pml/evaluator/texture_builtins.h"
#include "pml/evaluator/texture_map_builtins.h"

// ── Graphics ─────────────────────────────────────────────────────────────
#include "pml/graphics/render.h"

// ── Layers / Composition ─────────────────────────────────────────────────
#include "pml/layer/layer_builtins.h"

// ── Filters ──────────────────────────────────────────────────────────────
#include "pml/filter/filter_builtins.h"

// ── Assets / Bitmap I/O ──────────────────────────────────────────────────
#include "pml/asset/asset_builtins.h"

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

// ── 3D graphics builtins ─────────────────────────────────────────────────
#include "pml/graphics3d/builtins_3d.h"

#include <algorithm>
#include <filesystem>
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
        return val.bool_val();
    }
    // int → number
    if (is_integer(val)) {
        return val.int_val();
    }
    // float → number
    if (is_float(val)) {
        return val.double_val();
    }
    // string → string
    if (const auto* s = val.as_string()) {
        return *s;
    }
    // Symbol → "<symbol:name>"
    if (const auto* sym = val.as_symbol()) {
        return std::format("<symbol:{}>", sym->name);
    }
    // Keyword → "<keyword:name>"
    if (const auto* kw = val.as_keyword()) {
        return std::format("<keyword:{}>", kw->name);
    }
    // List → array (recursive)
    if (const auto* lst = val.as_list()) {
        if (!*lst)
            return nlohmann::json::array();
        const auto& elements = (*lst)->elements;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& elem : elements) {
            arr.push_back(serialize_value(elem));
        }
        return arr;
    }
    // BuiltinProcedure → "<builtin:name>"
    if (const auto* bp = val.as_builtin()) {
        if (!*bp)
            return "<builtin>";
        return std::format("<builtin:{}>", (*bp)->name);
    }
    // Procedure → "<procedure>"
    if (const auto* proc = val.as_procedure()) {
        if (!*proc)
            return "<procedure>";
        if ((*proc)->name.has_value()) {
            return std::format("<procedure:{}>", *(*proc)->name);
        }
        return "<procedure>";
    }
    // Macro → "<macro:name>"
    if (const auto* mac = val.as_macro()) {
        if (!*mac)
            return "<macro>";
        return std::format("<macro:{}>", (*mac)->name);
    }
    // GraphicObject → "<graphic-object:type>"
    if (const auto* go = val.as_graphic_object()) {
        if (!*go)
            return "<graphic-object>";
        return std::format("<graphic-object:{}>", (*go)->shape_type);
    }
    // Module → "<module>"
    if (val.is_module()) {
        return "<module>";
    }
    // Canvas → "<canvas>"
    if (val.is_canvas()) {
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
        return "Check the types of arguments. Ensure symbols, strings, and numbers are used "
               "correctly.";
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
    d["type"] = std::string(to_string(e.code));
    d["message"] = e.message;
    if (e.location.has_value()) {
        const auto& loc = *e.location;
        if (loc.line > 0)
            d["line"] = loc.line;
        if (loc.column > 0)
            d["column"] = loc.column;
        if (!loc.filename.empty())
            d["filename"] = loc.filename;
    }
    d["hint"] = e.repair_hint.has_value() ? *e.repair_hint : generate_hint(e.code);
    if (!e.call_stack.empty()) {
        nlohmann::json stack = nlohmann::json::array();
        for (const auto& frame : e.call_stack) {
            nlohmann::json f;
            f["function"] = frame.function_name.empty() ? "<anonymous>" : frame.function_name;
            f["location"] = frame.call_site.to_string();
            stack.push_back(f);
        }
        d["call_stack"] = stack;
    }
    return d;
}

// ═══════════════════════════════════════════════════════════════════════════
// PMLRuntime implementation
// ═══════════════════════════════════════════════════════════════════════════

PMLRuntime::PMLRuntime()
    : m_env(std::make_shared<Environment>()) {
    // Initialize per-runtime mutable state (styles, palettes, etc.) so that
    // each PMLRuntime instance is isolated from others.
    ctx_.reset();
    init_global_env();
}

void PMLRuntime::init_global_env() {
    // 1. Core builtins (arithmetic, comparison, IO, list/string ops, predicates)
    register_builtins(m_env);

    // 2. Graphics render builtins (render, render-set, render-spritesheet)
    register_render(m_env);

    // 3. Skeleton system (defskeleton, instantiate-skeleton, joint-position)
    // [封存] 骨骼系统已封存，不再注册到运行时
    // register_skeleton(m_env);

    // 4. IK solver (ik-solve — dispatches to CCD or FABRIK)
    // [封存] IK 求解器已封存，不再注册到运行时
    // register_ik(m_env);

    // 5. Style engine (define-style, use-style)
    register_style(m_env);

    // 6. Palette system (define-palette, palette-ref)
    register_palette(m_env);

    // 6.5 Sprite semantic components (body, head, eyes, hair, outfit, items, UI, scene)
    register_components(m_env);

    // 7. Animation timeline (_animate, _play, _stop, _pause, _seek)
    register_timeline_builtins(m_env);

    // 8. Skin binding (bind-skin)
    // [封存] 皮肤绑定已封存，不再注册到运行时
    // register_skin_binding(m_env);

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

    // 13. 3D graphics builtins
    //     (camera, cube3d, cuboid3d, rotate-x, rotate-y, rotate-z,
    //      translate3d, scale3d)
    register_3d_builtins(m_env);

    // 14. Layer / Composition builtins
    //     (make-layer, make-group, make-composition, composition-add, layer-with,
    //      composition-render, composition-animate, project-render-all,
    //      layer?, composition?)
    register_layer_builtins(m_env);

    // 15. Filter builtins
    //     (color-adjust, levels, curves, threshold, posterize,
    //      blur, sharpen, edge-detect, emboss, convolution,
    //      drop-shadow, inner-shadow, outer-glow, inner-glow, bevel-emboss,
    //      filter-chain, filter?)
    register_filter_builtins(m_env);

    // 16. Asset / Bitmap I/O builtins
    //     (image, load-image, bitmap-layer, load-spritesheet,
    //      asset-path?, clear-assets!)
    register_asset_builtins(m_env);

    // 17. Tilemap builtins
    //     (define-tileset, make-tilemap, tilemap-set!, render-tilemap)
    register_tilemap_builtins(m_env);

    // 18. Render channels builtins
    //     (render-channels graphic :output "path" [:channels ...] [:width W] [:height H] [:bg "transparent"])
    register_render_channels(m_env);

    // 19. Multi-texture shader builtins
    //     (bind-textures shader-handle :textures '((slot-name graphic-obj) ...))
    register_multi_texture_builtins(m_env);

    // 20. Polygon perturbation builtins
    //     (perturb-polygon points :edge-noise N ...)
    register_perturb_builtins(m_env);

    // 21. Threading builtins (->, ->>, thread-first, thread-last)
    register_threading_builtins(m_env);

    // 22. Polymorphic access builtins (get, set!)
    register_polymorphic_builtins(m_env);

    // 23. Texture builtins (define-texture, texture?, texture-width, ...)
    register_texture_builtins(m_env);

    // 24. Texture-map builtin (texture-map shape :source tex ...)
    register_texture_map_builtins(m_env);

    // ── Module loading is handled via the evaluator's eval_import/eval_provide
    // special forms — no separate module registration call needed.
}

RenderResult PMLRuntime::execute(const std::string& source, const std::string& filename) {

    // Reset the runtime call stack so that errors from previous executions do
    // not leak into the current one.
    CallStack::instance().clear();

    // Activate this runtime's context for the duration of the call.
    // All singleton-style accessors (current canvas, timeline, styles,
    // palettes) will see this instance's state.
    PMLContextScope ctx_scope(ctx_);

    // Activate the per-runtime arena for all short-lived AST nodes and
    // evaluation temporaries.  The arena is reset when the scope ends.
    ArenaScope arena_scope(&m_arena);

    // Always set __source_file__ so it never carries a stale value from a
    // prior execution (e.g., a previous execute_file() call). This is used
    // by asset loading, import resolution, and the (source-file) builtin.
    m_env->define("__source_file__", Value(filename.empty() ? "<stdin>" : std::string(filename)));

    // Also update source_dir so that relative asset paths resolve correctly.
    // execute_file() already sets source_dir before calling execute(), but
    // when execute() is called directly (MCP, embedded use) the directory
    // must be derived from the filename, or reset to the working directory
    // for stdin/no-filename.
    if (!filename.empty() && filename != "<stdin>") {
        ctx_.source_dir = std::filesystem::path(filename).parent_path().string();
    } else {
        ctx_.source_dir = std::filesystem::current_path().string();
    }

    // Lexer
    auto tokenResult = Lexer(source, filename).tokenize();
    if (!tokenResult.has_value()) {
        RenderResult r;
        r.success = false;
        r.error = error_to_dict(tokenResult.error());
        return r;
    }
    auto& tokens = *tokenResult;

    // Parser — collect all syntax errors using panic-mode recovery.
    auto parseResult = Parser(std::move(tokens), filename).parse_all();
    if (!parseResult.success()) {
        RenderResult r;
        r.success = false;
        if (parseResult.errors.size() == 1) {
            r.error = error_to_dict(parseResult.errors[0]);
        } else {
            nlohmann::json aggregate;
            aggregate["type"] = "PMLSyntaxError";
            aggregate["message"] =
                std::format("{} parse errors encountered", parseResult.errors.size());
            aggregate["hint"] = "Multiple syntax errors were found; see details below.";
            nlohmann::json details = nlohmann::json::array();
            for (const auto& e : parseResult.errors) {
                details.push_back(error_to_dict(e));
            }
            aggregate["details"] = details;
            r.error = aggregate;
        }
        return r;
    }
    auto ast = std::move(parseResult.expressions);

    // Expander (macro expansion)
    Expander expander(m_env);
    auto expandResult = expander.expand_all(ast);
    if (!expandResult.has_value()) {
        RenderResult r;
        r.success = false;
        r.error = error_to_dict(expandResult.error());
        return r;
    }
    auto expanded = std::move(*expandResult);

    // Evaluate each expression sequentially
    Value last_value;
    bool has_value = false;
    for (const auto& expr : expanded) {
        auto evalResult = eval_to_value(expr, m_env);
        if (!evalResult.has_value()) {
            RenderResult r;
            r.success = false;
            r.error = error_to_dict(evalResult.error());
            return r;
        }
        last_value = std::move(*evalResult);
        has_value = true;
    }

    // Build successful result
    RenderResult r;
    r.success = true;
    r.files = PMLContext::current().output_files;
    r.error = std::nullopt;
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

    // Resolve the source file directory so that (render "x.png") inside
    // this file is interpreted relative to the file's own directory.
    namespace fs = std::filesystem;
    fs::path p(path);
    if (p.is_relative()) {
        p = fs::current_path() / p;
    }
    ctx_.source_dir = p.parent_path().string();

    std::stringstream ss;
    ss << ifs.rdbuf();
    return execute(ss.str(), path);
}

nlohmann::json PMLRuntime::execute_pml(const std::string& source,
                                       const nlohmann::json& options) {
    // Extract optional "filename" from options so __source_file__ is set
    // correctly and error locations reference the right file.
    std::string filename = "<stdin>";
    if (options.is_object()) {
        auto it = options.find("filename");
        if (it != options.end() && it->is_string()) {
            filename = it->get<std::string>();
        }
    }
    auto result = execute(source, filename);
    return result.to_json();
}

ValidationResult PMLRuntime::validate(const std::string& source,
                                      const std::string& filename) {
    ValidationResult result;

    // Step 1: Lex
    auto tokenResult = Lexer(source, filename).tokenize();
    if (!tokenResult.has_value()) {
        result.valid = false;
        result.errors.push_back(error_to_dict(tokenResult.error()));
        return result;
    }

    // Step 2: Parse
    auto parseResult = Parser(std::move(*tokenResult), filename).parse();
    if (!parseResult.has_value()) {
        result.valid = false;
        result.errors.push_back(error_to_dict(parseResult.error()));
        return result;
    }

    // Step 3: Macro expansion
    Expander expander(m_env);
    auto expandResult = expander.expand_all(*parseResult);
    if (!expandResult.has_value()) {
        result.valid = false;
        result.errors.push_back(error_to_dict(expandResult.error()));
    }

    return result;
}

void PMLRuntime::reset() {
    m_env = std::make_shared<Environment>();
    ctx_.reset();
    init_global_env();
}

std::shared_ptr<Environment> PMLRuntime::env() const noexcept {
    return m_env;
}

} // namespace pml
