"""Tests for bezier curve path parsing and gradient fills."""

from __future__ import annotations

import math
import tempfile

import pytest
from PIL import Image

from pml.backend.pillow import (
    _parse_svg_path,
    _cubic_bezier_segment,
    _quad_bezier_segment,
    _is_gradient_fill,
    _parse_gradient_spec,
    _make_linear_gradient_image,
    _make_radial_gradient_image,
    _apply_gradient_fill,
    PillowBackend,
)
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# Bezier subdivision
# ======================================================================


class TestBezierSubdivision:
    def test_cubic_bezier_straight_line(self):
        """A cubic bezier with colinear control points should approximate a line."""
        pts = _cubic_bezier_segment((0, 0), (25, 0), (75, 0), (100, 0), steps=10)
        assert len(pts) == 11
        # First point is start
        assert pts[0] == (0.0, 0.0)
        # Last point is end
        assert pts[-1] == (100.0, 0.0)
        # All points should be roughly on the line y=0
        for x, y in pts:
            assert abs(y) < 0.001

    def test_cubic_bezier_endpoints(self):
        """Start and end should match exactly."""
        pts = _cubic_bezier_segment((10, 20), (30, 80), (70, 80), (90, 20))
        assert pts[0] == (10.0, 20.0)
        assert pts[-1] == (90.0, 20.0)

    def test_quad_bezier_straight_line(self):
        pts = _quad_bezier_segment((0, 0), (50, 0), (100, 0), steps=10)
        assert len(pts) == 11
        assert pts[0] == (0.0, 0.0)
        assert pts[-1] == (100.0, 0.0)
        for x, y in pts:
            assert abs(y) < 0.001

    def test_quad_bezier_endpoints(self):
        pts = _quad_bezier_segment((10, 10), (50, 80), (90, 10))
        assert pts[0] == (10.0, 10.0)
        assert pts[-1] == (90.0, 10.0)


# ======================================================================
# SVG Path parsing
# ======================================================================


class TestSVGPathParsing:
    def test_simple_moveto_lineto(self):
        """M + L commands produce a single polyline."""
        result = _parse_svg_path("M 10 10 L 50 10 L 30 50")
        assert len(result) == 1
        assert len(result[0]) == 3
        assert result[0][0] == (10, 10)
        assert result[0][1] == (50, 10)
        assert result[0][2] == (30, 50)

    def test_closepath_z(self):
        """Z closes the path by adding the first point."""
        result = _parse_svg_path("M 10 10 L 50 10 L 30 50 Z")
        assert len(result) == 1
        assert len(result[0]) == 4  # 3 original + 1 close
        assert result[0][0] == result[0][-1]  # start == end after close

    def test_cubic_bezier_C(self):
        """C command produces a polyline with 21 points (step=20 + 1)."""
        result = _parse_svg_path("M 10 10 C 20 20 40 20 50 10")
        assert len(result) == 1
        # Cubic bezier with default steps=20 produces 21 points
        assert len(result[0]) == 21
        assert result[0][0] == (10, 10)
        assert result[0][-1] == (50, 10)

    def test_smooth_cubic_S(self):
        """S command reflects previous control point from C."""
        result = _parse_svg_path("M 10 10 C 20 20 40 20 50 10 S 70 0 80 10")
        assert len(result) == 1
        # 21 + 21 points (minus shared endpoint)
        assert 21 <= len(result[0]) <= 42
        assert result[0][0] == (10, 10)
        assert result[0][-1] == (80, 10)

    def test_quadratic_Q(self):
        """Q command produces a polyline with 17 points (step=16 + 1)."""
        result = _parse_svg_path("M 10 10 Q 50 50 90 10")
        assert len(result) == 1
        assert len(result[0]) == 17
        assert result[0][0] == (10, 10)
        assert result[0][-1] == (90, 10)

    def test_smooth_quadratic_T(self):
        """T command reflects previous control point from Q."""
        result = _parse_svg_path("M 10 10 Q 50 50 90 10 T 170 10")
        assert len(result) == 1
        assert 17 <= len(result[0]) <= 34
        assert result[0][0] == (10, 10)
        assert result[0][-1] == (170, 10)

    def test_relative_cubic_c(self):
        """Lowercase c uses relative coordinates."""
        result = _parse_svg_path("M 10 10 c 10 10 30 10 40 0")
        assert len(result) == 1
        assert len(result[0]) == 21
        assert result[0][-1] == (50, 10)  # 10 + 40

    def test_horizontal_vertical_H_V(self):
        result = _parse_svg_path("M 10 10 H 50 V 50")
        assert len(result) == 1
        assert len(result[0]) == 3
        assert result[0][1] == (50, 10)
        assert result[0][2] == (50, 50)

    def test_implicit_lineto_after_M(self):
        """Consecutive coordinate pairs after M are treated as L."""
        result = _parse_svg_path("M 10 10 50 10 30 50")
        assert len(result) == 1
        assert len(result[0]) == 3

    def test_multiple_subpaths(self):
        """M can start a new subpath."""
        result = _parse_svg_path("M 10 10 L 50 10 M 100 100 L 150 100")
        assert len(result) == 2
        assert len(result[0]) == 2
        assert len(result[1]) == 2

    def test_complex_path(self):
        """A more realistic path mixing bezier and straight segments."""
        path = "M 10 35 C 10 15 40 15 40 35 S 70 55 70 35"
        result = _parse_svg_path(path)
        assert len(result) == 1
        assert len(result[0]) >= 21


