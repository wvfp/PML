// ==========================================================================================================================================================================================================================================═
// PML Skeleton & IK — Google Test Suite
// ==========================================================================================================================================================================================================================================═
//
// Tests for:
//   - SkeletonTemplate creation and joint lookup
//   - SkeletonInstance construction, FK, angle clamping, world positions
//   - FABRIK IK solver (convergence, unreachable, single-joint)
//   - CCD IK solver (convergence, unreachable, single-joint)
//   - Skin binding registry (clear, register)
//
// Each test creates fresh skeleton instances — no shared mutable state.
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>
#include "test_helpers.h"
#include "pml/skeleton/skeleton.h"
#include "pml/skeleton/ik_ccd.h"
#include "pml/skeleton/skin_binding.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr double kPi = std::numbers::pi_v<double>;
constexpr double kEpsilon = 1e-6;

/// Helper: build a 3-joint arm template (shoulder → elbow → wrist),
/// all bones length 50, no angle constraints, root at origin.
std::shared_ptr<pml::SkeletonTemplate> make_arm_template() {
    auto tmpl = std::make_shared<pml::SkeletonTemplate>();
    tmpl->name = "arm";
    tmpl->joints.push_back(pml::Joint{"shoulder", 0, 0, 50, 0, std::nullopt, std::nullopt});
    tmpl->joints.push_back(pml::Joint{"elbow",    0, 0, 50, 0, std::nullopt, std::nullopt});
    tmpl->joints.push_back(pml::Joint{"wrist",    0, 0, 50, 0, std::nullopt, std::nullopt});
    return tmpl;
}

/// Helper: build a SkeletonInstance from a template at (100, 100).
std::shared_ptr<pml::SkeletonInstance> make_arm_instance(
    std::shared_ptr<pml::SkeletonTemplate> tmpl = nullptr)
{
    if (!tmpl) tmpl = make_arm_template();
    return std::make_shared<pml::SkeletonInstance>(tmpl, 100.0, 100.0);
}

/// Helper: build a single-joint template.
std::shared_ptr<pml::SkeletonTemplate> make_single_joint_template() {
    auto tmpl = std::make_shared<pml::SkeletonTemplate>();
    tmpl->name = "single";
    tmpl->joints.push_back(pml::Joint{"tip", 0, 0, 100, 0, std::nullopt, std::nullopt});
    return tmpl;
}

}  // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// SkeletonTemplate tests
// ==========================================================================================================================================================================================================================================═

TEST(SkeletonTemplate, CreationDefault) {
    pml::SkeletonTemplate tmpl;
    EXPECT_TRUE(tmpl.name.empty());
    EXPECT_TRUE(tmpl.joints.empty());
    EXPECT_EQ(tmpl.root_params.size(), 0u);
}

TEST(SkeletonTemplate, CreationWithJoints) {
    auto tmpl = make_arm_template();
    EXPECT_EQ(tmpl->name, "arm");
    EXPECT_EQ(tmpl->joints.size(), 3u);
    EXPECT_EQ(tmpl->joints[0].name, "shoulder");
    EXPECT_EQ(tmpl->joints[1].name, "elbow");
    EXPECT_EQ(tmpl->joints[2].name, "wrist");
}

TEST(SkeletonTemplate, JointIndexFound) {
    auto tmpl = make_arm_template();
    EXPECT_EQ(tmpl->joint_index("shoulder"), 0);
    EXPECT_EQ(tmpl->joint_index("elbow"), 1);
    EXPECT_EQ(tmpl->joint_index("wrist"), 2);
}

TEST(SkeletonTemplate, JointIndexNotFound) {
    auto tmpl = make_arm_template();
    EXPECT_EQ(tmpl->joint_index("hip"), -1);
    EXPECT_EQ(tmpl->joint_index(""), -1);
}

