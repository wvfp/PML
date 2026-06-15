"""Tests for PML Phase 8 — Skeleton and IK system."""

from __future__ import annotations

import math
import os
import tempfile

import pytest

from pml.errors import IKNoSolutionError
from pml.evaluator import evaluate
from pml.repl import create_global_env
from pml.skeleton.skeleton import Joint, SkeletonInstance, SkeletonTemplate
from pml.types import Symbol


# ======================================================================
# Helpers
# ======================================================================


def _eval(source: str, env=None):
    """Parse and evaluate PML source, return (result, env)."""
    from pml.expander import Expander
    from pml.lexer import Lexer
    from pml.parser import Parser

    if env is None:
        env = create_global_env()

    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    expander = Expander(env)
    ast = expander.expand_all(ast)

    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result, env


# ======================================================================
# TestSkeletonTemplate
# ======================================================================


class TestSkeletonTemplate:
    """Tests for SkeletonTemplate creation and joint lookup."""

    def test_create_template(self):
        joints = (
            Joint("shoulder", pos=(0, 0), length=30, angle=0),
            Joint("elbow", pos=(30, 0), length=25, angle=0.3),
        )
        t = SkeletonTemplate("arm", root_params=("x", "y"), joints=joints)
        assert t.name == "arm"
        assert len(t.joints) == 2

    def test_joint_index(self):
        joints = (
            Joint("a", length=10, angle=0),
            Joint("b", length=20, angle=0),
            Joint("c", length=15, angle=0),
        )
        t = SkeletonTemplate("test", joints=joints)
        assert t.joint_index("a") == 0
        assert t.joint_index("b") == 1
        assert t.joint_index("c") == 2

    def test_joint_index_not_found(self):
        t = SkeletonTemplate("test", joints=(Joint("a", length=10, angle=0),))
        with pytest.raises(ValueError, match="not found"):
            t.joint_index("z")

    def test_joint_constraints(self):
        j = Joint("elbow", length=25, angle=0.3, min_angle=0.0, max_angle=2.5)
        assert j.min_angle == 0.0
        assert j.max_angle == 2.5

    def test_template_repr(self):
        joints = (Joint("a", length=10, angle=0), Joint("b", length=20, angle=0))
        t = SkeletonTemplate("arm", joints=joints)
        r = repr(t)
        assert "arm" in r
        assert "a" in r
        assert "b" in r


# ======================================================================
# TestSkeletonInstance
# ======================================================================


