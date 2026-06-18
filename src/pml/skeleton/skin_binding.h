#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Skin Binding — maps GraphicObjects to skeleton joints
//
// Ported from pml/skeleton/__init__.py: _skin_bindings, _bind_skin,
// and pml/animation/timeline.py: _merge_skin_bindings.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skeleton.h"
#include "types.h"
#include "environment.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Skin binding types
// ═══════════════════════════════════════════════════════════════════════════════

struct SkinBinding {
    std::shared_ptr<SkeletonInstance> skeleton;
    std::vector<std::string> joint_names;
};

using SkinBindingsMap = std::unordered_map<uint64_t, std::vector<SkinBinding>>;

// ═══════════════════════════════════════════════════════════════════════════════
// Global registry
// ═══════════════════════════════════════════════════════════════════════════════

SkinBindingsMap& get_skin_bindings();
void clear_skin_bindings();

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: bind-skin
// ═══════════════════════════════════════════════════════════════════════════════

/// (bind-skin <graphic> <instance> <joint-name> ...)
///
/// Bind a graphic object to one or more skeleton joints.
/// Stores graphic->id → (instance, [joint_names]) in the global registry.
Result<Value> builtin_bind_skin(const std::vector<Value>& args, Environment& env);

// ═══════════════════════════════════════════════════════════════════════════════
// Merge into animation modifications
// ═══════════════════════════════════════════════════════════════════════════════

/// Process skeleton skin bindings and merge joint-driven transforms
/// into the per-frame modification map.
///
/// For each graphic bound to a skeleton joint via bind-skin:
///   1. Compute forward kinematics
///   2. Get joint world position
///   3. Compute cumulative angle up to that joint
///   4. Build AffineTransform.translate(jx, jy).compose(rotate(cumulative))
///   5. Store as obj_mods[gid]["transform"] (encoded as two doubles: tx, ty)
///
/// The transform components are stored as numeric Values and applied in
/// _apply_modifications via transform.a/b/c/d/tx/ty.
void merge_skin_bindings(
    std::unordered_map<uint64_t, std::unordered_map<std::string, Value>>& obj_mods);

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Register bind-skin and _every-frame builtins.
void register_skin_binding(std::shared_ptr<Environment> env);

} // namespace pml
