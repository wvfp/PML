"""Tests for Phase 7 — animation system."""

import os
import tempfile

import pytest

from pml.animation.easing import (
    EASING_FUNCTIONS,
    get_easing,
    list_easings,
    _linear,
    _bounce,
    _elastic,
)
from pml.animation.interpolate import (
    lerp_color,
    lerp_numeric,
    lerp_transform,
    lerp_value,
)
from pml.animation.engine import (
    Animation,
    ParallelAnimation,
    PropertyAnimation,
    SequenceAnimation,
)
from pml.animation.timeline import Timeline
from pml.backend.pillow import PillowBackend
from pml.graphics.canvas import Canvas, _current_canvas
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.types import Symbol


# ======================================================================
# Easing Functions
# ======================================================================


class TestEasing:
    def test_all_easings_boundary_start(self):
        """All easing functions should return ~0 at t=0."""
        for name, fn in EASING_FUNCTIONS.items():
            assert abs(fn(0.0)) < 0.01, f"{name}(0) = {fn(0.0)}"

    def test_all_easings_boundary_end(self):
        """All easing functions should return ~1 at t=1."""
        for name, fn in EASING_FUNCTIONS.items():
            assert abs(fn(1.0) - 1.0) < 0.01, f"{name}(1) = {fn(1.0)}"

    def test_linear(self):
        assert _linear(0.0) == 0.0
        assert _linear(0.5) == 0.5
        assert _linear(1.0) == 1.0

    def test_quad_in_monotonic(self):
        fn = EASING_FUNCTIONS["quad-in"]
        prev = 0.0
        for i in range(11):
            t = i / 10
            val = fn(t)
            assert val >= prev - 0.001
            prev = val

    def test_bounce_shape(self):
        """Bounce should overshoot and come back."""
        # At t=0.5, bounce is still going up
        assert _bounce(0.5) > 0
        assert _bounce(1.0) == pytest.approx(1.0, abs=0.01)

    def test_elastic_overshoots(self):
        """Elastic should overshoot 1.0 before settling."""
        vals = [_elastic(t) for t in [i / 20 for i in range(21)]]
        # Should have values > 1.0 (overshoot)
        assert any(v > 1.0 for v in vals)
        # Should end at 1.0
        assert abs(vals[-1] - 1.0) < 0.01

    def test_get_easing_known(self):
        fn = get_easing("quad-out")
        assert fn is EASING_FUNCTIONS["quad-out"]

    def test_get_easing_unknown(self):
        fn = get_easing("nonexistent")
        assert fn(0.5) == 0.5  # Falls back to linear

    def test_get_easing_symbol(self):
        fn = get_easing(Symbol("cubic-in"))
        assert fn is EASING_FUNCTIONS["cubic-in"]

    def test_list_easings(self):
        names = list_easings()
        assert "linear" in names
        assert "bounce" in names
        assert len(names) == len(EASING_FUNCTIONS)


# ======================================================================
# Interpolation
# ======================================================================