# ======================================================================
# Gradient parsing
# ======================================================================


class TestGradientParsing:
    def test_linear_basic(self):
        g = _parse_gradient_spec("linear-gradient(45deg, #ff0000, #0000ff)")
        assert g is not None
        assert g["type"] == "linear"
        assert g["angle"] == 45.0
        assert len(g["stops"]) == 2
        assert g["stops"][0][1] == "#ff0000"
        assert g["stops"][1][1] == "#0000ff"

    def test_linear_three_stops(self):
        g = _parse_gradient_spec("linear-gradient(90deg, #ff0000, #00ff00, #0000ff)")
        assert g is not None
        assert len(g["stops"]) == 3

    def test_linear_no_angle(self):
        g = _parse_gradient_spec("linear-gradient(#ff0000, #0000ff)")
        assert g is not None
        assert g["type"] == "linear"
        assert g["angle"] == 0.0
        assert len(g["stops"]) == 2

    def test_radial_basic(self):
        g = _parse_gradient_spec("radial-gradient(#ff0000, #0000ff)")
        assert g is not None
        assert g["type"] == "radial"
        assert len(g["stops"]) == 2

    def test_linear_with_positions(self):
        g = _parse_gradient_spec("linear-gradient(90deg, #ff0000 0%, #00ff00 50%, #0000ff 100%)")
        assert g is not None
        assert len(g["stops"]) == 3
        assert g["stops"][0][0] == 0.0
        assert g["stops"][1][0] == 0.5
        assert g["stops"][2][0] == 1.0

    def test_is_gradient_fill_true(self):
        assert _is_gradient_fill("linear-gradient(#fff, #000)")

    def test_is_gradient_fill_false_plain(self):
        assert not _is_gradient_fill("#ff0000")

    def test_is_gradient_fill_false_none(self):
        assert not _is_gradient_fill(None)

    def test_is_gradient_fill_false_empty(self):
        assert not _is_gradient_fill("")


# ======================================================================
# Gradient image rendering
# ======================================================================


