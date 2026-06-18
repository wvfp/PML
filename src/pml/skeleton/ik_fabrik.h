#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML IK Solver — FABRIK (Forward And Backward Reaching Inverse Kinematics)
// ───────────────────────────────────────────────────────────────────────────────
// Port of pml/skeleton/ik_fabrik.py.
//
// FABRIK is a geometric IK algorithm that operates in position space:
//   1. Backward pass: move end effector to target, adjust chain backward
//   2. Forward pass: fix root, adjust chain forward
//   3. Convert final positions back to joint angles
//
// Angles are in radians, matching the C++ SkeletonInstance.
// ═══════════════════════════════════════════════════════════════════════════════

#include "skeleton.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pml {

/// Solve IK for a skeleton chain using the FABRIK algorithm.
///
/// Finds the chain from root to the named end effector, runs FABRIK
/// backward/forward passes, and converts the resulting world-space
/// positions to clamped joint angles on the skeleton.
///
/// @param skeleton      The skeleton instance to solve (must have a template).
/// @param end_effector  Name of the end effector joint.
/// @param target_x      Target X coordinate in world space.
/// @param target_y      Target Y coordinate in world space.
/// @param max_iterations Maximum number of FABRIK iterations (default: 10).
/// @param tolerance     Convergence threshold in pixels (default: 0.01).
/// @return True if converged within tolerance; false if unreachable or not found.
[[nodiscard]] bool solve_fabrik(
    std::shared_ptr<SkeletonInstance> skeleton,
    const std::string& end_effector,
    double target_x,
    double target_y,
    int max_iterations = 10,
    double tolerance = 0.01);

namespace detail {

/// Normalize a 2D vector. Returns (0, 0) for zero-length vectors.
[[nodiscard]] std::pair<double, double> normalize(double dx, double dy);

/// Convert a chain of world-space positions back to clamped joint angles.
///
/// Iterates from root outward, computing atan2 for each bone segment,
/// normalizing the relative angle to [-pi, pi], then clamping via
/// SkeletonInstance::set_angle.
///
/// @param skeleton     The skeleton whose angles to update.
/// @param positions    World-space positions (index 0 = root, last = end tip).
/// @param end_effector Name of the end effector joint (determines chain length).
void positions_to_angles(
    SkeletonInstance* skeleton,
    const std::vector<std::pair<double, double>>& positions,
    const std::string& end_effector);

}  // namespace detail
}  // namespace pml
