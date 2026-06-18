"""Tests for layout system: auto-fit canvas, relative positioning, templates."""

from __future__ import annotations

import os
import tempfile

import pytest

from pml.api import PMLRuntime, RenderResult, SpriteAsset
from pml.graphics.canvas import _current_canvas, get_current_canvas
from pml.graphics.layout import (
    LayoutBox,
    resolve_relative_positioning,
    update_tagged,
)
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# Auto-fit canvas
# ======================================================================


class TestAutoFit:
    def test_auto_fit_explicit_sizing_still_works(self):
        """Canvas with explicit w/h should not auto-fit."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 128 256 :bg "#333")
                (add (rect 0 0 80 60 :fill "#e74c3c"))
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert canvas.width == 128
            assert canvas.height == 256
            assert canvas.auto_fit is False

    def test_auto_fit_zero_dim(self):
        """Canvas with 0,0 dims should set auto_fit flag."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 0 0 :bg "transparent")
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert canvas.auto_fit is True

    def test_auto_fit_omitted_dim(self):
        """Canvas with no w/h args should set auto_fit flag."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas :bg "#eee")
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert canvas.auto_fit is True

    def test_auto_fit_resizes_to_content(self):
        """Auto-fit should resize canvas to wrap content bounding box."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            sprite = rt.render_sprite("""
                (sprite-canvas :bg "transparent")
                (add (rect 0 0 100 80 :fill "#e74c3c"))
            """)
            assert isinstance(sprite, SpriteAsset), f"Expected SpriteAsset, got {sprite}"
            assert os.path.isfile(sprite.path), f"Output file not found: {sprite.path}"
            assert sprite.width > 0, "Auto-fit should produce non-zero width"
            assert sprite.height > 0, "Auto-fit should produce non-zero height"

    def test_auto_fit_multi_object(self):
        """Auto-fit with multiple objects should compute combined bounds."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            sprite = rt.render_sprite("""
                (sprite-canvas :bg "transparent")
                (add (rect 0 0 50 50 :fill "#e74c3c"))
                (add (rect 80 60 40 30 :fill "#3498db"))
            """)
            assert isinstance(sprite, SpriteAsset), f"Expected SpriteAsset, got {sprite}"
            assert os.path.isfile(sprite.path)
            # Should cover both objects: max_x=80+40=120, max_y=60+30=90
            assert sprite.width >= 120, f"Expected width >= 120, got {sprite.width}"
            assert sprite.height >= 90, f"Expected height >= 90, got {sprite.height}"

    def test_auto_fit_empty_canvas_no_crash(self):
        """Auto-fit with no content should not crash, produce fallback dims."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas :bg "transparent")
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None


