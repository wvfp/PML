// ═══════════════════════════════════════════════════════════════════════════════
// PML Skin Binding — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "skin_binding.h"

#include "error.h"
#include "transform.h"
#include "objects.h"

#include <cmath>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Global registry
// ═══════════════════════════════════════════════════════════════════════════════

SkinBindingsMap& get_skin_bindings() {
    static SkinBindingsMap bindings;
    return bindings;
}

void clear_skin_bindings() {
    get_skin_bindings().clear();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: bind-skin
// ═══════════════════════════════════════════════════════════════════════════════

Result<Value> builtin_bind_skin(const std::vector<Value>& args, Environment&) {
    if (args.size() < 3) {
        return std::unexpected(arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
    }

    const auto* graphic_ptr = std::get_if<std::shared_ptr<GraphicObject>>(&args[0]);
    if (!graphic_ptr || !*graphic_ptr) {
        return std::unexpected(type_error(
            "bind-skin: first argument must be a GraphicObject"));
    }

    const auto* instance_ptr = std::get_if<std::shared_ptr<SkeletonInstance>>(&args[1]);
    if (!instance_ptr || !*instance_ptr) {
        return std::unexpected(type_error(
            "bind-skin: second argument must be a SkeletonInstance"));
    }

    std::vector<std::string> joint_names;
    for (size_t i = 2; i < args.size(); ++i) {
        if (const auto* sym = std::get_if<Symbol>(&args[i])) {
            joint_names.push_back(sym->name);
        } else if (const auto* s = std::get_if<std::string>(&args[i])) {
            joint_names.push_back(*s);
        } else {
            return std::unexpected(type_error(
                "bind-skin: joint names must be symbols or strings"));
        }
    }

    uint64_t gid = (*graphic_ptr)->id;
    get_skin_bindings()[gid].push_back(SkinBinding{*instance_ptr, std::move(joint_names)});

    return make_nil_value();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Merge skin bindings into animation modifications
// ═══════════════════════════════════════════════════════════════════════════════

void merge_skin_bindings(
    std::unordered_map<uint64_t, std::unordered_map<std::string, double>>& obj_mods)
{
    auto& bindings = get_skin_bindings();
    if (bindings.empty()) return;

    for (auto& [gid, binding_list] : bindings) {
        for (auto& binding : binding_list) {
            if (binding.joint_names.empty()) continue;

            const auto& joint_name = binding.joint_names[0];
            auto& skel = binding.skeleton;
            if (!skel || !skel->tmpl) continue;

            int idx = skel->tmpl->joint_index(joint_name);
            if (idx < 0) continue;

            auto positions = skel->forward_kinematics();
            if (idx >= static_cast<int>(positions.size())) continue;

            auto [jx, jy] = positions[idx];

            double cumulative = 0.0;
            for (int i = 0; i <= idx && i < static_cast<int>(skel->angles.size()); ++i) {
                cumulative += skel->angles[i];
            }

            // Build joint transform: translate(jx,jy) . rotate(cumulative)
            auto t_translate = AffineTransform::translate(jx, jy);
            auto t_rotate = AffineTransform::rotate(cumulative);
            auto joint_xform = t_translate.compose(t_rotate);

            // Encode as individual transform components
            auto& mods = obj_mods[gid];
            mods["transform.a"] = joint_xform.a;
            mods["transform.b"] = joint_xform.b;
            mods["transform.c"] = joint_xform.c;
            mods["transform.d"] = joint_xform.d;
            mods["transform.tx"] = joint_xform.e;
            mods["transform.ty"] = joint_xform.f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_skin_binding(std::shared_ptr<Environment> env) {
    auto proc = std::make_shared<BuiltinProcedure>("bind-skin", builtin_bind_skin);
    env->define("bind-skin", Value(proc));
}

} // namespace pml
