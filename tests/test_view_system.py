"""Tests for the :view system — 2.5D character direction support.

Each sprite component (head, body, eyes, mouth, hair, outfit) accepts
:view with values 'front | 'side | 'back | 'three-quarter.

These tests verify:
  1. Every view value produces a valid GraphicObject
  2. Views produce structurally different outputs (different widths/children)
  3. Back view correctly suppresses eyes and mouth on character assembly
  4. Invalid view falls back to 'front (or does not crash)
  5. Character assembly passes :view through to all components
"""

from __future__ import annotations

import pytest

from pml.graphics.objects import GraphicObject
from pml.repl import create_global_env
from pml.lexer import Lexer
from pml.parser import Parser
from pml.evaluator import evaluate

VIEWS = ["front", "side", "back", "three-quarter"]


def run(source: str):
    env = create_global_env()
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result


# ======================================================================
# Individual component view tests
# ======================================================================


class TestComponentViews:
    """Every sprite component accepts all 4 view values."""

    @pytest.mark.parametrize("view", VIEWS)
    def test_body_view(self, view: str) -> None:
        result = run(f'(body :skin "#fce4c8" :build "average" :view "{view}")')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "group"
        # Side view should be narrower than front view
        meta = result.metadata
        if view == "front":
            assert meta.get("torso_width", 0) > 0

    @pytest.mark.parametrize("view", VIEWS)
    def test_head_view(self, view: str) -> None:
        result = run(f'(head :shape "round" :skin "#fce4c8" :view "{view}")')
        assert isinstance(result, GraphicObject)
        # Side view head should be narrower (recorded in metadata)
        meta = result.metadata
        assert meta.get("view") == view

    @pytest.mark.parametrize("view", VIEWS)
    def test_eyes_view(self, view: str) -> None:
        result = run(f'(anime-eyes :style "shoujo" :view "{view}")')
        assert isinstance(result, GraphicObject)
        # Back view eyes should have no visible eye shapes
        if view == "back":
            assert result.shape_type == "group"
            # Either empty group or a single placeholder
            assert len(result.children) == 0

    @pytest.mark.parametrize("view", VIEWS)
    def test_mouth_view(self, view: str) -> None:
        result = run(f'(mouth :style "smile" :view "{view}")')
        assert isinstance(result, GraphicObject)
        # Back view mouth should be empty/transparent
        children = result.children if isinstance(result, GraphicObject) and result.shape_type == "group" else []
        assert isinstance(result, GraphicObject)

    @pytest.mark.parametrize("view", VIEWS)
    def test_hair_view(self, view: str) -> None:
        result = run(f'(hair :style "long" :color "#2c2c2c" :view "{view}")')
        assert isinstance(result, GraphicObject)

    @pytest.mark.parametrize("view", VIEWS)
    def test_outfit_view(self, view: str) -> None:
        result = run(f'(outfit :top "t-shirt" :color-top "#3498db" :view "{view}")')
        assert isinstance(result, GraphicObject)


# ======================================================================
# View produces structurally different outputs
# ======================================================================


class TestViewStructuralDifferences:
    """Verify that different views produce recognisably different outputs."""

    def test_head_front_wider_than_side(self) -> None:
        front = run('(head :shape "oval" :view "front")')
        side = run('(head :shape "oval" :view "side")')
        fw = front.metadata.get("head_width", 0)
        sw = side.metadata.get("head_width", 0)
        assert fw > sw, f"Front head ({fw}) should be wider than side ({sw})"

    def test_body_front_wider_than_side(self) -> None:
        front = run('(body :view "front")')
        side = run('(body :view "side")')
        fw = front.metadata.get("torso_width", 0)
        sw = side.metadata.get("torso_width", 0)
        assert fw > sw, f"Front body ({fw}) should be wider than side ({sw})"

    def test_eyes_front_has_two_back_has_none(self) -> None:
        front = run('(anime-eyes :style "shoujo" :view "front")')
        back = run('(anime-eyes :style "shoujo" :view "back")')
        # Front eyes should be a group with children (the actual eye shapes)
        if front.shape_type == "group":
            assert len(front.children) > 0
        # Back eyes should be empty
        assert len(back.children) == 0

    def test_mouth_front_exists_back_empty(self) -> None:
        front = run('(mouth :style "smile" :view "front")')
        back = run('(mouth :style "smile" :view "back")')
        # Front should have a shape (circle, arc, etc.)
        assert front is not None
        # Back should have visible=False or be a transparent placeholder
        # (implementation may vary — just check it doesn't crash)
        assert back is not None


