#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Skeleton System — Joint / SkeletonTemplate / SkeletonInstance
// ───────────────────────────────────────────────────────────────────────────────
// Port of pml/skeleton/skeleton.py:
//   - Joint: a single joint definition (name, offset, length, angle, constraints)
//   - SkeletonTemplate: a skeleton blueprint (name, root params, joint chain)
//   - SkeletonInstance: a live skeleton with mutable angles, forward kinematics,
//     chain positions, bone lengths, and angle clamping.
//
// Builtins:
//   - defskeleton          (special form, registered via register_skeleton)
//   - instantiate-skeleton (regular builtin)
//   - joint-position       (regular builtin)
//
// Angles are in radians, matching the Python PML implementation.
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"
#include "evaluator.h"
#include "types.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Joint — single joint definition
// ═══════════════════════════════════════════════════════════════════════════════

/// A single joint definition within a skeleton template.
/// Matches Python pml.skeleton.skeleton.Joint (frozen dataclass).
struct Joint {
    std::string name;                    ///< Joint identifier (e.g. "shoulder")
    double dx = 0.0;                     ///< Offset X from parent joint
    double dy = 0.0;                     ///< Offset Y from parent joint
    double length = 0.0;                 ///< Bone segment length from this joint
    double angle = 0.0;                  ///< Initial angle in radians
    std::optional<double> min_angle;     ///< Lower bound constraint (radians)
    std::optional<double> max_angle;     ///< Upper bound constraint (radians)
};

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonTemplate — skeleton blueprint
// ═══════════════════════════════════════════════════════════════════════════════

/// A skeleton blueprint — defines joint topology and initial pose.
/// Matches Python pml.skeleton.skeleton.SkeletonTemplate (frozen dataclass).
struct SkeletonTemplate {
    std::string name;                     ///< Template name
    std::vector<std::string> root_params; ///< Root position param names ("x", "y")
    std::vector<Joint> joints;            ///< Ordered joint chain (parent to child)

    SkeletonTemplate() = default;
    SkeletonTemplate(
        std::string name_,
        std::vector<Joint> joints_,
        std::vector<std::string> root_params_ = {"x", "y"})
        : name(std::move(name_))
        , root_params(std::move(root_params_))
        , joints(std::move(joints_)) {}

    /// Return the index of a joint by name, or -1 if not found.
    [[nodiscard]] int joint_index(const std::string& name) const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — live skeleton with mutable angles
// ═══════════════════════════════════════════════════════════════════════════════

/// A live skeleton with mutable joint angles.
/// Created from a SkeletonTemplate via instantiate-skeleton.
/// IK solvers modify angles in-place.
/// Matches Python pml.skeleton.skeleton.SkeletonInstance.
class SkeletonInstance {
public:
    std::shared_ptr<SkeletonTemplate> tmpl;  ///< The skeleton blueprint
    double root_x = 0.0;                     ///< Root X position
    double root_y = 0.0;                     ///< Root Y position
    std::vector<double> angles;              ///< Mutable angles (one per joint, radians)

    SkeletonInstance(
        std::shared_ptr<SkeletonTemplate> t,
        double rx,
        double ry)
        : tmpl(std::move(t))
        , root_x(rx)
        , root_y(ry)
    {
        // Initialize angles from template defaults
        if (tmpl) {
            angles.reserve(tmpl->joints.size());
            for (const auto& j : tmpl->joints) {
                angles.push_back(j.angle);
            }
        }
    }

    // ── Chain positions ────────────────────────────────────────────────────

    /// Return positions from root to end effector, including bone tip.
    /// The returned vector has (end_index + 2) elements:
    /// [joint_0_pos, joint_1_pos, ..., end_effector_joint_pos, end_effector_tip_pos]
    [[nodiscard]] std::vector<std::pair<double, double>> chain_positions(int end_index) const;
    [[nodiscard]] std::vector<std::pair<double, double>> chain_positions(const std::string& end_name) const;

    // ── Bone lengths ───────────────────────────────────────────────────────

    /// Return bone lengths from root to end effector (inclusive).
    [[nodiscard]] std::vector<double> bone_lengths(int end_index) const;
    [[nodiscard]] std::vector<double> bone_lengths(const std::string& end_name) const;

    // ── Angle setting with clamping ────────────────────────────────────────

    /// Apply min/max constraints to a joint angle.
    [[nodiscard]] double clamp_angle(int index, double angle) const noexcept;

    /// Set a joint angle with constraint clamping.
    void set_angle(int index, double value);
    void set_angle(const std::string& name, double value);

    // ── World position queries ─────────────────────────────────────────────

    /// Get a specific joint's world-space position.
    [[nodiscard]] std::pair<double, double> get_joint_world_pos(int index) const;
    [[nodiscard]] std::pair<double, double> get_joint_world_pos(const std::string& name) const;

    /// Get the position at the end of the last bone segment (tip of last joint).
    [[nodiscard]] std::pair<double, double> end_effector_position() const;

    // ── Forward kinematics ─────────────────────────────────────────────────

    /// Compute world-space positions for all joints.
    /// Returns a vector of (x, y) tuples — one per joint.
    /// The position is the START of each bone segment.
    [[nodiscard]] std::vector<std::pair<double, double>> forward_kinematics() const;

private:
    [[nodiscard]] int joint_index(const std::string& name) const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin registration
// ═══════════════════════════════════════════════════════════════════════════════

/// Special form handler for (defskeleton ...).
/// Parses the raw AST to define a SkeletonTemplate in the environment.
[[nodiscard]] Result<Value> eval_defskeleton(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env);

/// Register all skeleton builtins and the defskeleton special form.
void register_skeleton(std::shared_ptr<Environment> env);

}  // namespace pml