TEST(SkeletonTemplate, ConstructorWithArgs) {
    std::vector<pml::Joint> joints = {
        pml::Joint{"a", 0, 0, 10, 0, std::nullopt, std::nullopt},
        pml::Joint{"b", 0, 0, 20, 0, std::nullopt, std::nullopt},
    };
    pml::SkeletonTemplate tmpl("test", std::move(joints));
    EXPECT_EQ(tmpl.name, "test");
    EXPECT_EQ(tmpl.joints.size(), 2u);
    EXPECT_EQ(tmpl.root_params.size(), 2u);
    EXPECT_EQ(tmpl.root_params[0], "x");
    EXPECT_EQ(tmpl.root_params[1], "y");
}

// ==========================================================================================================================================================================================================================================═
// SkeletonInstance tests
// ==========================================================================================================================================================================================================================================═

TEST(SkeletonInstance, Creation) {
    auto tmpl = make_arm_template();
    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 100.0, 100.0);

    EXPECT_DOUBLE_EQ(skel->root_x, 100.0);
    EXPECT_DOUBLE_EQ(skel->root_y, 100.0);
    EXPECT_EQ(skel->angles.size(), 3u);
    EXPECT_EQ(skel->tmpl, tmpl);
}

TEST(SkeletonInstance, AnglesInitializedFromTemplate) {
    auto tmpl = std::make_shared<pml::SkeletonTemplate>();
    tmpl->name = "test";
    tmpl->joints.push_back(pml::Joint{"j0", 0, 0, 10, 0.5, std::nullopt, std::nullopt});
    tmpl->joints.push_back(pml::Joint{"j1", 0, 0, 10, 1.2, std::nullopt, std::nullopt});

    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(skel->angles[0], 0.5);
    EXPECT_DOUBLE_EQ(skel->angles[1], 1.2);
}

TEST(SkeletonInstance, ForwardKinematicsStraightChain) {
    // 3-joint arm at (100,100), all angles 0, all lengths 50.
    // Expected joint positions:
    //   shoulder: (100, 100)
    //   elbow:    (150, 100)
    //   wrist:    (200, 100)
    auto skel = make_arm_instance();
    auto positions = skel->forward_kinematics();

    ASSERT_EQ(positions.size(), 3u);
    EXPECT_NEAR(positions[0].first,  100.0, kEpsilon);
    EXPECT_NEAR(positions[0].second, 100.0, kEpsilon);
    EXPECT_NEAR(positions[1].first,  150.0, kEpsilon);
    EXPECT_NEAR(positions[1].second, 100.0, kEpsilon);
    EXPECT_NEAR(positions[2].first,  200.0, kEpsilon);
    EXPECT_NEAR(positions[2].second, 100.0, kEpsilon);
}

TEST(SkeletonInstance, ForwardKinematicsWithOffsetAngle) {
    // Set shoulder angle to pi/2 (90 degrees upward).
    // Chain goes upward from (100,100):
    //   shoulder: (100, 100)
    //   elbow:    (100, 150)
    //   wrist:    (100, 200)
    auto skel = make_arm_instance();
    skel->angles[0] = kPi / 2.0;

    auto positions = skel->forward_kinematics();
    ASSERT_EQ(positions.size(), 3u);

    EXPECT_NEAR(positions[0].first,  100.0, kEpsilon);
    EXPECT_NEAR(positions[0].second, 100.0, kEpsilon);
    EXPECT_NEAR(positions[1].first,  100.0, kEpsilon);
    EXPECT_NEAR(positions[1].second, 150.0, kEpsilon);
    EXPECT_NEAR(positions[2].first,  100.0, kEpsilon);
    EXPECT_NEAR(positions[2].second, 200.0, kEpsilon);
}

