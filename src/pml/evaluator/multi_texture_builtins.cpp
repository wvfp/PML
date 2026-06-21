// ═══════════════════════════════════════════════════════════════════════════════
// PML Multi-Texture Shader Builtins — bind SkImages to `uniform shader` slots
// ═══════════════════════════════════════════════════════════════════════════════
//
// (bind-textures shader-handle :textures '((slot-name graphic-obj) ...))
//   - Renders each GraphicObject to an SkImage and binds it as a child shader
//   - Matches slot names to SkSL `uniform shader <slot_name>` declarations
//   - Returns a new shader handle with children bound
//
// Requires an active backend that supports bind_textures_to_shader (e.g. Skia).
// ═══════════════════════════════════════════════════════════════════════════════

#include "multi_texture_builtins.h"

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/graphics/graphic_object.h"
#include "types.h"

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pml {

using pml::kwargs::value_to_opt_string;

void register_multi_texture_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn,
                   bool accepts_kwargs = true) {
        auto proc = std::make_shared<BuiltinProcedure>(
            name, std::move(fn), accepts_kwargs);
        env->define(name, Value(proc));
    };

    // ── (bind-textures shader-handle :textures '((slot-name graphic-obj) ...)) ──
    //
    // Bind texture GraphicObjects to a compiled shader's `uniform shader` slots.
    //
    // Signature:
    //   (bind-textures shader-handle :textures tex-list)
    //
    //   shader-handle  — compiled shader handle (returned by `shader`)
    //   :textures      — list of (slot-name graphic-obj) pairs
    //     slot-name    — string or symbol matching `uniform shader <slot_name>`
    //     graphic-obj  — PML GraphicObject value (already evaluated)
    //
    // Returns:
    //   New shader handle with textures bound as child shaders.
    //
    // Example:
    //   (let ((s (shader "vec4 main(vec2 p) { return sample(tex, p); }
    //                       uniform shader tex;")))
    //     (bind-textures s :textures (list (list "tex" (rect 0 0 64 64 :fill "red")))))
    def("bind-textures", [](const std::vector<Value>& args,
                             Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(arity_error(
                SourceLocation{}, 1, static_cast<int>(args.size())));
        }

        // 1. Extract shader handle from first positional arg
        uint64_t shader_handle = 0;
        if (args[0].is_int()) {
            shader_handle = static_cast<uint64_t>(args[0].int_val());
        } else if (args[0].is_double()) {
            shader_handle = static_cast<uint64_t>(args[0].double_val());
        } else {
            return std::unexpected(type_error(
                "bind-textures: first argument must be a shader handle (integer), got " +
                value_to_string(args[0])));
        }
        if (shader_handle == 0) {
            return std::unexpected(type_error(
                "bind-textures: invalid shader handle (0)"));
        }

        // 2. Parse kwargs starting at position 1
        auto kwargs = pml::kwargs::parse_kwargs(args, 1);
        auto tex_it = kwargs.find("textures");
        if (tex_it == kwargs.end()) {
            return std::unexpected(type_error(
                "bind-textures: :textures keyword argument is required"));
        }

        // 3. Parse textures list
        const auto* tex_list = tex_it->second.as_list();
        if (!tex_list || !*tex_list) {
            return std::unexpected(type_error(
                "bind-textures: :textures must be a list of (slot-name graphic) pairs"));
        }

        std::vector<std::pair<std::string, Value>> textures;
        textures.reserve((*tex_list)->elements.size());

        for (size_t i = 0; i < (*tex_list)->elements.size(); ++i) {
            const auto& entry = (*tex_list)->elements[i];
            const auto* entry_list = entry.as_list();
            if (!entry_list || !*entry_list) {
                return std::unexpected(type_error(
                    std::format("bind-textures: entry {} in :textures is not a list", i)));
            }
            const auto& elems = (*entry_list)->elements;
            if (elems.size() < 2) {
                return std::unexpected(type_error(
                    std::format("bind-textures: entry {} in :textures needs at least 2 elements "
                                "(slot-name graphic-obj)", i)));
            }

            // 3a. Slot name (string or symbol)
            auto slot_name = pml::kwargs::value_to_opt_string(elems[0]);
            if (!slot_name) {
                return std::unexpected(type_error(
                    std::format("bind-textures: entry {} in :textures first element must be "
                                "a string or symbol (slot name), got {}",
                                i, value_to_string(elems[0]))));
            }

            // 3b. GraphicObject value
            const auto* go = elems[1].as_graphic_object();
            if (!go || !*go) {
                return std::unexpected(type_error(
                    std::format("bind-textures: entry {} in :textures second element must be "
                                "a GraphicObject, got {}",
                                i, value_to_string(elems[1]))));
            }

            textures.emplace_back(std::move(*slot_name), elems[1]);
        }

        // 4. Delegate to the active render backend
        RenderBackend& backend = BackendRegistry::instance().active();
        auto new_handle = backend.bind_textures_to_shader(shader_handle, textures);
        if (!new_handle) return std::unexpected(std::move(new_handle.error()));

        return Value(static_cast<int64_t>(*new_handle));
    });
}

}  // namespace pml
