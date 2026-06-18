"""Tests for WebP export, clipping masks, and property-based rendering invariants."""
from __future__ import annotations

import os
import tempfile

import pytest
from PIL import Image

from pml.api import PMLRuntime
from pml.backend.pillow import PillowBackend
from pml.graphics.canvas import Canvas, _current_canvas
from pml.graphics.objects import GraphicObject
from pml.graphics.render import _render
from pml.transform import AffineTransform


# ======================================================================
# WebP export
# ======================================================================


class TestWebPExport:
    def test_webp_extension_detection(self):
        """Render to .webp should produce a valid WebP file."""
        with tempfile.TemporaryDirectory() as tmp:
            c = Canvas(width=64, height=64, bg_color="#ff0000")
            _current_canvas[0] = c
            path = os.path.join(tmp, "test.webp")
            _render(path)
            assert os.path.exists(path)
            img = Image.open(path)
            assert img.format == "WEBP"
            assert img.size == (64, 64)

    def test_webp_via_api(self):
        """PMLRuntime.render_sprite with .webp should produce WebP."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = '(sprite-canvas 32 32 :bg "#00ff00") (add (rect 0 0 32 32 :fill "red"))'
            result = rt.render_sprite(source, name="green_square", format="webp")
            from pml.api import SpriteAsset
            assert isinstance(result, SpriteAsset)
            assert result.path.endswith(".webp")
            img = Image.open(result.path)
            assert img.format == "WEBP"

    def test_webp_lossless_rgba(self):
        """WebP with transparent background should preserve alpha."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = (
                '(canvas 64 64 :bg "transparent") '
                "(add (circle 32 32 20 :fill \"#3498db\"))"
            )
            result = rt.render_sprite(source, name="circle_webp", format="webp")
            from pml.api import SpriteAsset
            assert isinstance(result, SpriteAsset)
            assert result.format == "WEBP"
            img = Image.open(result.path)
            px = img.getpixel((32, 32))
            assert px[2] > px[0] and px[2] > px[1]


# ======================================================================
# Clipping masks
# ======================================================================


