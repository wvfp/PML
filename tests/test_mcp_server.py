"""Tests for the PML MCP server — tool contracts and error handling.

The MCP server exposes 5 tools for AI agents:
  - execute_pml(source) → JSON result
  - render_sprite(source, name) → JSON result
  - validate(source) → JSON result
  - list_components(category) → JSON result
  - preview_params(component) → JSON result

Tests call the tool functions directly (they are regular Python functions
before the @mcp.tool() decoration is applied).
"""

from __future__ import annotations

import json
import os
import tempfile

import pytest

try:
    from pml.mcp_server import (
        execute_pml,
        render_sprite,
        validate,
        list_components,
        preview_params,
    )
    HAS_MCP = True
except ImportError:
    HAS_MCP = False


def _parse(s: str) -> dict:
    """Parse a JSON string response from an MCP tool."""
    return json.loads(s)


pytestmark = pytest.mark.skipif(not HAS_MCP, reason="mcp package not installed")


# ======================================================================
# execute_pml
# ======================================================================


class TestExecutePML:
    """execute_pml tool — execute arbitrary PML code."""

    def test_execute_arithmetic(self) -> None:
        result = _parse(execute_pml("(+ 1 2)"))
        assert result["success"] is True
        assert result["value"] == 3
        assert result["error"] is None

    def test_execute_string_op(self) -> None:
        result = _parse(execute_pml('(string-append "a" "b")'))
        assert result["success"] is True
        assert result["value"] == "ab"

    def test_execute_define_and_use(self) -> None:
        execute_pml("(define x 42)")
        result = _parse(execute_pml("x"))
        assert result["success"] is True
        assert result["value"] == 42

    def test_execute_with_syntax_error(self) -> None:
        result = _parse(execute_pml("(+ 1 "))
        assert result["success"] is False
        assert result["error"] is not None
        assert "type" in result["error"]

    def test_execute_with_runtime_error(self) -> None:
        result = _parse(execute_pml("(undefined-function 1 2)"))
        assert result["success"] is False
        assert result["error"] is not None

    def test_execute_returns_value(self) -> None:
        result = _parse(execute_pml('"hello"'))
        assert result["success"] is True
        assert result["value"] == "hello"

    def test_execute_empty_source(self) -> None:
        result = _parse(execute_pml(""))
        assert result["success"] is True
        assert result["value"] is None


# ======================================================================
# render_sprite
# ======================================================================