class TestAutoFitSpriteRender:
    def test_auto_fit_render_sprite_api(self):
        """render_sprite should work with auto-fit canvas."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            sprite = rt.render_sprite(
                '(sprite-canvas :bg "transparent")\n(add (rect 0 0 64 64 :fill "#e74c3c"))',
                name="auto-fit-sprite",
            )
            assert isinstance(sprite, SpriteAsset), f"Expected SpriteAsset, got {sprite}"
            assert os.path.isfile(sprite.path), f"File not found: {sprite.path}"


# ======================================================================
# Relative positioning
# ======================================================================


class TestRelativePositioning:
    def test_place_right_of_tag(self):
        """Place right of a tagged element should set correct x transform."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 200 100 :bg "#f0f0f0")
                (add (place (rect 0 0 40 40 :fill "#e74c3c") :tag "box1"))
                (add (place (rect 0 0 40 40 :fill "#3498db") :right-of "box1"))
                (render "right-of.png")
            """)
            assert result.success, result.error

    def test_place_right_of_with_gap(self):
        """Place right-of with gap should add spacing."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 200 100 :bg "#f0f0f0")
                (add (place (rect 0 0 40 40 :fill "#e74c3c") :tag "a"))
                (add (place (rect 0 0 30 30 :fill "#3498db") :right-of "a:10"))
                (render "right-of-gap.png")
            """)
            assert result.success, result.error

    def test_place_below_tag(self):
        """Place below a tagged element should set correct y transform."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 100 200 :bg "#f0f0f0")
                (add (place (rect 0 0 40 40 :fill "#e74c3c") :tag "first"))
                (add (place (rect 0 0 40 40 :fill "#2ecc71") :below "first"))
                (render "below.png")
            """)
            assert result.success, result.error

    def test_place_center(self):
        """Place with center should center within canvas."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 120 120 :bg "#f0f0f0")
                (add (place (circle 0 0 30 :fill "#9b59b6") :center #t))
                (render "center.png")
            """)
            assert result.success, result.error

    def test_place_center_x(self):
        """Place with center-x should center horizontally only."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 200 100 :bg "#f0f0f0")
                (add (place (rect 0 0 80 40 :fill "#e67e22") :center-x #t))
                (render "center-x.png")
            """)
            assert result.success, result.error

    def test_place_tag_and_last_default(self):
        """Implicit :last tag should track most recent element."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 300 100 :bg "#f0f0f0")
                (add (rect 0 0 40 40 :fill "#e74c3c"))
                (add (place (rect 0 0 40 40 :fill "#3498db") :right-of ":last"))
                (add (place (rect 0 0 40 40 :fill "#2ecc71") :right-of ":last"))
                (render "last-chain.png")
            """)
            assert result.success, result.error

    def test_chained_relative_positioning(self):
        """Chain right-of and below for grid-like layout."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 160 160 :bg "#fefefe")
                (add (place (rect 0 0 40 40 :fill "#e74c3c") :tag "tl"))
                (add (place (rect 0 0 40 40 :fill "#3498db") :right-of "tl:10" :tag "tr"))
                (add (place (rect 0 0 40 40 :fill "#2ecc71") :below "tl:10" :tag "bl"))
                (add (place (rect 0 0 40 40 :fill "#f39c12") :right-of "bl:10"))
                (render "grid-layout.png")
            """)
            assert result.success, result.error


# ======================================================================
# Template system
# ======================================================================


class TestTemplates:
    def test_list_templates(self):
        """list-templates should discover built-in templates."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(list-templates)")
            assert result.success, result.error
            templates = result.value
            assert isinstance(templates, list), f"Expected list, got {type(templates)}"
            names = [t["name"] for t in templates]
            assert "portrait" in names, f"portrait not found in {names}"
            assert "item-card" in names, f"item-card not found in {names}"
            assert "battle-scene" in names, f"battle-scene not found in {names}"

    def test_list_templates_by_category(self):
        """list-templates should filter by category."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute('(list-templates "character")')
            assert result.success, result.error
            templates = result.value
            assert isinstance(templates, list)
            for t in templates:
                assert t["category"] == "character"

    def test_list_templates_no_match_category(self):
        """list-templates with no-matches category should return empty list."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute('(list-templates "nonexistent")')
            assert result.success, result.error
            assert result.value == []

    def test_template_has_description(self):
        """Templates should have non-empty descriptions."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(list-templates)")
            assert result.success, result.error
            for t in result.value:
                assert t["description"], f"Template {t['name']} has no description"

    def test_template_has_category(self):
        """Templates should have category assigned."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(list-templates)")
            assert result.success, result.error
            for t in result.value:
                categories = {"character", "scene", "items", "general"}
                assert t["category"] in categories, (
                    f"Template {t['name']} has unexpected category: {t['category']}"
                )

    def test_use_template_exists(self):
        """use-template should be a registered builtin."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute('(use-template "portrait")')
            # May fail at runtime (template needs sprite components),
            # but should not be an unbound variable error
            if not result.success:
                err_type = result.error["type"] if result.error else "unknown"
                assert err_type != "UnboundVariableError", (
                    f"use-template is not registered: {result.error}"
                )


# ======================================================================
# LayoutBox helper tests
# ======================================================================


class TestLayoutBoxHelpers:
    def test_is_layoutbox(self):
        """is_layoutbox helper should work correctly."""
        from pml.graphics.layout import is_layoutbox

        go = GraphicObject(shape_type="rect", params={"w": 40, "h": 40})
        lb = LayoutBox(graphic=go)
        assert is_layoutbox(lb) is True
        assert is_layoutbox(go) is False
        assert is_layoutbox("not-a-box") is False

    def test_resolve_relative_center(self):
        """resolve_relative_positioning with center should center in canvas."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas.width = 200
        canvas.height = 100

        go = GraphicObject(shape_type="rect", params={"w": 40, "h": 40})
        lb = LayoutBox(graphic=go, center_x=True, center_y=True)
        w, h = 40, 40  # _intrinsic_size

        result = resolve_relative_positioning(lb, canvas)
        assert isinstance(result, GraphicObject)
        assert result.transform.e == (200 - 40) // 2  # center x
        assert result.transform.f == (100 - 40) // 2  # center y

    def test_resolve_relative_right_of_tag(self):
        """resolve_relative_positioning with right-of should compute x from tag."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas._tagged = {"ref": (10, 20, 50, 30)}

        go = GraphicObject(shape_type="rect", params={"w": 30, "h": 20})
        lb = LayoutBox(graphic=go, right_of="ref")

        result = resolve_relative_positioning(lb, canvas)
        assert isinstance(result, GraphicObject)
        assert result.transform.e == 10 + 50  # ref.x + ref.w
        assert result.transform.f == 20  # ref.y

    def test_resolve_relative_right_of_with_gap(self):
        """right-of with gap should include spacing."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas._tagged = {"ref": (10, 20, 50, 30)}

        go = GraphicObject(shape_type="rect", params={"w": 30, "h": 20})
        lb = LayoutBox(graphic=go, right_of="ref:15")

        result = resolve_relative_positioning(lb, canvas)
        assert result.transform.e == 10 + 50 + 15  # ref.x + ref.w + gap

    def test_resolve_relative_below_tag(self):
        """below should compute y from tag."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas._tagged = {"ref": (10, 20, 50, 30)}

        go = GraphicObject(shape_type="rect", params={"w": 30, "h": 20})
        lb = LayoutBox(graphic=go, below="ref")

        result = resolve_relative_positioning(lb, canvas)
        assert result.transform.e == 10  # ref.x
        assert result.transform.f == 20 + 30  # ref.y + ref.h

    def test_update_tagged_registers_by_name(self):
        """update_tagged should register tag with correct bounds."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas._tagged = {}
        canvas._last_position = (0, 0)

        go = GraphicObject(shape_type="rect", params={"w": 40, "h": 30})
        go = go.with_transform(AffineTransform.translate(15, 25))
        lb = LayoutBox(graphic=go, tag="mytag")

        update_tagged(canvas, lb, go)

        tag_bounds = canvas._tagged.get("mytag")
        assert tag_bounds is not None
        assert tag_bounds == (15, 25, 40, 30)

    def test_update_tagged_last_position(self):
        """update_tagged should update :last and _last_position."""
        from unittest.mock import MagicMock

        canvas = MagicMock()
        canvas._tagged = {}
        canvas._last_position = (0, 0)

        go = GraphicObject(shape_type="rect", params={"w": 50, "h": 40})
        go = go.with_transform(AffineTransform.translate(5, 10))
        lb = LayoutBox(graphic=go)

        update_tagged(canvas, lb, go)

        assert canvas._last_position == (5 + 50, 10 + 40)  # tx+w, ty+h
        last_bounds = canvas._tagged.get(":last")
        assert last_bounds == (5, 10, 50, 40)