class TestInterpolate:
    def test_lerp_numeric_int(self):
        assert lerp_numeric(0, 100, 0.5) == 50.0

    def test_lerp_numeric_float(self):
        assert lerp_numeric(0.0, 1.0, 0.25) == 0.25

    def test_lerp_numeric_boundaries(self):
        assert lerp_numeric(10, 20, 0.0) == 10.0
        assert lerp_numeric(10, 20, 1.0) == 20.0

    def test_lerp_color_basic(self):
        result = lerp_color("#000000", "#ffffff", 0.5)
        assert result.startswith("#")
        # Should be approximately gray
        r = int(result[1:3], 16)
        assert 120 <= r <= 135

    def test_lerp_color_boundaries(self):
        assert lerp_color("#ff0000", "#0000ff", 0.0) == "#ff0000"
        result = lerp_color("#ff0000", "#0000ff", 1.0)
        assert result == "#0000ff"

    def test_lerp_color_named(self):
        result = lerp_color("red", "blue", 0.5)
        assert isinstance(result, str)
        assert result.startswith("#")

    def test_lerp_transform_translate(self):
        a = AffineTransform.translate(0, 0)
        b = AffineTransform.translate(100, 200)
        mid = lerp_transform(a, b, 0.5)
        assert abs(mid.e - 50) < 1
        assert abs(mid.f - 100) < 1

    def test_lerp_transform_identity(self):
        a = AffineTransform.identity()
        b = AffineTransform.identity()
        mid = lerp_transform(a, b, 0.5)
        assert abs(mid.e) < 0.01
        assert abs(mid.f) < 0.01

    def test_lerp_value_int(self):
        result = lerp_value(0, 100, 0.5)
        assert result == 50

    def test_lerp_value_float(self):
        result = lerp_value(0.0, 1.0, 0.5)
        assert result == 0.5

    def test_lerp_value_string(self):
        result = lerp_value("#000000", "#ffffff", 0.5)
        assert isinstance(result, str)

    def test_lerp_value_bool_discrete(self):
        assert lerp_value(True, False, 0.3) is True
        assert lerp_value(True, False, 0.7) is False

    def test_lerp_value_discrete(self):
        """Non-interpolatable types switch at t=0.5."""
        assert lerp_value("a", 42, 0.3) == "a"
        assert lerp_value("a", 42, 0.7) == 42


# ======================================================================
# Property Animation
# ======================================================================


class TestPropertyAnimation:
    def test_basic_linear(self):
        anim = PropertyAnimation(
            target_id=12345,
            property="x",
            from_value=0,
            to_value=100,
            duration=1.0,
        )
        # At t=0, should be 0
        vals = anim.get_value_at(0.0)
        assert len(vals) == 1
        assert vals[0] == (12345, "x", 0)

        # At t=0.5, should be ~50
        vals = anim.get_value_at(0.5)
        assert abs(vals[0][2] - 50) < 1

        # At t=1.0, should be 100
        vals = anim.get_value_at(1.0)
        assert abs(vals[0][2] - 100) < 1

    def test_duration(self):
        anim = PropertyAnimation(
            target_id=1, property="y", from_value=0, to_value=100,
            duration=2.5,
        )
        assert anim.get_duration() == 2.5

    def test_delay(self):
        anim = PropertyAnimation(
            target_id=1, property="y", from_value=0, to_value=100,
            duration=1.0, delay=0.5,
        )
        # Before delay — should be at from_value
        vals = anim.get_value_at(0.2)
        assert vals[0][2] == 0

        # After delay — should be interpolating
        vals = anim.get_value_at(1.0)
        assert vals[0][2] == 50 or abs(vals[0][2] - 50) < 1

        # Total duration includes delay
        assert anim.get_duration() == 1.5

    def test_repeat(self):
        anim = PropertyAnimation(
            target_id=1, property="x", from_value=0, to_value=100,
            duration=1.0, repeat=3,
        )
        assert anim.get_duration() == 3.0

        # At t=1.5 (middle of second cycle), should be at ~50
        vals = anim.get_value_at(1.5)
        assert abs(vals[0][2] - 50) < 1

    def test_repeat_infinite(self):
        anim = PropertyAnimation(
            target_id=1, property="x", from_value=0, to_value=100,
            duration=1.0, repeat="infinite",
        )
        assert anim.get_duration() == float("inf")

        # Even at t=100, should still produce values
        vals = anim.get_value_at(100.5)
        assert abs(vals[0][2] - 50) < 1

    def test_easing_applied(self):
        fn = get_easing("quad-in")
        anim = PropertyAnimation(
            target_id=1, property="x", from_value=0, to_value=100,
            duration=1.0, ease_fn=fn,
        )
        # quad-in at t=0.5 → 0.25 → value should be ~25
        vals = anim.get_value_at(0.5)
        assert abs(vals[0][2] - 25) < 1

    def test_past_end(self):
        anim = PropertyAnimation(
            target_id=1, property="x", from_value=0, to_value=100,
            duration=1.0, repeat=1,
        )
        # Past the end — should be at to_value
        vals = anim.get_value_at(5.0)
        assert abs(vals[0][2] - 100) < 1