class TestRenderSprite:
    """render_sprite tool — execute PML and render canvas to PNG."""

    def test_render_basic(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            result = json.loads(render_sprite(
                f"""
                (sprite-canvas 64 64 :bg "transparent")
                (add (rect 0 0 32 32 :fill "#ff0000"))
                (render "{tmpdir}/test.png")
                """,
                name="test",
            ))
            assert result["success"] is True
            assert "file" in result
            assert result["format"] == "PNG"

    def test_render_without_canvas(self) -> None:
        result = json.loads(render_sprite(
            "(+ 1 2)",
            name="fail",
        ))
        assert result["success"] is False
        assert "error" in result

    def test_render_invalid_source(self) -> None:
        result = json.loads(render_sprite(
            "(this will !!! break",
            name="broken",
        ))
        assert result["success"] is False

    def test_render_character(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            result = json.loads(render_sprite(
                f"""
                (sprite-canvas 128 256 :bg "transparent")
                (add (character :expression "happy" :style 'cel))
                (render "{tmpdir}/hero.png")
                """,
                name="hero",
            ))
            assert result["success"] is True
            assert result["width"] > 0
            assert result["height"] > 0


# ======================================================================
# validate
# ======================================================================


class TestValidate:
    """validate tool — check PML syntax without executing."""

    def test_validate_valid_code(self) -> None:
        result = _parse(validate("(+ 1 2)"))
        assert result["valid"] is True
        assert len(result["errors"]) == 0

    def test_validate_invalid_syntax(self) -> None:
        result = _parse(validate("(+ 1 "))
        assert result["valid"] is False
        assert len(result["errors"]) > 0

    def test_validate_complex_code(self) -> None:
        code = """
            (sprite-canvas 128 128 :bg "transparent")
            (add (rect 0 0 32 32 :fill "#ff0000"))
            (render "out.png")
        """
        result = _parse(validate(code))
        assert result["valid"] is True

    def test_validate_empty_string(self) -> None:
        result = _parse(validate(""))
        assert result["valid"] is True  # Empty is valid PML

    def test_validate_unicode(self) -> None:
        result = _parse(validate('(string-append "héllo" " 世界")'))
        assert result["valid"] is True


# ======================================================================
# list_components
# ======================================================================


class TestListComponents:
    """list_components tool — discover available sprite components."""

    def test_list_all(self) -> None:
        result = _parse(list_components(None))
        assert isinstance(result, list)
        assert len(result) > 0
        # Should include core components
        names = [c["component"] for c in result]
        assert "character" in names
        assert "head" in names
        assert "body" in names

    def test_list_by_character_category(self) -> None:
        result = _parse(list_components("character"))
        assert isinstance(result, list)
        names = [c["component"] for c in result]
        assert "character" in names
        assert "body" in names
        assert "head" in names

    def test_list_by_items_category(self) -> None:
        result = _parse(list_components("items"))
        assert isinstance(result, list)
        names = [c["component"] for c in result]
        assert "weapon" in names
        assert "potion" in names

    def test_list_by_ui_category(self) -> None:
        result = _parse(list_components("ui"))
        assert isinstance(result, list)
        names = [c["component"] for c in result]
        assert "button" in names

    def test_list_by_scene_category(self) -> None:
        result = _parse(list_components("scene"))
        assert isinstance(result, list)
        names = [c["component"] for c in result]
        assert "tile" in names

    def test_list_invalid_category(self) -> None:
        result = _parse(list_components("nonexistent"))
        assert isinstance(result, list)
        assert len(result) == 0

    def test_components_have_params(self) -> None:
        result = _parse(list_components(None))
        for comp in result:
            assert "params" in comp, f"Component {comp['component']} missing params"
            assert isinstance(comp["params"], list)


# ======================================================================
# preview_params
# ======================================================================


class TestPreviewParams:
    """preview_params tool — get parameter details for a component."""

    def test_preview_character(self) -> None:
        result = _parse(preview_params("character"))
        assert result["component"] == "character"
        assert "params" in result
        params = {p["name"]: p for p in result["params"]}
        assert "expression" in params
        assert "scale" in params
        assert "view" in params

    def test_preview_body(self) -> None:
        result = _parse(preview_params("body"))
        assert result["component"] == "body"
        params = {p["name"]: p for p in result["params"]}
        assert "build" in params
        assert "view" in params

    def test_preview_eyes(self) -> None:
        result = _parse(preview_params("anime-eyes"))
        assert result["component"] == "anime-eyes"
        params = {p["name"]: p for p in result["params"]}
        assert "style" in params
        assert params["style"]["allowed"] == ["shoujo", "shounen", "round", "sharp", "sleepy", "closed"]

    def test_preview_unknown_component(self) -> None:
        result = _parse(preview_params("not-a-real-component"))
        assert "error" in result
        assert "available" in result


# ======================================================================
# Environment persistence across calls
# ======================================================================


class TestMCPEnvironmentPersistence:
    """MCP tools share state across calls (same PMLRuntime instance)."""

    def test_define_persists(self) -> None:
        execute_pml("(define mcp-test-var 99)")
        result = _parse(execute_pml("mcp-test-var"))
        assert result["success"] is True
        assert result["value"] == 99

    def test_character_cache_warm(self) -> None:
        """Components registered once should be available across calls."""
        result = _parse(execute_pml("(character :expression 'neutral)"))
        assert result["success"] is True
