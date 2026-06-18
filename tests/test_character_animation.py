"""Tests for character animation presets — blink, idle-breath, walk-cycle.

These tests verify:
  1. Animation presets produce correct PropertyAnimation objects
  2. The builtins are available from PML
  3. Animation tracks are registered on the Timeline
"""

from __future__ import annotations

import pytest

from pml.animation.engine import PropertyAnimation
from pml.animation.timeline import Timeline
from pml.graphics.objects import GraphicObject
from pml.repl import create_global_env
from pml.lexer import Lexer
from pml.parser import Parser
from pml.evaluator import evaluate


def run(source: str):
    env = create_global_env()
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result


class TestBlink:
    """Blink animation presets."""

    def test_blink_creates_animations(self) -> None:
        from pml.sprites.character_animation import blink
        char = run("(character :expression 'neutral)")
        timeline = Timeline.get_current()
        count_before = len(timeline.animations)
        result = blink(char)
        timeline.reset()
        assert len(result) > 0
        assert all(isinstance(a, PropertyAnimation) for a in result)
        assert all(a.property == "ry" for a in result)

    def test_blink_on_back_view_returns_empty(self) -> None:
        from pml.sprites.character_animation import blink
        char = run('(character :expression "neutral" :view "back")')
        result = blink(char)
        assert len(result) == 0  # no eyes visible from behind

    def test_blink_pml_builtin(self) -> None:
        Timeline.reset()
        result = run("""
            (define c (character :expression "neutral"))
            (blink c)
        """)
        timeline = Timeline.get_current()
        assert len(timeline.animations) > 0
        timeline.reset()


class TestIdleBreath:
    """Idle breath animation presets."""

    def test_idle_breath_creates_animations(self) -> None:
        from pml.sprites.character_animation import idle_breath
        char = run("(character :expression 'neutral)")
        result = idle_breath(char)
        assert len(result) > 0
        assert all(isinstance(a, PropertyAnimation) for a in result)

    def test_idle_breath_pml_builtin(self) -> None:
        Timeline.reset()
        result = run("""
            (define c (character))
            (idle-breath c)
        """)
        timeline = Timeline.get_current()
        assert len(timeline.animations) > 0
        timeline.reset()


class TestWalkCycle:
    """Walk cycle animation presets."""

    def test_walk_cycle_creates_animations(self) -> None:
        from pml.sprites.character_animation import walk_cycle
        char = run("(character :expression 'neutral)")
        result = walk_cycle(char)
        assert len(result) > 0
        assert all(isinstance(a, PropertyAnimation) for a in result)

    def test_walk_cycle_pml_builtin(self) -> None:
        Timeline.reset()
        result = run("""
            (define c (character))
            (walk-cycle c :distance 60 :duration 0.8)
        """)
        timeline = Timeline.get_current()
        assert len(timeline.animations) > 0
        timeline.reset()

    def test_walk_cycle_properties(self) -> None:
        from pml.sprites.character_animation import walk_cycle
        char = run("(character)")
        result = walk_cycle(char, distance=100, duration=1.5, bounce=4)
        # Should have x movement, y bounce, and body tilt
        props = {a.property for a in result}
        assert "transform.tx" in props
        assert "transform.ty" in props
        assert "transform.rot" in props


class TestTransformAnimations:
    """Animation system supports scale and rotation transforms."""

    def test_transform_sx_scale(self) -> None:
        """transform.sx should apply horizontal scale."""
        from pml.animation.engine import PropertyAnimation
        from pml.animation.timeline import Timeline, _apply_modifications
        from pml.graphics.objects import GraphicObject
        from pml.transform import AffineTransform

        obj = GraphicObject(shape_type="rect", params={"x": 0, "y": 0, "w": 10, "h": 10})
        mods = {"transform.sx": 2.0}
        modified = _apply_modifications(obj, mods)
        assert modified.transform.a == 2.0

    def test_transform_sy_scale(self) -> None:
        from pml.animation.timeline import _apply_modifications
        from pml.graphics.objects import GraphicObject
        from pml.transform import AffineTransform

        obj = GraphicObject(shape_type="rect", params={"x": 0, "y": 0, "w": 10, "h": 10},
                            transform=AffineTransform.translate(5, 10))
        mods = {"transform.sy": 0.5}
        modified = _apply_modifications(obj, mods)
        assert modified.transform.d == 0.5
        # Translation should be preserved
        assert modified.transform.e == 5
        assert modified.transform.f == 10

    def test_transform_rot(self) -> None:
        import math
        from pml.animation.timeline import _apply_modifications
        from pml.graphics.objects import GraphicObject
        from pml.transform import AffineTransform

        obj = GraphicObject(shape_type="rect", params={"x": 0, "y": 0, "w": 10, "h": 10})
        mods = {"transform.rot": math.pi / 2}
        modified = _apply_modifications(obj, mods)
        assert abs(modified.transform.a) < 0.001  # cos(π/2) ≈ 0
        assert abs(modified.transform.b - 1.0) < 0.001  # sin(π/2) = 1


class TestCharacterAnimationIntegration:
    """End-to-end character rendering with animation."""

    def test_render_blinking_character(self) -> None:
        import os
        import tempfile
        from pathlib import Path

        Timeline.reset()
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "blink.gif").as_posix()
            run(f"""
                (sprite-canvas 128 256 :bg "transparent")
                (define c (character :expression "neutral" :style 'cel))
                (add c)
                (blink c)
                (play)
                (render "{path}" :fps 30)
            """)
            assert os.path.exists(path)
            assert os.path.getsize(path) > 0

    def test_render_walking_character(self) -> None:
        import os
        import tempfile
        from pathlib import Path

        Timeline.reset()
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "walk.gif").as_posix()
            run(f"""
                (sprite-canvas 200 200 :bg "transparent")
                (define c (character :expression "happy" :style 'cel))
                (add c)
                (walk-cycle c :distance 60 :duration 0.8)
                (play)
                (render "{path}" :fps 30)
            """)
            assert os.path.exists(path)
            assert os.path.getsize(path) > 0

    def test_full_scene_with_animation(self) -> None:
        """Complete scene: character + blink + breath + walk, rendered as GIF."""
        import os
        import tempfile
        from pathlib import Path

        Timeline.reset()
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "scene.gif").as_posix()
            run(f"""
                (sprite-canvas 200 200 :bg "#f0f0f0")
                (define c (character :expression "happy" :style 'cel))
                (add c)
                (blink c)
                (idle-breath c)
                (play)
                (render "{path}" :fps 30)
            """)
            assert os.path.exists(path)
            assert os.path.getsize(path) > 0