TEST(SkeletonInstance, ForwardKinematicsBentChain) {
    // shoulder=0, elbow=pi/2 (bend upward at elbow).
    // shoulder: (100,100) → bone goes right → (150,100)
    // elbow: (150,100), cumulative = pi/2 → bone goes up → (150,150)
    // wrist: (150,150), cumulative = pi/2 → bone goes up → (150,200)
    auto skel = make_arm_instance();
    skel->angles[1] = kPi / 2.0;

    auto positions = skel->forward_kinematics();
    ASSERT_EQ(positions.size(), 3u);

    EXPECT_NEAR(positions[0].first,  100.0, kEpsilon);
    EXPECT_NEAR(positions[0].second, 100.0, kEpsilon);
    EXPECT_NEAR(positions[1].first,  150.0, kEpsilon);
    EXPECT_NEAR(positions[1].second, 100.0, kEpsilon);
    EXPECT_NEAR(positions[2].first,  150.0, kEpsilon);
    EXPECT_NEAR(positions[2].second, 150.0, kEpsilon);
}

TEST(SkeletonInstance, SetAngleClampsToMax) {
    auto tmpl = std::make_shared<pml::SkeletonTemplate>();
    tmpl->name = "clamped";
    tmpl->joints.push_back(pml::Joint{"j0", 0, 0, 50, 0, -kPi / 4, kPi / 4});

    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 0.0, 0.0);
    skel->set_angle(0, kPi);  // Try to set to pi, should clamp to pi/4

    EXPECT_NEAR(skel->angles[0], kPi / 4.0, kEpsilon);
}

TEST(SkeletonInstance, SetAngleClampsToMin) {
    auto tmpl = std::make_shared<pml::SkeletonTemplate>();
    tmpl->name = "clamped";
    tmpl->joints.push_back(pml::Joint{"j0", 0, 0, 50, 0, -kPi / 4, kPi / 4});

    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 0.0, 0.0);
    skel->set_angle(0, -kPi);  // Try to set to -pi, should clamp to -pi/4

    EXPECT_NEAR(skel->angles[0], -kPi / 4.0, kEpsilon);
}

TEST(SkeletonInstance, SetAngleByName) {
    auto skel = make_arm_instance();
    skel->set_angle("elbow", 1.23);
    EXPECT_NEAR(skel->angles[1], 1.23, kEpsilon);
}

TEST(SkeletonInstance, SetAngleByNameUnknownIsNoop) {
    auto skel = make_arm_instance();
    double original = skel->angles[0];
    skel->set_angle("nonexistent", 99.0);
    EXPECT_DOUBLE_EQ(skel->angles[0], original);
}

TEST(SkeletonInstance, EndEffectorPositionStraightChain) {
    // 3 joints, length 50 each, all angle 0, root at (100,100).
    // End effector = tip of last bone = (250, 100).
    auto skel = make_arm_instance();
    auto [ex, ey] = skel->end_effector_position();
    EXPECT_NEAR(ex, 250.0, kEpsilon);
    EXPECT_NEAR(ey, 100.0, kEpsilon);
}

TEST(SkeletonInstance, EndEffectorMatchesLastBoneTip) {
    // After FK, the end effector should be at the tip of the last bone.
    auto skel = make_arm_instance();
    skel->angles[0] = 0.3;
    skel->angles[1] = -0.2;
    skel->angles[2] = 0.5;

    auto positions = skel->forward_kinematics();
    auto [ex, ey] = skel->end_effector_position();

    // Compute expected end effector manually:
    // last joint pos + length * direction(cumulative angle)
    double cumulative = skel->angles[0] + skel->angles[1] + skel->angles[2];
    double expected_x = positions[2].first  + 50.0 * std::cos(cumulative);
    double expected_y = positions[2].second + 50.0 * std::sin(cumulative);

    EXPECT_NEAR(ex, expected_x, kEpsilon);
    EXPECT_NEAR(ey, expected_y, kEpsilon);
}

TEST(SkeletonInstance, GetJointWorldPos) {
    auto skel = make_arm_instance();
    auto [jx, jy] = skel->get_joint_world_pos("elbow");
    EXPECT_NEAR(jx, 150.0, kEpsilon);
    EXPECT_NEAR(jy, 100.0, kEpsilon);
}

