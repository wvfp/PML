#pragma once

// ==========================================================================================================================================================================================================================================═
// PML IK Solver — CCD (Cyclic Coordinate Descent)
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Port of pml/skeleton/ik_ccd.py:
//   - CCD iterates from the end joint toward the root, rotating each joint
//     to minimize the distance between the end effector and the target.
//   - Angles in radians, matching the Python PML implementation.
//
// Builtins:
//   - ik-solve          (dispatches to CCD or FABRIK by :method keyword)
// ==========================================================================================================================================================================================================================================═

#include "skeleton.h"

#include <memory>
#include <string>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// CCD Solver
// ==========================================================================================================================================================================================================================================═

/// Solve IK using Cyclic Coordinate Descent.
///
/// Iterates from the end effector joint back to the root, rotating each joint
/// to minimize the angular error toward the target position. After each joint
/// adjustment, world positions are recomputed via forward kinematics.
///
/// @param skeleton       The skeleton instance to solve (modified in-place).
/// @param end_effector   Name of the end effector joint.
/// @param target_x       Target X coordinate.
/// @param target_y       Target Y coordinate.
/// @param max_iterations Maximum number of full-chain iterations (default 20).
/// @param tolerance      Convergence threshold in pixels (default 0.01).
/// @return True if converged within tolerance, False otherwise.
[[nodiscard]] bool solve_ccd(
    std::shared_ptr<SkeletonInstance> skeleton,
    const std::string& end_effector,
    double target_x,
    double target_y,
    int max_iterations = 20,
    double tolerance = 0.01);

// ==========================================================================================================================================================================================================================================═
// FABRIK forward declaration (T24 — implemented in ik_fabrik.h/.cpp)
// ==========================================================================================================================================================================================================================================═

/// Solve IK using FABRIK (Forward And Backward Reaching Inverse Kinematics).
/// Forward-declared here so register_ik can dispatch to it.
[[nodiscard]] bool solve_fabrik(
    std::shared_ptr<SkeletonInstance> skeleton,
    const std::string& end_effector,
    double target_x,
    double target_y,
    int max_iterations = 10,
    double tolerance = 0.01);

// ==========================================================================================================================================================================================================================================═
// Builtin registration
// ==========================================================================================================================================================================================================================================═

/// Register the `ik-solve` builtin into the given environment.
///
/// Syntax:
///   (ik-solve <instance> <end-effector> <target-x> <target-y>
///             [:method 'fabrik|'ccd]
///             [:iterations <int>]
///             [:tolerance <float>])
///
/// Dispatches to solve_fabrik or solve_ccd based on the :method keyword.
void register_ik(std::shared_ptr<Environment> env);

}  // namespace pml