class TestGradientRendering:
    def test_linear_gradient_size(self):
        img = _make_linear_gradient_image(100, 50, 0, [(0.0, "#ff0000"), (1.0, "#0000ff")])
        assert img.size == (100, 50)
        assert img.mode == "RGBA"

    def test_linear_gradient_horizontal(self):
        """Horizontal gradient: left=red, right=blue."""
        img = _make_linear_gradient_image(100, 50, 0, [(0.0, "#ff0000"), (1.0, "#0000ff")])
        left_px = img.getpixel((0, 25))
        right_px = img.getpixel((99, 25))
        # Left should be more red, right more blue
        assert left_px[0] > right_px[0], f"left red={left_px[0]} right red={right_px[0]}"
        assert right_px[2] > left_px[2], f"left blue={left_px[2]} right blue={right_px[2]}"

    def test_linear_gradient_vertical(self):
        """Vertical gradient: top=red, bottom=blue."""
        img = _make_linear_gradient_image(50, 100, 90, [(0.0, "#ff0000"), (1.0, "#0000ff")])
        top_px = img.getpixel((25, 0))
        bottom_px = img.getpixel((25, 99))
        assert top_px[0] > bottom_px[0]
        assert bottom_px[2] > top_px[2]

    def test_radial_gradient_size(self):
        img = _make_radial_gradient_image(100, 50, [(0.0, "#ff0000"), (1.0, "#0000ff")])
        assert img.size == (100, 50)
        assert img.mode == "RGBA"

    def test_radial_gradient_center(self):
        """Radial gradient: center=red, edge=blue."""
        img = _make_radial_gradient_image(100, 100, [(0.0, "#ff0000"), (1.0, "#0000ff")])
        center_px = img.getpixel((50, 50))
        edge_px = img.getpixel((99, 50))
        assert center_px[0] > edge_px[0]
        assert edge_px[2] > center_px[2]

    def test_empty_size(self):
        img = _make_linear_gradient_image(0, 0, 0, [(0.0, "#fff"), (1.0, "#000")])
        assert img.size[0] >= 1

    def test_single_stop_gradient(self):
        """Single stop should fill with that color."""
        img = _make_linear_gradient_image(10, 10, 0, [(0.0, "#ff0000")])
        px = img.getpixel((5, 5))
        assert px[0] > 200 and px[1] < 50 and px[2] < 50


# ======================================================================
# End-to-end: gradient fills on shapes
# ======================================================================


class TestGradientFillOnShapes:
    def _make_backend_and_surface(self, w=100, h=100, bg="white"):
        backend = PillowBackend()
        surface = backend.create_surface(w, h, bg)
        return backend, surface

    def test_rect_linear_gradient(self):
        backend, surface = self._make_backend_and_surface(100, 50)
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 100, "h": 50},
            fill="linear-gradient(0deg, #ff0000, #0000ff)",
        )
        backend.draw(surface, obj)
        # Left pixel should be red, right pixel should be blue
        left = surface.getpixel((5, 25))
        right = surface.getpixel((95, 25))
        assert left[0] > right[0], f"left red={left[0]} right red={right[0]}"
        assert right[2] > left[2], f"left blue={left[2]} right blue={right[2]}"

    def test_circle_radial_gradient(self):
        backend, surface = self._make_backend_and_surface(100, 100)
        obj = GraphicObject(
            shape_type="circle",
            params={"x": 50, "y": 50, "r": 40},
            fill="radial-gradient(#ff0000, #0000ff)",
        )
        backend.draw(surface, obj)
        # Center should be red, edge should be blue
        center = surface.getpixel((50, 50))
        edge = surface.getpixel((85, 50))
        assert center[0] > edge[0], f"center red={center[0]} edge red={edge[0]}"
        assert edge[2] > center[2], f"center blue={center[2]} edge blue={edge[2]}"

    def test_ellipse_linear_gradient(self):
        backend, surface = self._make_backend_and_surface(100, 80)
        obj = GraphicObject(
            shape_type="ellipse",
            params={"cx": 50, "cy": 40, "rx": 40, "ry": 30},
            fill="linear-gradient(90deg, #00ff00, #0000ff)",
        )
        backend.draw(surface, obj)
        # Top should be green, bottom should be blue (90deg = vertical)
        top = surface.getpixel((50, 12))
        bottom = surface.getpixel((50, 68))
        assert top[1] > bottom[1], f"top green={top[1]} bottom green={bottom[1]}"
        assert bottom[2] > top[2], f"top blue={top[2]} bottom blue={bottom[2]}"

    def test_polygon_gradient(self):
        backend, surface = self._make_backend_and_surface(100, 100)
        obj = GraphicObject(
            shape_type="polygon",
            params={"points": [(50, 10), (90, 80), (10, 80)]},
            fill="linear-gradient(0deg, #ff0000, #0000ff)",
        )
        backend.draw(surface, obj)
        # Check inside the triangle at y=50 (triangle spans x≈27 to x≈73)
        left = surface.getpixel((35, 50))
        right = surface.getpixel((65, 50))
        assert left[0] > right[0], f"left red={left[0]} right red={right[0]}"
        assert right[2] > left[2], f"left blue={left[2]} right blue={right[2]}"

    def test_path_gradient(self):
        backend, surface = self._make_backend_and_surface(100, 100)
        obj = GraphicObject(
            shape_type="path",
            params={"d": "M 10 10 L 90 10 L 90 90 L 10 90 Z"},
            fill="linear-gradient(45deg, #ff0000, #0000ff)",
            stroke="#000000",
        )
        backend.draw(surface, obj)
        # Should have visible content
        center = surface.getpixel((50, 50))
        assert center[3] > 0

    def test_solid_fill_still_works(self):
        """Existing solid color fills should be unaffected."""
        backend, surface = self._make_backend_and_surface(100, 50)
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 100, "h": 50},
            fill="#ff0000",
        )
        backend.draw(surface, obj)
        px = surface.getpixel((50, 25))
        assert px == (255, 0, 0, 255)

    def test_gradient_with_stroke(self):
        """Gradient fill should not interfere with stroke rendering."""
        backend, surface = self._make_backend_and_surface(100, 100, "white")
        obj = GraphicObject(
            shape_type="circle",
            params={"x": 50, "y": 50, "r": 40},
            fill="linear-gradient(0deg, #ff0000, #0000ff)",
            stroke="#000000",
            stroke_width=3.0,
        )
        backend.draw(surface, obj)
        # Stroke should be visible at edge
        edge = surface.getpixel((10, 50))
        # Stroke is black, so RGB should be low
        assert edge[0] < 100 or edge[2] < 100

    def test_rotated_rect_gradient(self):
        """Rotated rectangle with gradient should not crash."""
        backend, surface = self._make_backend_and_surface(100, 100, "white")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 25, "y": 25, "w": 50, "h": 50},
            fill="linear-gradient(0deg, #ff0000, #0000ff)",
            transform=AffineTransform.rotate(45),
        )
        backend.draw(surface, obj)
        center = surface.getpixel((50, 50))
        assert center[3] > 0