TEST(SkeletonInstance, GetJointWorldPosByIndex) {
    auto skel = make_arm_instance();
    auto [jx, jy] = skel->get_joint_world_pos(0);
    EXPECT_NEAR(jx, 100.0, kEpsilon);
    EXPECT_NEAR(jy, 100.0, kEpsilon);
}

TEST(SkeletonInstance, ChainPositions) {
    auto skel = make_arm_instance();
    // chain_positions(end_idx=2) should return 4 positions:
    // [joint_0, joint_1, joint_2, tip_of_joint_2]
    auto chain = skel->chain_positions(2);
    ASSERT_EQ(chain.size(), 4u);

    EXPECT_NEAR(chain[0].first, 100.0, kEpsilon);
    EXPECT_NEAR(chain[0].second, 100.0, kEpsilon);
    EXPECT_NEAR(chain[1].first, 150.0, kEpsilon);
    EXPECT_NEAR(chain[1].second, 100.0, kEpsilon);
    EXPECT_NEAR(chain[2].first, 200.0, kEpsilon);
    EXPECT_NEAR(chain[2].second, 100.0, kEpsilon);
    // Tip of last bone
    EXPECT_NEAR(chain[3].first, 250.0, kEpsilon);
    EXPECT_NEAR(chain[3].second, 100.0, kEpsilon);
}

TEST(SkeletonInstance, BoneLengths) {
    auto skel = make_arm_instance();
    auto lengths = skel->bone_lengths(2);
    ASSERT_EQ(lengths.size(), 3u);
    EXPECT_DOUBLE_EQ(lengths[0], 50.0);
    EXPECT_DOUBLE_EQ(lengths[1], 50.0);
    EXPECT_DOUBLE_EQ(lengths[2], 50.0);
}

TEST(SkeletonInstance, BoneLengthsByName) {
    auto skel = make_arm_instance();
    auto lengths = skel->bone_lengths("elbow");
    ASSERT_EQ(lengths.size(), 2u);
    EXPECT_DOUBLE_EQ(lengths[0], 50.0);
    EXPECT_DOUBLE_EQ(lengths[1], 50.0);
}

TEST(SkeletonInstance, NullTemplateSafeBehavior) {
    auto skel = std::make_shared<pml::SkeletonInstance>(nullptr, 10.0, 20.0);
    auto fk = skel->forward_kinematics();
    EXPECT_TRUE(fk.empty());

    auto [ex, ey] = skel->end_effector_position();
    EXPECT_DOUBLE_EQ(ex, 10.0);
    EXPECT_DOUBLE_EQ(ey, 20.0);
}

// ==========================================================================================================================================================================================================================================═
// FABRIK IK Solver tests
// ==========================================================================================================================================================================================================================================═

TEST(FABRIK, ConvergesForReachableTarget) {
    auto skel = make_arm_instance();
    // Off-axis target ensures non-degenerate FABRIK convergence.
    // Target (180, 120) is distance ~82.5 from root (100,100), within reach.
    bool converged = pml::solve_fabrik(skel, "wrist", 180.0, 120.0, 50, 0.5);
    EXPECT_TRUE(converged);

    auto [ex, ey] = skel->end_effector_position();
    // Should be close to target
    EXPECT_NEAR(ex, 180.0, 5.0);
    EXPECT_NEAR(ey, 120.0, 5.0);
}

TEST(FABRIK, ExactReachablePosition) {
    auto skel = make_arm_instance();
    // Target at exact reachable position along x-axis: (250, 100) = end effector at rest.
    bool converged = pml::solve_fabrik(skel, "wrist", 250.0, 100.0, 20, 0.01);
    EXPECT_TRUE(converged);

    auto [ex, ey] = skel->end_effector_position();
    EXPECT_NEAR(ex, 250.0, 0.1);
    EXPECT_NEAR(ey, 100.0, 0.1);
}

