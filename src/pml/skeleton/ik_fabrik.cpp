// ==========================================================================================================================================================================================================================================═
// PML IK Solver — FABRIK Implementation
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Port of pml/skeleton/ik_fabrik.py.
//
// FABRIK operates in position space:
//   1. Backward pass: place end effector at target, then work backward toward
//      root, keeping each bone segment at its fixed length.
//   2. Forward pass: fix root at its original position, then work forward,
//      again respecting bone lengths.
//   3. Convert resulting world-space positions to clamped joint angles.
//
// Edge cases handled:
//   - Unreachable target: stretch chain toward target, return false.
//   - Single-joint chain: always treated as converged (only 1 DOF).
//   - Zero bone segments: no-op, returns true.
//   - Unknown end effector: returns false.
// ==========================================================================================================================================================================================================================================═

#include "ik_fabrik.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

namespace pml {

bool solve_fabrik(
    std::shared_ptr<SkeletonInstance> skeleton,
    const std::string& end_effector,
    double target_x,
    double target_y,
    int max_iterations,
    double tolerance)
{
    // ---- Validate inputs ------------------------------------------------------------------------------------------------------------
    if (!skeleton || !skeleton->tmpl) {
        return false;
    }

    int end_idx = skeleton->tmpl->joint_index(end_effector);
    if (end_idx < 0) {
        return false;
    }

    // ---- Get chain data ------------------------------------------------------------------------------------------------------------─
    // positions: [joint_0, joint_1, ..., joint_n, tip_of_joint_n]
    //            size = end_idx + 2
    // lengths:   [bone_0_len, bone_1_len, ..., bone_n_len]
    //            size = end_idx + 1 = n
    auto positions = skeleton->chain_positions(end_idx);
    auto lengths = skeleton->bone_lengths(end_idx);
    int n = static_cast<int>(lengths.size());  // number of bone segments

    if (n == 0) {
        return true;
    }

    const std::pair<double, double> root_pos(skeleton->root_x, skeleton->root_y);
    const std::pair<double, double> target(target_x, target_y);

    // Total reach of the chain
    double total_length = 0.0;
    for (double len : lengths) {
        total_length += len;
    }

    // Distance from root to target
    double dist_to_target = std::hypot(
        target.first - root_pos.first,
        target.second - root_pos.second);

    // ---- Unreachable target: stretch chain toward target --------------------------------------------
    // When the target is beyond total reach, simply align all bones toward the
    // target. The end effector will point in the right direction but won't reach.
    if (dist_to_target > total_length) {
        for (int i = 0; i < n; ++i) {
            auto dir = detail::normalize(
                target.first - positions[i].first,
                target.second - positions[i].second);
            positions[i + 1] = {
                positions[i].first + dir.first * lengths[i],
                positions[i].second + dir.second * lengths[i]};
        }
        detail::positions_to_angles(skeleton.get(), positions, end_effector);
        return false;
    }

    // ---- FABRIK iterations --------------------------------------------------------------------------------------------------------
    for (int iter = 0; iter < max_iterations; ++iter) {
        // Convergence check: is end effector close enough to target?
        const auto& end_pos = positions[n];  // positions has n+1 elements
        double dist = std::hypot(
            end_pos.first - target.first,
            end_pos.second - target.second);
        if (dist < tolerance) {
            detail::positions_to_angles(skeleton.get(), positions, end_effector);
            return true;
        }

        // ---- Backward pass --------------------------------------------------------------------------------------------------------
        // Place end effector at target, then work backward toward root.
        positions[n] = target;
        for (int i = n - 1; i >= 0; --i) {
            auto dir = detail::normalize(
                positions[i].first - positions[i + 1].first,
                positions[i].second - positions[i + 1].second);
            positions[i] = {
                positions[i + 1].first + dir.first * lengths[i],
                positions[i + 1].second + dir.second * lengths[i]};
        }

        // ---- Forward pass --------------------------------------------------------------------------------------------------------─
        // Fix root at original position, then work forward toward end effector.
        positions[0] = root_pos;
        for (int i = 0; i < n; ++i) {
            auto dir = detail::normalize(
                positions[i + 1].first - positions[i].first,
                positions[i + 1].second - positions[i].second);
            positions[i + 1] = {
                positions[i].first + dir.first * lengths[i],
                positions[i].second + dir.second * lengths[i]};
        }
    }

    // ---- After max iterations: final check ------------------------------------------------------------------------
    const auto& end_pos = positions[n];
    double dist = std::hypot(
        end_pos.first - target.first,
        end_pos.second - target.second);
    bool converged = dist < tolerance;

    // Single-joint chain: after one backward+forward pass the bone is already
    // pointing in the best possible direction. The end effector can never get
    // closer to target than |length - dist(root, target)|, so treat as converged.
    if (n == 1) {
        converged = true;
    }

    detail::positions_to_angles(skeleton.get(), positions, end_effector);
    return converged;
}

namespace detail {

std::pair<double, double> normalize(double dx, double dy) {
    double length = std::hypot(dx, dy);
    if (length < 1e-10) {
        return {0.0, 0.0};
    }
    return {dx / length, dy / length};
}

void positions_to_angles(
    SkeletonInstance* skeleton,
    const std::vector<std::pair<double, double>>& positions,
    const std::string& end_effector)
{
    if (!skeleton || !skeleton->tmpl) {
        return;
    }

    int end_idx = skeleton->tmpl->joint_index(end_effector);
    if (end_idx < 0) {
        return;
    }

    double cumulative_angle = 0.0;
    constexpr double pi = std::numbers::pi_v<double>;

    for (int i = 0; i <= end_idx; ++i) {
        // Direction from this joint to next position (the bone vector)
        double dx = positions[i + 1].first - positions[i].first;
        double dy = positions[i + 1].second - positions[i].second;

        // Absolute angle of this bone segment in world space
        double bone_angle = std::atan2(dy, dx);

        // The joint's relative angle is the difference between the bone's
        // absolute angle and the cumulative parent angle
        double joint_angle = bone_angle - cumulative_angle;

        // Normalize to [-pi, pi]
        while (joint_angle > pi) {
            joint_angle -= 2.0 * pi;
        }
        while (joint_angle < -pi) {
            joint_angle += 2.0 * pi;
        }

        // Clamp to joint constraints via set_angle, which reads min/max from
        // the SkeletonTemplate and clamps the value before storing
        skeleton->set_angle(i, joint_angle);

        // Update cumulative angle with the *clamped* value so subsequent
        // joints in the chain compute correct relative angles
        cumulative_angle += skeleton->angles[i];
    }
}

}  // namespace detail
}  // namespace pml