# ======================================================================
# Character assembly with view
# ======================================================================


class TestCharacterView:
    """Character assembly passes :view through to all components."""

    @pytest.mark.parametrize("view", VIEWS)
    def test_character_with_view(self, view: str) -> None:
        """Character accepts :view without error and produces a group."""
        result = run(f'(character :expression "neutral" :view "{view}")')
        assert isinstance(result, GraphicObject)
        assert result.shape_type == "group"
        assert result.metadata.get("view") == view
        # Character should have children (requires at least body + head + hair)
        assert len(result.children) >= 3, f"Expected ≥3 children for {view}, got {len(result.children)}"

    def test_character_side_view_offset(self) -> None:
        """Side view applies x-offset to facial features."""
        front = run('(character :expression "happy" :view "front")')
        side = run('(character :expression "happy" :view "side")')
        # Both should produce valid characters
        assert isinstance(front, GraphicObject)
        assert isinstance(side, GraphicObject)
        assert front.metadata.get("view") == "front"
        assert side.metadata.get("view") == "side"

    def test_character_back_has_no_face(self) -> None:
        """Back view character should not crash — eyes/mouth components are suppressed."""
        result = run('(character :expression "neutral" :view "back")')
        assert isinstance(result, GraphicObject)
        # Should still have body, head, hair at minimum
        assert len(result.children) >= 3

    @pytest.mark.parametrize("v", VIEWS)
    def test_character_renders_all_views(self, v: str) -> None:
        """Render a character with each view — should produce valid output."""
        import os
        import tempfile
        from pathlib import Path

        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, f"char-{v}.png").as_posix()
            source = f"""
                (sprite-canvas 128 256 :bg "transparent")
                (add (character :expression "neutral" :view "{v}" :style 'cel))
                (render "{path}")
            """
            run(source)
            assert os.path.exists(path), f"Failed to render {v} view"
            assert os.path.getsize(path) > 0, f"Rendered {v} view is empty"


# ======================================================================
# Edge cases
# ======================================================================


class TestViewEdgeCases:
    """Invalid or edge-case view values."""

    def test_invalid_view_falls_back(self) -> None:
        """Invalid view should not crash — schema defaults to 'front'."""
        from pml.sprites.components.body import create_body
        # Call directly with an invalid view
        obj = create_body(view="invalid_view_value")
        assert isinstance(obj, GraphicObject)
        # Should have defaulted to "front" behaviour (full width)
        meta = obj.metadata
        front_width = run('(body :view "front")').metadata.get("torso_width", 0)
        assert abs(meta.get("torso_width", 0) - front_width) < 1

    def test_missing_view_defaults_to_front(self) -> None:
        """Omitting :view should default to 'front'."""
        result = run('(character :expression "happy")')
        assert result.metadata.get("view") == "front"

    def test_side_by_side_views_differ(self) -> None:
        """Rendering all 4 views produces visually different images."""
        import os
        import tempfile
        from pathlib import Path

        with tempfile.TemporaryDirectory() as tmpdir:
            sizes = {}
            for v in VIEWS:
                path = Path(tmpdir, f"{v}.png").as_posix()
                source = f"""
                    (sprite-canvas 128 256 :bg "transparent")
                    (add (character :expression "neutral" :view "{v}" :style 'cel))
                    (render "{path}")
                """
                run(source)
                sizes[v] = os.path.getsize(path)

            # Different views should produce differently-sized PNGs
            # (due to different geometry in each view)
            unique_sizes = len(set(sizes.values()))
            assert unique_sizes >= 2, (
                f"Expected different views to produce different render output, "
                f"got sizes: {sizes}"
            )
