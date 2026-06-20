// ═══════════════════════════════════════════════════════════════════════════════
// PML Skeleton System — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// Port of pml/skeleton/skeleton.py + pml/skeleton/__init__.py + evaluator.py
// ═══════════════════════════════════════════════════════════════════════════════

#include "skeleton.h"

#include "evaluator.h"
#include "error.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonTemplate
// ═══════════════════════════════════════════════════════════════════════════════

int SkeletonTemplate::joint_index(const std::string& name) const {
    for (size_t i = 0; i < joints.size(); ++i) {
        if (joints[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — clamping
// ═══════════════════════════════════════════════════════════════════════════════

double SkeletonInstance::clamp_angle(int index, double angle) const noexcept {
    if (!tmpl || index < 0 || static_cast<size_t>(index) >= tmpl->joints.size()) {
        return angle;
    }
    const auto& joint = tmpl->joints[static_cast<size_t>(index)];
    if (joint.min_angle.has_value()) {
        angle = std::max(*joint.min_angle, angle);
    }
    if (joint.max_angle.has_value()) {
        angle = std::min(*joint.max_angle, angle);
    }
    return angle;
}

void SkeletonInstance::set_angle(int index, double value) {
    if (index >= 0 && static_cast<size_t>(index) < angles.size()) {
        angles[static_cast<size_t>(index)] = clamp_angle(index, value);
    }
}

void SkeletonInstance::set_angle(const std::string& name, double value) {
    int idx = joint_index(name);
    if (idx >= 0) {
        set_angle(idx, value);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — forward kinematics
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<std::pair<double, double>> SkeletonInstance::forward_kinematics() const {
    std::vector<std::pair<double, double>> positions;
    if (!tmpl) return positions;

    double cx = root_x;
    double cy = root_y;
    double cumulative_angle = 0.0;

    for (size_t i = 0; i < tmpl->joints.size(); ++i) {
        const auto& joint = tmpl->joints[i];

        // Apply offset from parent (in parent's local frame)
        double dx = joint.dx;
        double dy = joint.dy;
        if (dx != 0.0 || dy != 0.0) {
            double cos_a = std::cos(cumulative_angle);
            double sin_a = std::sin(cumulative_angle);
            cx += dx * cos_a - dy * sin_a;
            cy += dx * sin_a + dy * cos_a;
        }

        // Record this joint's world position
        positions.emplace_back(cx, cy);

        // Cumulative angle includes this joint's angle
        cumulative_angle += angles[i];

        // Advance to end of bone (start of next joint)
        cx += joint.length * std::cos(cumulative_angle);
        cy += joint.length * std::sin(cumulative_angle);
    }

    return positions;
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — world position queries
// ═══════════════════════════════════════════════════════════════════════════════

std::pair<double, double> SkeletonInstance::get_joint_world_pos(int index) const {
    auto positions = forward_kinematics();
    if (index >= 0 && static_cast<size_t>(index) < positions.size()) {
        return positions[static_cast<size_t>(index)];
    }
    return {root_x, root_y};
}

std::pair<double, double> SkeletonInstance::get_joint_world_pos(const std::string& name) const {
    int idx = joint_index(name);
    return get_joint_world_pos(idx);
}

std::pair<double, double> SkeletonInstance::end_effector_position() const {
    auto positions = forward_kinematics();
    if (!tmpl || positions.empty()) {
        return {root_x, root_y};
    }

    // The end effector is the tip of the last bone
    const auto& last_joint = tmpl->joints.back();
    const auto& last_pos = positions.back();

    // Cumulative angle through ALL joints determines the bone direction
    double cumulative = 0.0;
    for (double a : angles) cumulative += a;

    double ex = last_pos.first + last_joint.length * std::cos(cumulative);
    double ey = last_pos.second + last_joint.length * std::sin(cumulative);
    return {ex, ey};
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — chain positions
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<std::pair<double, double>> SkeletonInstance::chain_positions(int end_index) const {
    if (!tmpl) return {};

    auto positions = forward_kinematics();

    // Get chain up to and including end effector joint
    size_t end = static_cast<size_t>(std::max(0, end_index));
    if (end >= positions.size()) end = positions.size() - 1;

    std::vector<std::pair<double, double>> chain;
    chain.reserve(end + 2);
    for (size_t i = 0; i <= end; ++i) {
        chain.push_back(positions[i]);
    }

    // Add the end effector tip (end of last bone in chain)
    double cumulative = 0.0;
    for (size_t i = 0; i <= end && i < angles.size(); ++i) {
        cumulative += angles[i];
    }
    double tip_x = chain.back().first + tmpl->joints[end].length * std::cos(cumulative);
    double tip_y = chain.back().second + tmpl->joints[end].length * std::sin(cumulative);
    chain.emplace_back(tip_x, tip_y);

    return chain;
}

std::vector<std::pair<double, double>> SkeletonInstance::chain_positions(const std::string& end_name) const {
    int idx = joint_index(end_name);
    return chain_positions(idx);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — bone lengths
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<double> SkeletonInstance::bone_lengths(int end_index) const {
    std::vector<double> lengths;
    if (!tmpl) return lengths;

    size_t end = static_cast<size_t>(std::max(0, end_index));
    if (end >= tmpl->joints.size()) end = tmpl->joints.size() - 1;

    lengths.reserve(end + 1);
    for (size_t i = 0; i <= end; ++i) {
        lengths.push_back(tmpl->joints[i].length);
    }
    return lengths;
}

std::vector<double> SkeletonInstance::bone_lengths(const std::string& end_name) const {
    int idx = joint_index(end_name);
    return bone_lengths(idx);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SkeletonInstance — private helpers
// ═══════════════════════════════════════════════════════════════════════════════

int SkeletonInstance::joint_index(const std::string& name) const {
    if (!tmpl) return -1;
    return tmpl->joint_index(name);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Special form: defskeleton
// ═══════════════════════════════════════════════════════════════════════════════
//
// Syntax:
//   (defskeleton <name> (<root-x> <root-y>)
//     (joint <joint-name> :pos (<dx> <dy>) :length <len> :angle <angle>
//            [:min <min>] [:max <max>]) ...)
//
// Matches pml/skeleton/evaluator.py:_eval_defskeleton

Result<Value> eval_defskeleton(
    const std::vector<Expr>& expr, std::shared_ptr<Environment> env)
{
    // expr[0] = 'defskeleton', expr[1] = name, expr[2] = root params, expr[3:] = joint clauses
    if (expr.size() < 4) {
        return std::unexpected(arity_error(
            SourceLocation{}, 4, static_cast<int>(expr.size())));
    }

    // 1. Parse template name
    const Expr& name_expr = expr[1];
    const Symbol* name_sym = std::get_if<Symbol>(&name_expr);
    if (!name_sym) {
        return std::unexpected(type_error(
            SourceLocation{}, "defskeleton: name must be a symbol"));
    }
    std::string tmpl_name = name_sym->name;

    // 2. Parse root parameter names
    const Expr& root_params_expr = expr[2];
    const ArenaExprVector* root_list = get_list(root_params_expr);
    if (!root_list || root_list->size() != 2) {
        return std::unexpected(type_error(
            SourceLocation{}, "defskeleton: root params must be (param-x param-y)"));
    }

    std::vector<std::string> root_params;
    for (const auto& p : *root_list) {
        const Symbol* p_sym = std::get_if<Symbol>(&p);
        if (!p_sym) {
            return std::unexpected(type_error(
                SourceLocation{}, "defskeleton: root param must be a symbol"));
        }
        root_params.push_back(p_sym->name);
    }

    // 3. Parse joint clauses
    std::vector<Joint> joints;
    for (size_t i = 3; i < expr.size(); ++i) {
        const ArenaExprVector* joint_expr = get_list(expr[i]);
        if (!joint_expr || joint_expr->size() < 2) {
            return std::unexpected(type_error(
                SourceLocation{}, "defskeleton: each joint must be (joint name :key val ...)"));
        }

        // Check that first element is 'joint' symbol
        const Symbol* head_sym = std::get_if<Symbol>(&(*joint_expr)[0]);
        if (!head_sym || head_sym->name != "joint") {
            return std::unexpected(type_error(
                SourceLocation{}, "defskeleton: expected 'joint' at head of clause"));
        }

        // Joint name
        const Symbol* jname_sym = std::get_if<Symbol>(&(*joint_expr)[1]);
        if (!jname_sym) {
            return std::unexpected(type_error(
                SourceLocation{}, "defskeleton: joint name must be a symbol"));
        }

        Joint j;
        j.name = jname_sym->name;

        // Parse keyword arguments
        size_t kw_idx = 2;
        while (kw_idx < joint_expr->size()) {
            const Keyword* kw = std::get_if<Keyword>(&(*joint_expr)[kw_idx]);
            if (!kw) {
                kw_idx += 1;
                continue;
            }

            if (kw_idx + 1 >= joint_expr->size()) {
                return std::unexpected(type_error(
                    SourceLocation{}, "defskeleton: keyword " + kw->name + " has no value"));
            }

            const Expr& val = (*joint_expr)[kw_idx + 1];

            if (kw->name == "pos") {
                const ArenaExprVector* pos_list = get_list(val);
                if (!pos_list || pos_list->size() != 2) {
                    return std::unexpected(type_error(
                        SourceLocation{}, "defskeleton: :pos must be (dx dy)"));
                }
                if (const auto* dx = std::get_if<int64_t>(&(*pos_list)[0])) {
                    j.dx = static_cast<double>(*dx);
                } else if (const auto* dx = std::get_if<double>(&(*pos_list)[0])) {
                    j.dx = *dx;
                }
                if (const auto* dy = std::get_if<int64_t>(&(*pos_list)[1])) {
                    j.dy = static_cast<double>(*dy);
                } else if (const auto* dy = std::get_if<double>(&(*pos_list)[1])) {
                    j.dy = *dy;
                }
            } else if (kw->name == "length") {
                if (const auto* l = std::get_if<int64_t>(&val)) {
                    j.length = static_cast<double>(*l);
                } else if (const auto* l = std::get_if<double>(&val)) {
                    j.length = *l;
                }
            } else if (kw->name == "angle") {
                if (const auto* a = std::get_if<int64_t>(&val)) {
                    j.angle = static_cast<double>(*a);
                } else if (const auto* a = std::get_if<double>(&val)) {
                    j.angle = *a;
                }
            } else if (kw->name == "min") {
                if (const auto* m = std::get_if<int64_t>(&val)) {
                    j.min_angle = static_cast<double>(*m);
                } else if (const auto* m = std::get_if<double>(&val)) {
                    j.min_angle = *m;
                }
            } else if (kw->name == "max") {
                if (const auto* m = std::get_if<int64_t>(&val)) {
                    j.max_angle = static_cast<double>(*m);
                } else if (const auto* m = std::get_if<double>(&val)) {
                    j.max_angle = *m;
                }
            }

            kw_idx += 2;
        }

        joints.push_back(std::move(j));
    }

    // 4. Create template and bind in environment
    auto tmpl = std::make_shared<SkeletonTemplate>(tmpl_name, std::move(joints), std::move(root_params));
    env->define(tmpl_name, Value(std::move(tmpl)));
    return make_nil_value();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: instantiate-skeleton
// ═══════════════════════════════════════════════════════════════════════════════
//
// Syntax: (instantiate-skeleton <template> <x> <y> [:name <string>])

static Result<Value> builtin_instantiate_skeleton(
    const std::vector<Value>& args, Environment& /*env*/)
{
    if (args.size() < 3) {
        return std::unexpected(arity_error(
            SourceLocation{}, 3, static_cast<int>(args.size())));
    }

    // First arg: template
    const auto* tmpl = args[0].as_skeleton_template();
    if (!tmpl || !*tmpl) {
        return std::unexpected(type_error(
            SourceLocation{}, "instantiate-skeleton: expected SkeletonTemplate, got " +
                value_to_string(args[0])));
    }

    // Second and third args: root x, y
    double rx = 0.0, ry = 0.0;
    if (args[1].is_int()) {
        rx = static_cast<double>(args[1].int_val());
    } else if (args[1].is_double()) {
        rx = args[1].double_val();
    } else {
        return std::unexpected(type_error(
            SourceLocation{}, "instantiate-skeleton: x must be a number"));
    }
    if (args[2].is_int()) {
        ry = static_cast<double>(args[2].int_val());
    } else if (args[2].is_double()) {
        ry = args[2].double_val();
    } else {
        return std::unexpected(type_error(
            SourceLocation{}, "instantiate-skeleton: y must be a number"));
    }

    auto instance = std::make_shared<SkeletonInstance>(*tmpl, rx, ry);
    return Value(std::move(instance));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin: joint-position
// ═══════════════════════════════════════════════════════════════════════════════
//
// Syntax: (joint-position <instance> <joint-name>)

static Result<Value> builtin_joint_position(
    const std::vector<Value>& args, Environment& /*env*/)
{
    if (args.size() < 2) {
        return std::unexpected(arity_error(
            SourceLocation{}, 2, static_cast<int>(args.size())));
    }

    // First arg: SkeletonInstance
    const auto* instance = args[0].as_skeleton_instance();
    if (!instance || !*instance) {
        return std::unexpected(type_error(
            SourceLocation{}, "joint-position: expected SkeletonInstance, got " +
                value_to_string(args[0])));
    }

    // Second arg: joint name
    std::string joint_name;
    if (const auto* sym = args[1].as_symbol()) {
        joint_name = sym->name;
    } else if (const auto* s = args[1].as_string()) {
        joint_name = *s;
    } else if (const auto* kw = args[1].as_keyword()) {
        joint_name = kw->name;
    } else {
        return std::unexpected(type_error(
            SourceLocation{}, "joint-position: joint name must be a string or symbol"));
    }

    auto pos = (*instance)->get_joint_world_pos(joint_name);
    auto result_list = std::make_shared<ValueList>();
    result_list->elements.push_back(Value(pos.first));
    result_list->elements.push_back(Value(pos.second));
    return Value(std::move(result_list));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_skeleton(std::shared_ptr<Environment> env)
{
    if (!env) return;

    // Register defskeleton as a special form
    register_special_form("defskeleton", eval_defskeleton);

    // Register regular builtins
    env->define("instantiate-skeleton",
        Value(std::make_shared<BuiltinProcedure>(
            "instantiate-skeleton", builtin_instantiate_skeleton, false)));

    env->define("joint-position",
        Value(std::make_shared<BuiltinProcedure>(
            "joint-position", builtin_joint_position, false)));
}

}  // namespace pml