# ======================================================================
# Composite Anim
# ======================================================================


class TestCompositeAnimation:
    def test_parallel_duration(self):
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 2.5)
        par = ParallelAnimation([a1, a2])
        assert par.get_duration() == 2.5

    def test_parallel_evaluate(self):
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 2.0)
        par = ParallelAnimation([a1, a2])
        vals = par.get_value_at(0.5)
        # Should have results from both animations
        targets = {v[0] for v in vals}
        assert 1 in targets
        assert 2 in targets

    def test_sequence_duration(self):
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 1.5)
        seq = SequenceAnimation([a1, a2])
        assert seq.get_duration() == 2.5

    def test_sequence_offset(self):
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 1.0)
        seq = SequenceAnimation([a1, a2])

        # At t=0.5, only first animation is active
        vals = seq.get_value_at(0.5)
        target_ids = {v[0] for v in vals}
        assert 1 in target_ids

        # At t=1.5, second animation should be active
        vals = seq.get_value_at(1.5)
        target_ids = {v[0] for v in vals}
        assert 2 in target_ids

    def test_empty_parallel(self):
        par = ParallelAnimation([])
        assert par.get_duration() == 0.0
        assert par.get_value_at(0.5) == []

    def test_nested_composite(self):
        """Parallel of sequences should work."""
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 1.0)
        seq = SequenceAnimation([a1, a2])
        a3 = PropertyAnimation(3, "r", 10, 50, 2.0)
        par = ParallelAnimation([seq, a3])
        assert par.get_duration() == 2.0


# ======================================================================
# Timeline
# ======================================================================


class TestTimeline:
    def setup_method(self):
        Timeline.reset()

    def test_add_animation(self):
        tl = Timeline.get_current()
        anim = PropertyAnimation(1, "x", 0, 100, 1.0)
        tl.add(anim)
        assert len(tl.animations) == 1

    def test_play_stop(self):
        tl = Timeline.get_current()
        tl.play()
        assert tl.state == "playing"
        tl.stop()
        assert tl.state == "stopped"
        assert tl.current_time == 0.0

    def test_pause(self):
        tl = Timeline.get_current()
        tl.play()
        tl.pause()
        assert tl.state == "paused"

    def test_seek(self):
        tl = Timeline.get_current()
        tl.seek(2.5)
        assert tl.current_time == 2.5

    def test_total_duration(self):
        tl = Timeline.get_current()
        a1 = PropertyAnimation(1, "x", 0, 100, 1.0)
        a2 = PropertyAnimation(2, "y", 0, 200, 3.0)
        tl.add(a1)
        tl.add(a2)
        assert tl.get_total_duration() == 3.0

    def test_render_frames(self):
        tl = Timeline.get_current()
        # Create a simple scene
        obj = GraphicObject(
            shape_type="circle",
            params={"x": 50, "y": 50, "r": 20},
            fill="red",
        )
        canvas = Canvas(width=100, height=100, bg_color="white")
        canvas.objects.append(obj)

        # Animate x from 50 to 150
        anim = PropertyAnimation(
            target_id=id(obj), property="x",
            from_value=50, to_value=150, duration=1.0,
        )
        tl.add(anim)

        backend = PillowBackend()
        frames = tl.render_frames(
            canvas=canvas, fps=10, backend=backend,
            width=100, height=100,
        )
        assert len(frames) == 10  # 1 second * 10 fps

    def test_singleton(self):
        t1 = Timeline.get_current()
        t2 = Timeline.get_current()
        assert t1 is t2

    def test_reset(self):
        t1 = Timeline.get_current()
        Timeline.reset()
        t2 = Timeline.get_current()
        assert t1 is not t2


# ======================================================================
# GIF Output
# ======================================================================