class TestSkeletonInstance:
    """Tests for SkeletonInstance — creation, FK, angle clamping."""

    def _make_arm(self):
        joints = (
            Joint("shoulder", pos=(0, 0), length=30, angle=0),
            Joint("elbow", pos=(0, 0), length=25, angle=0),
        )
        t = SkeletonTemplate("arm", joints=joints)
        return SkeletonInstance(t, 100, 100)

    def test_create_instance(self):
        inst = self._make_arm()
        assert inst.root_x == 100
        assert inst.root_y == 100
        assert len(inst.angles) == 2

    def test_forward_kinematics_straight(self):
        """All angles 0 → bones extend along +x axis."""
        inst = self._make_arm()
        positions = inst.forward_kinematics()
        assert len(positions) == 2
        # shoulder starts at root
        sx, sy = positions[0]
        assert abs(sx - 100) < 0.01
        assert abs(sy - 100) < 0.01
        # elbow is at root + shoulder.length
        ex, ey = positions[1]
        assert abs(ex - 130) < 0.01  # 100 + 30
        assert abs(ey - 100) < 0.01

    def test_forward_kinematics_with_offset(self):
        """Joint with :pos offset from parent."""
        joints = (
            Joint("root", pos=(0, 0), length=20, angle=0),
            Joint("tip", pos=(10, 5), length=15, angle=0),
        )
        t = SkeletonTemplate("test", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        positions = inst.forward_kinematics()
        # root joint at (0, 0), bone extends 20 along +x
        # tip joint at bone_end + offset = (20, 0) + (10, 5) = (30, 5)
        tx, ty = positions[1]
        assert abs(tx - 30) < 0.01
        assert abs(ty - 5) < 0.01

    def test_joint_world_position(self):
        inst = self._make_arm()
        pos = inst.joint_world_position("elbow")
        assert abs(pos[0] - 130) < 0.01

    def test_clamp_angle(self):
        joints = (
            Joint("j", length=10, angle=0, min_angle=-1.0, max_angle=1.0),
        )
        t = SkeletonTemplate("test", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        assert inst.clamp_angle(0, 0.5) == 0.5
        assert inst.clamp_angle(0, 2.0) == 1.0
        assert inst.clamp_angle(0, -3.0) == -1.0

    def test_end_effector_position(self):
        """End effector is the tip of the last bone."""
        joints = (
            Joint("a", pos=(0, 0), length=10, angle=0),
        )
        t = SkeletonTemplate("test", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        pos = inst.end_effector_position()
        assert abs(pos[0] - 10) < 0.01
        assert abs(pos[1] - 0) < 0.01


# ======================================================================
# TestDefskeleton
# ======================================================================


class TestDefskeleton:
    """Tests for the defskelton PML special form."""

    def test_basic_defskeleton(self):
        source = """
        (defskeleton arm (x y)
          (joint shoulder :pos (0 0) :length 30 :angle 0 :min -1.5 :max 1.5)
          (joint elbow :pos (30 0) :length 25 :angle 0.3 :min 0 :max 2.0))
        """
        result, env = _eval(source)
        template = env.lookup("arm")
        assert isinstance(template, SkeletonTemplate)
        assert template.name == "arm"
        assert len(template.joints) == 2
        assert template.joints[0].name == "shoulder"
        assert template.joints[1].name == "elbow"

    def test_defskeleton_params(self):
        source = """
        (defskeleton test (rx ry)
          (joint j1 :pos (5 10) :length 20 :angle 1.5))
        """
        _, env = _eval(source)
        t = env.lookup("test")
        assert t.root_params == ("rx", "ry")
        assert t.joints[0].pos == (5.0, 10.0)
        assert t.joints[0].length == 20.0
        assert t.joints[0].angle == 1.5

    def test_defskeleton_constraints(self):
        source = """
        (defskeleton constrained (x y)
          (joint j :length 10 :angle 0 :min -0.5 :max 0.5))
        """
        _, env = _eval(source)
        t = env.lookup("constrained")
        assert t.joints[0].min_angle == -0.5
        assert t.joints[0].max_angle == 0.5

    def test_defskeleton_defaults(self):
        """Minimal joint — only name and length."""
        source = """
        (defskeleton minimal (x y)
          (joint j :length 15))
        """
        _, env = _eval(source)
        t = env.lookup("minimal")
        assert t.joints[0].length == 15.0
        assert t.joints[0].angle == 0.0
        assert t.joints[0].pos == (0.0, 0.0)
        assert t.joints[0].min_angle is None


# ======================================================================
# TestFABRIK
# ======================================================================


class TestFABRIK:
    """Tests for the FABRIK IK solver."""

    def _make_chain(self, n=3, length=20.0):
        """Create a straight chain of n joints."""
        joints = tuple(
            Joint(f"j{i}", pos=(0, 0), length=length, angle=0)
            for i in range(n)
        )
        t = SkeletonTemplate("chain", joints=joints)
        return SkeletonInstance(t, 0, 0)

    def test_convergence_reachable(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        inst = self._make_chain(3, 20)
        # Target within reach (total = 60)
        converged = solve_fabrik(inst, "j2", 40, 10, max_iterations=20)
        assert converged is True

    def test_convergence_exact_reach(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        inst = self._make_chain(2, 20)
        # Target at exact reach
        converged = solve_fabrik(inst, "j1", 40, 0, max_iterations=20)
        assert converged is True

    def test_unreachable_target(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        inst = self._make_chain(2, 20)
        # Target beyond total reach (40) + some margin
        converged = solve_fabrik(inst, "j1", 100, 0, max_iterations=10)
        assert converged is False

    def test_single_joint(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        joints = (Joint("j0", pos=(0, 0), length=20, angle=0),)
        t = SkeletonTemplate("single", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        # Target at exactly bone_length distance (Pythagorean triple 12²+16²=20²)
        converged = solve_fabrik(inst, "j0", 12, 16, max_iterations=20)
        assert converged is True
        # End effector should reach target exactly
        pos = inst.end_effector_position()
        assert math.hypot(pos[0] - 12, pos[1] - 16) < 0.1

    def test_with_constraints(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        joints = (
            Joint("j0", pos=(0, 0), length=20, angle=0,
                  min_angle=-0.5, max_angle=0.5),
            Joint("j1", pos=(20, 0), length=20, angle=0,
                  min_angle=-0.5, max_angle=0.5),
        )
        t = SkeletonTemplate("constrained", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        solve_fabrik(inst, "j1", 30, 5, max_iterations=20)
        # Angles should be within constraints
        for i, angle in enumerate(inst.angles):
            j = t.joints[i]
            if j.min_angle is not None:
                assert angle >= j.min_angle - 0.01
            if j.max_angle is not None:
                assert angle <= j.max_angle + 0.01

    def test_angles_updated(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        inst = self._make_chain(2, 20)
        original = list(inst.angles)
        solve_fabrik(inst, "j1", 20, 15, max_iterations=20)
        # At least one angle should have changed
        assert inst.angles != original

    def test_tolerance(self):
        from pml.skeleton.ik_fabrik import solve_fabrik

        inst = self._make_chain(3, 20)
        converged = solve_fabrik(
            inst, "j2", 30, 10, max_iterations=50, tolerance=0.5
        )
        if converged:
            pos = inst.end_effector_position()
            assert math.hypot(pos[0] - 30, pos[1] - 10) < 0.5


# ======================================================================
# TestCCD
# ======================================================================


class TestCCD:
    """Tests for the CCD IK solver."""

    def _make_chain(self, n=3, length=20.0):
        joints = tuple(
            Joint(f"j{i}", pos=(0, 0), length=length, angle=0)
            for i in range(n)
        )
        t = SkeletonTemplate("chain", joints=joints)
        return SkeletonInstance(t, 0, 0)

    def test_convergence_reachable(self):
        from pml.skeleton.ik_ccd import solve_ccd

        inst = self._make_chain(3, 20)
        converged = solve_ccd(inst, "j2", 40, 10, max_iterations=20)
        assert converged is True

    def test_single_joint(self):
        from pml.skeleton.ik_ccd import solve_ccd

        joints = (Joint("j0", pos=(0, 0), length=20, angle=0),)
        t = SkeletonTemplate("single", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        # Target at exactly bone_length distance (Pythagorean triple 16²+12²=20²)
        converged = solve_ccd(inst, "j0", 16, 12, max_iterations=20)
        assert converged is True

    def test_with_constraints(self):
        from pml.skeleton.ik_ccd import solve_ccd

        joints = (
            Joint("j0", pos=(0, 0), length=20, angle=0,
                  min_angle=-1.0, max_angle=1.0),
            Joint("j1", pos=(20, 0), length=20, angle=0,
                  min_angle=-1.0, max_angle=1.0),
        )
        t = SkeletonTemplate("constrained", joints=joints)
        inst = SkeletonInstance(t, 0, 0)
        solve_ccd(inst, "j1", 25, 10, max_iterations=20)
        for i, angle in enumerate(inst.angles):
            j = t.joints[i]
            if j.min_angle is not None:
                assert angle >= j.min_angle - 0.01
            if j.max_angle is not None:
                assert angle <= j.max_angle + 0.01

    def test_angles_updated(self):
        from pml.skeleton.ik_ccd import solve_ccd

        inst = self._make_chain(2, 20)
        original = list(inst.angles)
        solve_ccd(inst, "j1", 20, 15, max_iterations=20)
        assert inst.angles != original

    def test_unreachable_stretches(self):
        from pml.skeleton.ik_ccd import solve_ccd

        inst = self._make_chain(2, 20)
        converged = solve_ccd(inst, "j1", 100, 0, max_iterations=10)
        assert converged is False

    def test_end_effector_close(self):
        from pml.skeleton.ik_ccd import solve_ccd

        inst = self._make_chain(4, 15)
        solve_ccd(inst, "j3", 30, 20, max_iterations=50, tolerance=1.0)
        pos = inst.end_effector_position()
        dist = math.hypot(pos[0] - 30, pos[1] - 20)
        assert dist < 5.0  # Reasonable proximity


# ======================================================================
# TestIKSolve (PML builtin)
# ======================================================================


class TestIKSolve:
    """Tests for the ik-solve PML builtin."""

    def test_ik_solve_fabrik(self):
        source = """
        (defskeleton arm (x y)
          (joint shoulder :pos (0 0) :length 30 :angle 0)
          (joint elbow :pos (30 0) :length 25 :angle 0)
          (joint hand :pos (25 0) :length 0 :angle 0))
        (define my-arm (instantiate-skeleton arm 100 100))
        (ik-solve my-arm 'hand 150 120 :method 'fabrik :iterations 20)
        """
        result, _ = _eval(source)
        assert result is True or result is False

    def test_ik_solve_ccd(self):
        source = """
        (defskeleton chain (x y)
          (joint a :pos (0 0) :length 20 :angle 0)
          (joint b :pos (20 0) :length 20 :angle 0))
        (define sk (instantiate-skeleton chain 0 0))
        (ik-solve sk 'b 25 15 :method 'ccd :iterations 20)
        """
        result, _ = _eval(source)
        assert result is True or result is False

    def test_ik_solve_strict_mode(self):
        source = """
        (defskeleton short (x y)
          (joint j :pos (0 0) :length 10 :angle 0))
        (define sk (instantiate-skeleton short 0 0))
        (ik-solve sk 'j 1000 1000 :iterations 1 :tolerance 0.001 :strict #t)
        """
        with pytest.raises(IKNoSolutionError):
            _eval(source)

    def test_instantiate_skeleton(self):
        source = """
        (defskeleton test (x y)
          (joint j1 :length 15 :angle 0)
          (joint j2 :length 10 :angle 0))
        (define sk (instantiate-skeleton test 50 60 :name "demo"))
        sk
        """
        result, _ = _eval(source)
        assert isinstance(result, SkeletonInstance)
        assert result.root_x == 50
        assert result.root_y == 60
        assert result.name == "demo"


# ======================================================================
# TestBindSkin
# ======================================================================


class TestBindSkin:
    """Tests for the bind-skin builtin and registry."""

    def setup_method(self):
        from pml.skeleton import clear_skin_bindings
        clear_skin_bindings()

    def test_bind_registers(self):
        source = """
        (defskeleton arm (x y)
          (joint shoulder :length 30 :angle 0)
          (joint elbow :length 25 :angle 0))
        (define sk (instantiate-skeleton arm 100 100))
        (define shape (rect 0 0 30 10 :fill "gray"))
        (bind-skin shape sk 'shoulder)
        """
        _eval(source)
        from pml.skeleton import get_skin_bindings
        bindings = get_skin_bindings()
        assert len(bindings) > 0

    def test_bind_multiple_joints(self):
        source = """
        (defskeleton arm (x y)
          (joint shoulder :length 30 :angle 0)
          (joint elbow :length 25 :angle 0))
        (define sk (instantiate-skeleton arm 0 0))
        (define shape (rect 0 0 10 10 :fill "red"))
        (bind-skin shape sk 'shoulder 'elbow)
        """
        _eval(source)
        from pml.skeleton import get_skin_bindings
        bindings = get_skin_bindings()
        # Should have one entry with two joint names
        for gid, blist in bindings.items():
            for skel, names in blist:
                assert len(names) == 2

    def test_clear_bindings(self):
        from pml.skeleton import clear_skin_bindings, get_skin_bindings
        source = """
        (defskeleton arm (x y)
          (joint j :length 10 :angle 0))
        (define sk (instantiate-skeleton arm 0 0))
        (define shape (circle 0 0 5 :fill "blue"))
        (bind-skin shape sk 'j)
        """
        _eval(source)
        assert len(get_skin_bindings()) > 0
        clear_skin_bindings()
        assert len(get_skin_bindings()) == 0

    def teardown_method(self):
        from pml.skeleton import clear_skin_bindings
        clear_skin_bindings()


# ======================================================================
# TestJointPosition
# ======================================================================


class TestJointPosition:
    """Tests for the joint-position builtin."""

    def test_basic_query(self):
        source = """
        (defskeleton test (x y)
          (joint j1 :pos (0 0) :length 20 :angle 0)
          (joint j2 :pos (20 0) :length 15 :angle 0))
        (define sk (instantiate-skeleton test 100 50))
        (joint-position sk 'j1)
        """
        result, _ = _eval(source)
        assert isinstance(result, list)
        assert len(result) == 2
        assert abs(result[0] - 100) < 0.01
        assert abs(result[1] - 50) < 0.01

    def test_fk_consistency(self):
        """joint-position should match forward_kinematics."""
        source = """
        (defskeleton chain (x y)
          (joint a :pos (0 0) :length 20 :angle 0.5)
          (joint b :pos (20 0) :length 15 :angle 0.3))
        (define sk (instantiate-skeleton chain 0 0))
        (joint-position sk 'b)
        """
        result, env = _eval(source)
        sk = env.lookup("sk")
        fk_pos = sk.joint_world_position("b")
        assert abs(result[0] - fk_pos[0]) < 0.01
        assert abs(result[1] - fk_pos[1]) < 0.01

    def test_position_after_ik(self):
        """joint-position should reflect IK-solved angles."""
        source = """
        (defskeleton arm (x y)
          (joint shoulder :pos (0 0) :length 30 :angle 0)
          (joint elbow :pos (30 0) :length 25 :angle 0))
        (define sk (instantiate-skeleton arm 0 0))
        (define before (joint-position sk 'elbow))
        (ik-solve sk 'elbow 40 20 :iterations 30)
        (define after (joint-position sk 'elbow))
        after
        """
        result, env = _eval(source)
        before = env.lookup("before")
        # Positions should have changed after IK
        assert abs(result[0] - before[0]) > 0.1 or abs(result[1] - before[1]) > 0.1


# ======================================================================
# TestPMLIntegration
# ======================================================================


class TestPMLIntegration:
    """End-to-end PML integration tests."""

    def test_full_skeleton_workflow(self):
        """Define skeleton, instantiate, solve IK, query positions."""
        source = """
        (defskeleton arm (x y)
          (joint shoulder :pos (0 0) :length 60 :angle 0 :min -1.5 :max 1.5)
          (joint elbow   :pos (0 0) :length 50 :angle 0 :min 0 :max 2.5)
          (joint hand    :pos (0 0) :length 0  :angle 0))
    
        (define my-arm (instantiate-skeleton arm 200 200 :name "demo"))
        (ik-solve my-arm 'hand 260 230 :method 'fabrik :iterations 20)
        (joint-position my-arm 'hand)
        """
        result, _ = _eval(source)
        assert isinstance(result, list)
        # Hand should be near target (260, 230)
        assert math.hypot(result[0] - 260, result[1] - 230) < 5.0

    def test_skeleton_with_animation_hooks(self):
        """every-frame hook calling ik-solve works without errors."""
        source = """
        (canvas 200 200 :bg "white")
        (defskeleton arm (x y)
          (joint shoulder :length 30 :angle 0)
          (joint elbow :length 25 :angle 0))
        (define sk (instantiate-skeleton arm 100 100))
        (define target-x 150)
        (define target-y 80)
        (every-frame (lambda ()
          (ik-solve sk 'elbow target-x target-y :method 'fabrik)))
        """
        result, env = _eval(source)
        # Should not raise; every-frame registered
        from pml.animation.timeline import Timeline
        timeline = Timeline.get_current()
        assert len(timeline.frame_hooks) >= 1
        Timeline.reset()

    def test_multiple_skeletons(self):
        """Two independent skeleton instances."""
        source = """
        (defskeleton arm (x y)
          (joint j1 :length 20 :angle 0)
          (joint j2 :length 15 :angle 0))
        (define left (instantiate-skeleton arm 0 0 :name "left"))
        (define right (instantiate-skeleton arm 100 0 :name "right"))
        (ik-solve left 'j2 15 10)
        (ik-solve right 'j2 120 10)
        (list (joint-position left 'j2) (joint-position right 'j2))
        """
        result, _ = _eval(source)
        assert isinstance(result, list)
        assert len(result) == 2
        # Left and right should have different positions
        left_pos = result[0]
        right_pos = result[1]
        assert left_pos[0] != right_pos[0]

    def test_skeleton_gif_pipeline(self):
        """End-to-end: skeleton + IK + every-frame → GIF output."""
        with tempfile.TemporaryDirectory() as tmpdir:
            gif_path = os.path.join(tmpdir, "ik_demo.gif")
            pml_path = gif_path.replace("\\", "/")

            source = (
                '(canvas 200 200 :bg "white")\n'
                "(defskeleton arm (x y)\n"
                "  (joint shoulder :pos (0 0) :length 40 :angle 0 :min -1.5 :max 1.5)\n"
                "  (joint elbow   :pos (0 0) :length 35 :angle 0 :min 0 :max 2.0))\n"
                "(define sk (instantiate-skeleton arm 100 150))\n"
                "(define tx 150)\n"
                "(define ty 80)\n"
                "(every-frame (lambda () (ik-solve sk 'elbow tx ty :iterations 10)))\n"
                "(define (draw-bone sk j1 j2)\n"
                "  (let ((p1 (joint-position sk j1))\n"
                "        (p2 (joint-position sk j2)))\n"
                '    (line (car p1) (car (cdr p1)) (car p2) (car (cdr p2)) :stroke "black" :stroke-width 3)))\n'
                "(add (draw-bone sk 'shoulder 'elbow))\n"
                f'(render "{pml_path}" :format \'gif :fps 10)\n'
            )
            result, env = _eval(source)
            assert os.path.exists(gif_path)
            assert os.path.getsize(gif_path) > 100

            from pml.animation.timeline import Timeline
            Timeline.reset()

    def test_error_hint_ik(self):
        """IKNoSolutionError produces a useful hint via the API."""
        from pml.api import _generate_hint

        err = IKNoSolutionError("test")
        hint = _generate_hint(err)
        assert "IK" in hint or "iterations" in hint