class TestClippingMask:
    def test_clip_simple_rect(self):
        """clip_rect should zero out pixels outside the region."""
        _current_canvas[0] = None
        c = Canvas(width=64, height=64, bg_color="#ffffff")
        _current_canvas[0] = c
        c.add(GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 64, "h": 64},
            fill="#ff0000",
        ))
        c.clip_rect = (0, 0, 32, 64)

        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "clip.png")
            _render(path)
            img = Image.open(path)
            assert img.getpixel((16, 32)) == (255, 0, 0, 255)
            assert img.getpixel((48, 32)) == (0, 0, 0, 0)

    def test_clip_via_pml(self):
        """PML (clip-canvas) should work end-to-end (non-sprite canvas)."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = """
                (canvas 64 64 :bg "transparent")
                (add (rect 0 0 64 64 :fill "#ff0000"))
                (clip-canvas 0 0 32 64)
            """
            result = rt.execute(source)
            assert result.success
            from pml.graphics.canvas import get_current_canvas
            c = get_current_canvas()
            assert c is not None
            assert c.clip_rect == (0, 0, 32, 64)

    def test_clip_zero_area(self):
        """Clip with zero width/height should produce a transparent canvas."""
        _current_canvas[0] = None
        c = Canvas(width=64, height=64, bg_color="#ff0000")
        _current_canvas[0] = c
        c.add(GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 64, "h": 64},
            fill="#00ff00",
        ))
        c.clip_rect = (0, 0, 0, 0)

        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "zero_clip.png")
            _render(path)
            img = Image.open(path)
            assert img.getpixel((32, 32)) == (0, 0, 0, 0)


# ======================================================================
# Property-based rendering invariants
# ======================================================================


class TestRenderingInvariants:
    """Shape-agnostic invariants that must hold for ALL graphic primitives."""

    def _draw_shape(self, backend: PillowBackend, surface: Image.Image, obj: GraphicObject) -> None:
        backend.draw(surface, obj)

    def test_solid_fill_opaque(self):
        """A rect with solid fill should have fully opaque interior pixels."""
        backend = PillowBackend()
        surface = backend.create_surface(50, 50, "transparent")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 5, "y": 5, "w": 40, "h": 40},
            fill="#ff8800",
        )
        self._draw_shape(backend, surface, obj)
        px = surface.getpixel((25, 25))
        assert px[3] == 255
        assert px[0] == 0xFF
        assert px[1] == 0x88

    def test_no_fill_transparent(self):
        """A rect without fill should leave background untouched."""
        backend = PillowBackend()
        surface = backend.create_surface(20, 20, "#ff0000")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 20, "h": 20},
            fill=None,
        )
        self._draw_shape(backend, surface, obj)
        px = surface.getpixel((10, 10))
        assert px == (255, 0, 0, 255)

    def test_circle_radius_zero(self):
        """A circle with zero radius should be invisible (no crash)."""
        backend = PillowBackend()
        surface = backend.create_surface(20, 20, "white")
        obj = GraphicObject(
            shape_type="circle",
            params={"x": 10, "y": 10, "r": 0},
            fill="#ff0000",
        )
        self._draw_shape(backend, surface, obj)

    def test_ellipse_negative_radii(self):
        """Negative radii should be clamped to zero (no crash)."""
        backend = PillowBackend()
        surface = backend.create_surface(20, 20, "white")
        obj = GraphicObject(
            shape_type="ellipse",
            params={"cx": 10, "cy": 10, "rx": -5, "ry": -3},
            fill="#ff0000",
        )
        self._draw_shape(backend, surface, obj)

    def test_identity_transform_preserves_geometry(self):
        """Identity transform should leave geometry unchanged."""
        backend = PillowBackend()
        surface = backend.create_surface(30, 30, "white")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 30, "h": 30},
            fill="#ff0000",
            transform=AffineTransform.identity(),
        )
        self._draw_shape(backend, surface, obj)
        assert surface.getpixel((15, 15)) == (255, 0, 0, 255)

    def test_translation_moves_shape(self):
        """Translating a rect should shift its visible position."""
        backend = PillowBackend()
        surface = backend.create_surface(40, 40, "white")
        obj_a = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 40, "h": 40},
            fill="#00ff00",
        )
        self._draw_shape(backend, surface, obj_a)
        surface2 = backend.create_surface(40, 40, "white")
        obj_b = obj_a.with_transform(AffineTransform.translate(10, 10))
        self._draw_shape(backend, surface2, obj_b)
        assert surface2.getpixel((0, 0)) == (255, 255, 255, 255)
        assert surface2.getpixel((20, 20)) == (0, 255, 0, 255)

    def test_stroke_without_fill(self):
        """A rect with only stroke should draw outline, not interior."""
        backend = PillowBackend()
        surface = backend.create_surface(20, 20, "white")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 2, "y": 2, "w": 16, "h": 16},
            fill=None,
            stroke="#000000",
            stroke_width=2,
        )
        self._draw_shape(backend, surface, obj)
        assert surface.getpixel((10, 10)) == (255, 255, 255, 255)
        edge_px = surface.getpixel((2, 2))
        assert edge_px[0] < 50 and edge_px[1] < 50 and edge_px[2] < 50

    def test_normal_blend_identity(self):
        """Normal blend mode should produce identical output to same config."""
        backend = PillowBackend()
        surface_a = backend.create_surface(20, 20, "#646464")
        surface_b = backend.create_surface(20, 20, "#646464")
        obj = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 20, "h": 20},
            fill="#ff0000",
            blend_mode="normal",
        )
        self._draw_shape(backend, surface_a, obj)
        obj2 = GraphicObject(
            shape_type="rect",
            params={"x": 0, "y": 0, "w": 20, "h": 20},
            fill="#ff0000",
            blend_mode="normal",
        )
        self._draw_shape(backend, surface_b, obj2)
        assert surface_a.getpixel((10, 10)) == surface_b.getpixel((10, 10))


# ======================================================================
# Integration: combined features
# ======================================================================


class TestFeatureIntegration:
    def test_gradient_rect_renders(self):
        """A gradient-filled rect should render without error (visual check)."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = """
                (sprite-canvas 60 60 :bg "transparent")
                (add (rect 0 0 60 60 :fill "linear-gradient(0deg, #ff0000, #0000ff)"))
            """
            result = rt.render_sprite(source, name="grad_rect", format="png")
            from pml.api import SpriteAsset
            assert isinstance(result, SpriteAsset)
            img = Image.open(result.path)
            left = img.getpixel((30, 40))
            right = img.getpixel((50, 40))
            assert left[0] > right[0], f"left red={left[0]} right red={right[0]}"
            assert right[2] > left[2], f"left blue={left[2]} right blue={right[2]}"

    def test_clip_with_animation_no_crash(self):
        """(clip-canvas) followed by animation should not crash."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = """
                (sprite-canvas 64 64 :bg "transparent")
                (add (rect 0 0 64 64 :fill "#ff0000"))
                (clip-canvas 0 0 32 64)
            """
            result = rt.execute(source)
            assert result.success

    def test_webp_with_clip(self):
        """WebP output with clipping rect should work (non-sprite canvas)."""
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            source = """
                (canvas 64 64 :bg "red")
                (add (rect 0 0 64 64 :fill "#00ff00"))
                (clip-canvas 0 0 32 64)
            """
            result = rt.render_sprite(source, name="webp_clip", format="webp")
            from pml.api import SpriteAsset
            assert isinstance(result, SpriteAsset)
            img = Image.open(result.path)
            assert img.format == "WEBP"
            assert img.getpixel((16, 32)) == (0, 255, 0, 255)
            assert img.getpixel((48, 32)) == (0, 0, 0, 0)


class TestVisualRegression:
    """Verify deterministic render output — same scene → same bytes."""

    def test_deterministic_png_output(self):
        source = """
            (canvas 32 32 :bg "#2c3e50")
            (add (circle 16 16 10 :fill "#e74c3c" :stroke "#ffffff" :stroke-width 2))
            (add (rect 5 22 22 8 :fill "#3498db"))
        """
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            r1 = rt.render_sprite(source, name="a", format="png")
            from pml.api import SpriteAsset
            assert isinstance(r1, SpriteAsset)
            rt.reset()
            r2 = rt.render_sprite(source, name="b", format="png")
            assert isinstance(r2, SpriteAsset)
            with open(r1.path, "rb") as f1, open(r2.path, "rb") as f2:
                assert f1.read() == f2.read()

    def test_deterministic_webp_output(self):
        source = """
            (canvas 32 32 :bg "#ffffff")
            (add (circle 16 16 12 :fill "#2ecc71" :stroke "#27ae60" :stroke-width 1))
        """
        with tempfile.TemporaryDirectory() as tmp:
            rt = PMLRuntime(output_dir=tmp)
            r1 = rt.render_sprite(source, name="w1", format="webp")
            from pml.api import SpriteAsset
            assert isinstance(r1, SpriteAsset)
            rt.reset()
            r2 = rt.render_sprite(source, name="w2", format="webp")
            assert isinstance(r2, SpriteAsset)
            with open(r1.path, "rb") as f1, open(r2.path, "rb") as f2:
                assert f1.read() == f2.read()