class TestGIFOutput:
    def test_save_animation_gif(self):
        backend = PillowBackend()
        # Create a few simple frames
        frames = []
        for i in range(5):
            surface = backend.create_surface(64, 64, "white")
            obj = GraphicObject(
                shape_type="circle",
                params={"x": 10 + i * 10, "y": 32, "r": 8},
                fill="red",
            )
            backend.draw(surface, obj)
            frames.append(surface)

        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "test.gif")
            backend.save_animation(frames, path, "GIF", 10)
            assert os.path.exists(path)
            assert os.path.getsize(path) > 0

    def test_save_animation_empty(self):
        backend = PillowBackend()
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "empty.gif")
            backend.save_animation([], path, "GIF", 10)
            # Should not create file (or create empty)
            assert not os.path.exists(path) or os.path.getsize(path) == 0


# ======================================================================
# PML Integration
# ======================================================================


class TestPMLIntegration:
    def setup_method(self):
        Timeline.reset()

    def _run(self, source: str):
        """Parse and evaluate PML source."""
        from pml.repl import create_global_env
        from pml.lexer import Lexer
        from pml.parser import Parser
        from pml.expander import Expander
        from pml.evaluator import evaluate

        env = create_global_env()
        tokens = Lexer(source).tokenize()
        ast = Parser(tokens).parse()
        ast = Expander(env).expand_all(ast)
        result = None
        for expr in ast:
            result = evaluate(expr, env)
        return result, env

    def test_animate_creates_animation(self):
        result, env = self._run(
            '(define c (circle 50 50 20 :fill "red"))\n'
            '(animate c \'y 50 150 1.0)\n'
        )
        assert isinstance(result, Animation)
        tl = Timeline.get_current()
        assert len(tl.animations) == 1

    def test_parallel_builtin(self):
        result, env = self._run(
            '(define c (circle 50 50 20 :fill "red"))\n'
            '(define a1 (animate c \'y 50 150 1.0))\n'
            '(define a2 (animate c \'x 50 100 1.0))\n'
            '(parallel a1 a2)\n'
        )
        assert isinstance(result, ParallelAnimation)

    def test_sequence_builtin(self):
        result, env = self._run(
            '(define c (circle 50 50 20 :fill "red"))\n'
            '(define a1 (animate c \'y 50 150 1.0))\n'
            '(define a2 (animate c \'x 50 100 1.0))\n'
            '(sequence a1 a2)\n'
        )
        assert isinstance(result, SequenceAnimation)

    def test_animation_state(self):
        result, env = self._run('(animation-state)')
        assert isinstance(result, Symbol)
        assert result.name == "idle"

    def test_play_stop(self):
        self._run('(play)')
        tl = Timeline.get_current()
        assert tl.state == "playing"

        self._run('(stop)')
        # Note: stop creates a new evaluation context but timeline is global
        assert tl.state == "stopped"

    def test_full_gif_pipeline(self):
        """End-to-end: create animation and render to GIF."""
        with tempfile.TemporaryDirectory() as tmpdir:
            gif_path = os.path.join(tmpdir, "bounce.gif")
            # Use forward slashes for PML
            pml_path = gif_path.replace("\\", "/")

            source = (
                '(canvas 64 64 :bg "white")\n'
                '(define ball (circle 32 50 10 :fill "red"))\n'
                '(add ball)\n'
                '(animate ball \'y 50 10 0.5 :ease \'quad-out)\n'
                f'(render "{pml_path}" :format \'gif :fps 10)\n'
            )
            result, env = self._run(source)
            assert os.path.exists(gif_path)
            assert os.path.getsize(gif_path) > 0

    def test_animate_with_ease(self):
        result, env = self._run(
            '(define c (circle 50 50 20 :fill "blue"))\n'
            '(animate c \'r 20 40 1.0 :ease \'bounce)\n'
        )
        assert isinstance(result, PropertyAnimation)
        tl = Timeline.get_current()
        assert len(tl.animations) == 1

    def test_animate_with_repeat(self):
        result, env = self._run(
            '(define c (circle 50 50 20 :fill "green"))\n'
            '(animate c \'y 50 100 0.5 :repeat 4)\n'
        )
        assert isinstance(result, PropertyAnimation)
        assert result.get_duration() == 2.0
