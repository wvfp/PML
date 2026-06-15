"""Tests for Phase 2+3: transforms, graphics, rendering, and sprites."""

import os
import tempfile

import pytest

from pml.graphics.objects import GraphicObject
from pml.repl import create_global_env, run_file
from pml.transform import AffineTransform
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


class TestTransform:
    def test_identity(self):
        m = AffineTransform.identity()
        assert m.a == 1 and m.d == 1 and m.e == 0

    def test_translate(self):
        m = AffineTransform.translate(10, 20)
        x, y = m.apply(0, 0)
        assert x == 10 and y == 20

    def test_scale(self):
        m = AffineTransform.scale(2, 3)
        x, y = m.apply(5, 10)
        assert x == 10 and y == 30

    def test_rotate_90(self):
        import math
        m = AffineTransform.rotate(math.pi / 2)
        x, y = m.apply(1, 0)
        assert abs(x) < 1e-9 and abs(y - 1) < 1e-9

    def test_compose(self):
        t = AffineTransform.translate(10, 0)
        s = AffineTransform.scale(2, 2)
        m = t.compose(s)  # scale first, then translate
        x, y = m.apply(5, 0)
        assert x == 20 and y == 0

    def test_inverse(self):
        m = AffineTransform.translate(10, 20)
        inv = m.inverse()
        x, y = inv.apply(10, 20)
        assert abs(x) < 1e-9 and abs(y) < 1e-9


class TestTransformBuiltins:
    def test_translate(self):
        result = run("(translate 10 20)")
        assert isinstance(result, AffineTransform)
        x, y = result.apply(0, 0)
        assert x == 10 and y == 20

    def test_scale(self):
        result = run("(scale 3 3)")
        assert isinstance(result, AffineTransform)

    def test_compose(self):
        result = run("(compose (translate 5 0) (scale 2 2))")
        assert isinstance(result, AffineTransform)

    def test_matrix_predicate(self):
        result = run("(matrix? (identity-matrix))")
        assert result is True
        result = run("(matrix? 42)")
        assert result is False

    def test_matrix_apply(self):
        result = run("(matrix-apply (translate 10 20) 0 0)")
        assert result == [10.0, 20.0]


class TestGraphicPrimitives:
    def test_circle(self):
        result = run('(circle 50 50 30 :fill "red")')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "circle"
        assert result.fill == "red"
        assert result.params["r"] == 30

    def test_rect(self):
        result = run('(rect 10 10 100 50 :fill "#FF0000" :stroke "#000000")')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "rect"
        assert result.params["w"] == 100

    def test_line(self):
        result = run('(line 0 0 100 100 :stroke "blue" :stroke-width 3)')
        assert isinstance(result, GraphicObject)
        assert result.stroke_width == 3

    def test_group(self):
        result = run('(group (circle 10 10 5 :fill "red") (rect 0 0 20 20 :fill "blue"))')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "group"
        assert len(result.children) == 2

    def test_graphic_object_predicate(self):
        result = run('(graphic-object? (circle 0 0 10))')
        assert result is True
        result = run("(graphic-object? 42)")
        assert result is False


class TestRender:
    def test_render_simple(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            from pathlib import Path
            path = Path(tmpdir, "test.png").as_posix()
            source = f"""
                (canvas 100 100 :bg "white")
                (add (circle 50 50 30 :fill "red"))
                (render "{path}")
            """
            run(source)
            assert os.path.exists(path)

    def test_render_multiple_shapes(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            from pathlib import Path
            path = Path(tmpdir, "shapes.png").as_posix()
            source = f"""
                (canvas 200 200 :bg "#f0f0f0")
                (add (rect 20 20 60 60 :fill "blue" :stroke "black"))
                (add (circle 140 100 40 :fill "#FF6347" :stroke "black" :stroke-width 2))
                (add (line 20 180 180 180 :stroke "green" :stroke-width 3))
                (render "{path}")
            """
            run(source)
            assert os.path.exists(path)

    def test_sprite_canvas_render(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            from pathlib import Path
            path = Path(tmpdir, "sprite.png").as_posix()
            source = f"""
                (sprite-canvas 64 64 :bg "transparent")
                (add (circle 32 32 20 :fill "#4a90d9"))
                (render "{path}")
            """
            run(source)
            assert os.path.exists(path)
            # Check metadata was generated
            meta_path = Path(tmpdir, "sprite.meta.json").as_posix()
            assert os.path.exists(meta_path)


class TestSpriteComponents:
    def test_body(self):
        result = run('(body :skin "#fce4c8" :build "average")')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "group"

    def test_head(self):
        result = run('(head :shape "round" :skin "#fce4c8")')
        assert isinstance(result, GraphicObject)

    def test_eyes(self):
        result = run('(anime-eyes :style "shoujo" :color "#4a90d9")')
        assert isinstance(result, GraphicObject)

    def test_mouth(self):
        result = run('(mouth :style "smile")')
        assert isinstance(result, GraphicObject)

    def test_hair(self):
        result = run('(hair :style "long" :color "#2c2c2c")')
        assert isinstance(result, GraphicObject)

    def test_character_default(self):
        result = run("(character)")
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "group"
        assert len(result.children) >= 4  # body, head, eyes+mouth, hair

    def test_character_custom(self):
        result = run("""
            (character
              :expression "happy"
              :style 'cel)
        """)
        assert isinstance(result, GraphicObject)

    def test_character_render(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            from pathlib import Path
            path = Path(tmpdir, "hero.png").as_posix()
            source = f"""
                (sprite-canvas 128 256 :bg "transparent")
                (add (character :expression "neutral" :style 'cel))
                (render "{path}")
            """
            run(source)
            assert os.path.exists(path)


class TestStyleSystem:
    def test_use_style(self):
        from pml.sprites.style import StyleDescriptor
        result = run("(use-style 'cel)")
        assert isinstance(result, StyleDescriptor)
        assert result.shading == "cel"

    def test_define_style(self):
        from pml.sprites.style import StyleDescriptor
        result = run("""
            (define-style "my-style"
              :outline-width 3.0
              :shading "gradient")
            (use-style "my-style")
        """)
        assert isinstance(result, StyleDescriptor)
        assert result.outline_width == 3.0


class TestPaletteSystem:
    def test_palette_ref(self):
        result = run("""
            (define-palette "test" (list (list "main" "#FF0000") (list "bg" "#00FF00")))
        """)
        # palette-ref needs an active palette; character sets it
        # Test that define-palette runs without error
        assert result is None