TEST(FABRIK, UnreachableTargetReturnsFalse) {
    auto skel = make_arm_instance();
    // Total reach = 150 from root (100,100).  Target at (400, 100) is distance 300.
    bool converged = pml::solve_fabrik(skel, "wrist", 400.0, 100.0, 20, 0.01);
    EXPECT_FALSE(converged);
}

TEST(FABRIK, SingleJointChainConverges) {
    auto tmpl = make_single_joint_template();
    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 0.0, 0.0);

    // Single-joint chain always returns true (1 DOF).
    bool converged = pml::solve_fabrik(skel, "tip", 50.0, 50.0, 10, 0.01);
    EXPECT_TRUE(converged);
}

TEST(FABRIK, UnknownEndEffectorReturnsFalse) {
    auto skel = make_arm_instance();
    bool converged = pml::solve_fabrik(skel, "nonexistent", 150.0, 100.0, 10, 0.01);
    EXPECT_FALSE(converged);
}

TEST(FABRIK, NullSkeletonReturnsFalse) {
    bool converged = pml::solve_fabrik(nullptr, "wrist", 150.0, 100.0, 10, 0.01);
    EXPECT_FALSE(converged);
}

// ==========================================================================================================================================================================================================================================═
// CCD IK Solver tests
// ==========================================================================================================================================================================================================================================═

TEST(CCD, ConvergesForReachableTarget) {
    auto skel = make_arm_instance();
    // Off-axis target to avoid degenerate straight-line case.
    bool converged = pml::solve_ccd(skel, "wrist", 180.0, 120.0, 100, 1.0);
    EXPECT_TRUE(converged);

    auto [ex, ey] = skel->end_effector_position();
    EXPECT_NEAR(ex, 180.0, 5.0);
    EXPECT_NEAR(ey, 120.0, 5.0);
}

TEST(CCD, SingleJointChainConverges) {
    auto tmpl = make_single_joint_template();
    auto skel = std::make_shared<pml::SkeletonInstance>(tmpl, 0.0, 0.0);

    // Single-joint chain always returns true.
    bool converged = pml::solve_ccd(skel, "tip", 50.0, 50.0, 20, 0.01);
    EXPECT_TRUE(converged);
}

TEST(CCD, UnreachableTargetReturnsFalse) {
    auto skel = make_arm_instance();
    // Total reach = 150 from root (100,100).  Target at (400, 100) is distance 300.
    bool converged = pml::solve_ccd(skel, "wrist", 400.0, 100.0, 50, 0.01);
    EXPECT_FALSE(converged);
}

TEST(CCD, UnknownEndEffectorReturnsFalse) {
    auto skel = make_arm_instance();
    bool converged = pml::solve_ccd(skel, "nonexistent", 150.0, 100.0, 20, 0.01);
    EXPECT_FALSE(converged);
}

TEST(CCD, NullSkeletonReturnsFalse) {
    bool converged = pml::solve_ccd(nullptr, "wrist", 150.0, 100.0, 20, 0.01);
    EXPECT_FALSE(converged);
}

// ==========================================================================================================================================================================================================================================═
// Skin binding tests
// ==========================================================================================================================================================================================================================================═

TEST(SkinBinding, ClearEmptiesMap) {
    // Insert something into the global registry, then clear.
    auto& bindings = pml::get_skin_bindings();
    bindings[42] = {};  // insert a dummy entry
    EXPECT_FALSE(bindings.empty());

    pml::clear_skin_bindings();
    EXPECT_TRUE(pml::get_skin_bindings().empty());
}

TEST(SkinBinding, GetSkinBindingsReturnsSameReference) {
    auto& ref1 = pml::get_skin_bindings();
    auto& ref2 = pml::get_skin_bindings();
    EXPECT_EQ(&ref1, &ref2);
}

TEST(SkinBinding, RegisterDefinesBindSkinInEnv) {
    auto env = std::make_shared<pml::Environment>();
    pml::register_skin_binding(env);

    // "bind-skin" should now be defined in the environment.
    auto result = env->lookup("bind-skin");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(pml::is_builtin(*result));
}