# ======================================================================
# End-to-end: PML integration with gradient and bezier
# ======================================================================


class TestPMLIntegration:
    def _run(self, source: str):
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

    def test_gradient_builtin(self):
        """The (gradient ...) builtin should return a valid gradient string."""
        result, env = self._run('(gradient :type \'linear :angle 45 :stops \'((0 "#ff0000") (1 "#0000ff")))')
        assert isinstance(result, str)
        assert "linear-gradient" in result
        assert "#ff0000" in result

    def test_gradient_builtin_radial(self):
        result, env = self._run('(gradient :type \'radial :stops \'((0 "#ff0000") (1 "#0000ff")))')
        assert isinstance(result, str)
        assert "radial-gradient" in result

    def test_gradient_builtin_errors_no_stops(self):
        with pytest.raises(Exception):
            self._run('(gradient :type \'linear :stops \'((0 "#ff0000")))')

    def test_canvas_with_gradient_pml(self):
        """Full pipeline: define gradient, use as fill on rect, render to file."""
        import tempfile, os
        with tempfile.TemporaryDirectory() as tmpdir:
            pml_path = os.path.join(tmpdir, "grad_test.png").replace("\\", "/")
            source = (
                '(canvas 100 50 :bg "white")\n'
                '(define g (gradient :type \'linear :angle 0 :stops \'((0 "#ff0000") (1 "#0000ff"))))\n'
                '(add (rect 0 0 100 50 :fill g))\n'
                f'(render "{pml_path}")\n'
            )
            result, env = self._run(source)
            assert os.path.exists(pml_path)
            img = Image.open(pml_path)
            left = img.getpixel((5, 25))
            right = img.getpixel((95, 25))
            assert left[0] > right[0]

    def test_bezier_path_pml(self):
        """Render a bezier path through PML to verify no crash."""
        import tempfile, os
        with tempfile.TemporaryDirectory() as tmpdir:
            pml_path = os.path.join(tmpdir, "bezier_test.png").replace("\\", "/")
            source = (
                '(canvas 100 100 :bg "white")\n'
                '(add (path "M 10 50 C 30 10 70 10 90 50" :fill "none" :stroke "#ff0000" :stroke-width 2))\n'
                f'(render "{pml_path}")\n'
            )
            result, env = self._run(source)
            assert os.path.exists(pml_path)
            img = Image.open(pml_path)
            # Should have visible red pixels
            has_red = False
            for y in range(100):
                for x in range(100):
                    px = img.getpixel((x, y))
                    if px[0] > 200 and px[1] < 50 and px[2] < 50:
                        has_red = True
                        break
                if has_red:
                    break
            assert has_red, "Bezier path should render visible red pixels"
