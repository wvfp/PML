// ==========================================================================================================================================================================================================================================═
// PML IK Solver — CCD (Cyclic Coordinate Descent) — Implementation
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Port of pml/skeleton/ik_ccd.py exactly:
//   - CCD iterates from the end joint toward the root, rotating each joint
//     to minimize the angular error toward the target.
//   - Angles are in radians, matching Python PML.
// ==========================================================================================================================================================================================================================================═

#include "ik_ccd.h"

#include "error.h"
#include "evaluator.h"
#include "types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pml {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

/// Parse a numeric Value (int64_t or double) as double.
double to_double_safe(const Value& v) {
    if (v.is_int()) return static_cast<double>(v.int_val());
    if (v.is_double()) return v.double_val();
    return 0.0;
}

/// Parse a Value as a string — accepts Symbol, std::string, or Keyword.
std::string value_to_name(const Value& v) {
    if (const auto* sym = v.as_symbol()) return sym->name;
    if (const auto* s = v.as_string()) return *s;
    if (const auto* kw = v.as_keyword()) return kw->name;
    return value_to_string(v);
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// solve_ccd — Cyclic Coordinate Descent IK solver
// ==========================================================================================================================================================================================================================================═

bool solve_ccd(
    std::shared_ptr<SkeletonInstance> skeleton,
    const std::string& end_effector,
    double target_x,
    double target_y,
    int max_iterations,
    double tolerance)
{
    // Validate skeleton
    if (!skeleton || !skeleton->tmpl) return false;

    int end_idx = skeleton->tmpl->joint_index(end_effector);
    if (end_idx < 0) return false;

    double tx = target_x;
    double ty = target_y;

    for (int iter = 0; iter < max_iterations; ++iter) {
        // ---- Check convergence ------------------------------------------------------------------------------------
        auto [ex, ey] = skeleton->end_effector_position();
        double dist = std::hypot(ex - tx, ey - ty);
        if (dist < tolerance) return true;

        // ---- Iterate from end effector joint back to root ----------------------------─
        for (int joint_idx = end_idx; joint_idx >= 0; --joint_idx) {
            // Compute current world positions for all joints
            auto positions = skeleton->forward_kinematics();
            auto [jx, jy] = positions[static_cast<size_t>(joint_idx)];

            // End effector tip position:
            //   cumulative = sum of angles up to end_idx
            //   tip = position[end_idx] + joint[end_idx].length * (cos(cumulative), sin(cumulative))
            double cumulative = 0.0;
            for (int i = 0; i <= end_idx; ++i) {
                cumulative += skeleton->angles[static_cast<size_t>(i)];
            }
            auto [ejx, ejy] = positions[static_cast<size_t>(end_idx)];
            double tip_x = ejx
                + skeleton->tmpl->joints[static_cast<size_t>(end_idx)].length
                    * std::cos(cumulative);
            double tip_y = ejy
                + skeleton->tmpl->joints[static_cast<size_t>(end_idx)].length
                    * std::sin(cumulative);

            // Vector from joint to current end effector tip
            double v_end_x = tip_x - jx;
            double v_end_y = tip_y - jy;

            // Vector from joint to target
            double v_target_x = tx - jx;
            double v_target_y = ty - jy;

            // Angle between the two vectors
            double angle_end   = std::atan2(v_end_y, v_end_x);
            double angle_target = std::atan2(v_target_y, v_target_x);
            double delta = angle_target - angle_end;

            // Normalize delta to [-pi, pi]
            while (delta > kPi)  delta -= kTwoPi;
            while (delta < -kPi) delta += kTwoPi;

            // Apply rotation (with angle clamping via set_angle)
            double current_angle = skeleton->angles[static_cast<size_t>(joint_idx)];
            skeleton->set_angle(joint_idx, current_angle + delta);
        }
    }

    // ---- Final convergence check --------------------------------------------------------------------------------
    auto [ex, ey] = skeleton->end_effector_position();
    double dist = std::hypot(ex - tx, ey - ty);

    // Single-joint chain: the bone is already pointing in the best
    // possible direction.  Converged.
    if (end_idx == 0) return true;

    return dist < tolerance;
}

// ==========================================================================================================================================================================================================================================═
// register_ik — register the ik-solve builtin
// ==========================================================================================================================================================================================================================================═
//
// Syntax:
//   (ik-solve <instance> <end-effector> <target-x> <target-y>
//             [:method 'fabrik|'ccd]
//             [:iterations <int>]
//             [:tolerance <float>])
//
// Dispatches to solve_fabrik or solve_ccd based on :method.

void register_ik(std::shared_ptr<Environment> env)
{
    if (!env) return;

    auto proc = std::make_shared<BuiltinProcedure>(
        "ik-solve",
        [](const std::vector<Value>& args, Environment&) -> Result<Value>
        {
            // Require at least 4 positional args
            if (args.size() < 4) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 4, static_cast<int>(args.size())));
            }

            // ---- Parse positional arguments --------------------------------------------------------─
            // args[0] = SkeletonInstance
            const auto* instance = args[0].as_skeleton_instance();
            if (!instance || !*instance) {
                return std::unexpected(type_error(
                    "ik-solve: expected SkeletonInstance, got " +
                    value_to_string(args[0])));
            }

            // args[1] = end effector name (Symbol or string)
            std::string ee_name = value_to_name(args[1]);

            // args[2] = target-x
            if (!is_number(args[2])) {
                return std::unexpected(type_error(
                    "ik-solve: target-x must be a number"));
            }
            double tx = to_double_safe(args[2]);

            // args[3] = target-y
            if (!is_number(args[3])) {
                return std::unexpected(type_error(
                    "ik-solve: target-y must be a number"));
            }
            double ty = to_double_safe(args[3]);

            // ---- Parse keyword arguments (flat list after positional) ----─
            //   :method 'fabrik, :iterations 20, :tolerance 0.01
            std::string method = "fabrik";
            int iterations = 20;
            double tolerance = 0.01;

            for (size_t i = 4; i + 1 < args.size(); i += 2) {
                const auto* kw = args[i].as_keyword();
                if (!kw) {
                    return std::unexpected(type_error(
                        "ik-solve: expected keyword argument at position " +
                        std::to_string(i)));
                }

                const Value& kw_val = args[i + 1];

                if (kw->name == "method") {
                    method = value_to_name(kw_val);
                } else if (kw->name == "iterations") {
                    if (!is_number(kw_val)) {
                        return std::unexpected(type_error(
                            "ik-solve: :iterations must be a number"));
                    }
                    iterations = static_cast<int>(to_double_safe(kw_val));
                } else if (kw->name == "tolerance") {
                    if (!is_number(kw_val)) {
                        return std::unexpected(type_error(
                            "ik-solve: :tolerance must be a number"));
                    }
                    tolerance = to_double_safe(kw_val);
                }
                // Unknown keywords are silently ignored (matching Python)
            }

            // ---- Dispatch to solver ------------------------------------------------------------------------─
            bool converged = false;

            if (method == "fabrik") {
                converged = solve_fabrik(
                    *instance, ee_name, tx, ty, iterations, tolerance);
            } else if (method == "ccd") {
                converged = solve_ccd(
                    *instance, ee_name, tx, ty, iterations, tolerance);
            } else {
                return std::unexpected(type_error(
                    "ik-solve: unknown method '" + method +
                    "', use 'fabrik' or 'ccd'"));
            }

            return Value(converged);
        },
        true  // accepts_kwargs = true
    );

    env->define("ik-solve", Value(std::move(proc)));
}

}  // namespace pml
