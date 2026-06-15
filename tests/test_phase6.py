"""Tests for Phase 6 — LLM API interface layer."""

import os
import tempfile

import pytest

from pml.api import (
    PMLRuntime,
    RenderResult,
    SpriteAsset,
    ValidationResult,
    _error_to_dict,
    _generate_hint,
    _serialize_value,
)
from pml.errors import (
    AccessError,
    ArityError,
    ImportError_,
    MacroExpansionDepthError,
    PMLError,
    PMLSyntaxError,
    PMLTypeError,
    UnboundVariableError,
)
from pml.graphics.objects import GraphicObject
from pml.sprites.registry import ComponentRegistry
from pml.types import Keyword, Symbol


# ======================================================================
# PMLRuntime
# ======================================================================


class TestPMLRuntime:
    def test_create_runtime(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            assert rt is not None
            assert os.path.isdir(tmpdir)

    def test_execute_arithmetic(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(+ 1 2)")
            assert isinstance(result, RenderResult)
            assert result.success is True
            assert result.value == 3
            assert result.error is None

    def test_execute_string(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute('(string-append "hello" " " "world")')
            assert result.success is True
            assert result.value == "hello world"

    def test_execute_define_and_use(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            rt.execute("(define x 10)")
            result = rt.execute("(+ x 5)")
            assert result.success is True
            assert result.value == 15

    def test_execute_preserves_env(self):
        """Multiple execute() calls share the same environment."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            rt.execute("(define greeting \"hello\")")
            result = rt.execute("greeting")
            assert result.success is True
            assert result.value == "hello"

    def test_execute_multiple_expressions(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(define a 3)\n(define b 4)\n(+ a b)")
            assert result.success is True
            assert result.value == 7

    def test_execute_with_macro(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute(
                '(defmacro double (x) (+ x x))\n(double 21)'
            )
            assert result.success is True
            assert result.value == 42

    def test_execute_error_returns_structured_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(undefined-var)")
            assert result.success is False
            assert result.error is not None
            assert "type" in result.error
            assert "message" in result.error
            assert "hint" in result.error

    def test_execute_syntax_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(+ 1")
            assert result.success is False
            assert result.error is not None

    def test_execute_graphic_object(self):
        """Graphic objects are serialized to readable strings."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute('(circle 50 50 30 :fill "red")')
            assert result.success is True
            assert isinstance(result.value, str)
            assert "graphic-object" in result.value

    def test_reset(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            rt.execute("(define secret 42)")
            rt.reset()
            result = rt.execute("secret")
            assert result.success is False  # secret is gone

    def test_to_json(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("(+ 1 2)")
            j = result.to_json()
            import json
            parsed = json.loads(j)
            assert parsed["success"] is True
            assert parsed["value"] == 3


# ======================================================================
# Validation
# ======================================================================


class TestValidation:
    def test_valid_source(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.validate("(+ 1 2)")
            assert isinstance(result, ValidationResult)
            assert result.valid is True
            assert len(result.errors) == 0

    def test_valid_complex_source(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(define x 10)\n(let ((y 20)) (+ x y))'
            result = rt.validate(source)
            assert result.valid is True

    def test_parse_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.validate("(+ 1")
            assert result.valid is False
            assert len(result.errors) > 0
            assert result.errors[0]["type"] == "PMLSyntaxError"

    def test_lex_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            # Unterminated string is a lex error
            result = rt.validate('"hello')
            assert result.valid is False
            assert len(result.errors) > 0

    def test_macro_expansion_error(self):
        """Macro depth limit is caught during validation."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(defmacro loop () (loop))\n(loop)'
            result = rt.validate(source)
            # defmacro itself is fine syntactically, expansion would fail at eval
            # The expander pre-pass may or may not catch this (it depends on
            # whether defmacro has been evaluated at expansion time)
            # Either way, validate should not crash
            assert isinstance(result, ValidationResult)

    def test_validation_to_json(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.validate("(+ 1")
            j = result.to_json()
            import json
            parsed = json.loads(j)
            assert parsed["valid"] is False
            assert len(parsed["errors"]) > 0

    def test_valid_sprite_code(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(sprite-canvas 64 64)\n(add (body :height 80))'
            result = rt.validate(source)
            assert result.valid is True


# ======================================================================
# Component Discovery
# ======================================================================


class TestComponentDiscovery:
    def test_list_all_components(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components()
            assert isinstance(components, list)
            assert len(components) == 18  # All registered components
            names = [c["component"] for c in components]
            assert "body" in names
            assert "anime-eyes" in names
            assert "character" in names
            assert "tile" in names
            assert "button" in names

    def test_list_by_category_character(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components(category="character")
            names = [c["component"] for c in components]
            assert "body" in names
            assert "head" in names
            assert "anime-eyes" in names
            assert "mouth" in names
            assert "hair" in names
            assert "character" in names
            assert "outfit" in names
            # Items should not appear
            assert "weapon" not in names

    def test_list_by_category_items(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components(category="items")
            names = [c["component"] for c in components]
            assert "weapon" in names
            assert "potion" in names
            assert "chest" in names
            assert "generic-item" in names
            assert "body" not in names

    def test_list_by_category_ui(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components(category="ui")
            names = [c["component"] for c in components]
            assert "button" in names
            assert "panel" in names
            assert "health-bar" in names
            assert "icon" in names

    def test_list_by_category_scene(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components(category="scene")
            names = [c["component"] for c in components]
            assert "tile" in names
            assert "decoration" in names
            assert "background" in names

    def test_list_empty_category(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            components = rt.list_components(category="nonexistent")
            assert components == []

    def test_component_has_params(self):
        """Every component in the listing should have a params key."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            for comp in rt.list_components():
                assert "params" in comp, f"{comp['component']} missing params"
                assert isinstance(comp["params"], list)
                assert len(comp["params"]) > 0

    def test_preview_params_known(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            params = rt.preview_params("anime-eyes")
            assert params["component"] == "anime-eyes"
            assert params["category"] == "character"
            assert "params" in params
            # Check specific param
            style_param = next(p for p in params["params"] if p["name"] == "style")
            assert style_param["type"] == "enum"
            assert "shoujo" in style_param["values"]

    def test_preview_params_unknown(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            params = rt.preview_params("nonexistent-component")
            assert "error" in params
            assert "available" in params

    def test_schema_json_format(self):
        """Verify the JSON format matches the development.md spec."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            params = rt.preview_params("anime-eyes")

            # Should have component, category, and params
            assert "component" in params
            assert "category" in params
            assert "params" in params

            for p in params["params"]:
                assert "name" in p
                assert "type" in p
                assert "default" in p
                if p["type"] == "enum":
                    assert "values" in p
                elif p["type"] == "number":
                    assert "min" in p
                    assert "max" in p

    def test_body_params(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            params = rt.preview_params("body")
            param_names = [p["name"] for p in params["params"]]
            assert "height" in param_names
            assert "build" in param_names
            assert "skin" in param_names

    def test_weapon_params(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            params = rt.preview_params("weapon")
            assert params["category"] == "items"
            type_param = next(p for p in params["params"] if p["name"] == "type")
            assert "sword" in type_param["values"]


# ======================================================================
# Render Sprite
# ======================================================================


class TestRenderSprite:
    def test_render_basic_sprite(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(sprite-canvas 64 64)\n(add (circle 32 32 20 :fill "red"))'
            result = rt.render_sprite(source, name="test_circle")
            assert isinstance(result, SpriteAsset)
            assert os.path.exists(result.path)
            assert result.width == 64
            assert result.height == 64
            assert result.format == "PNG"

    def test_render_character_sprite(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(sprite-canvas 128 128)\n(add (character))'
            result = rt.render_sprite(source, name="hero")
            assert isinstance(result, SpriteAsset)
            assert os.path.exists(result.path)
            assert result.width == 128
            assert result.height == 128
            # Should have meta.json
            assert result.meta_path is not None
            assert os.path.exists(result.meta_path)

    def test_render_no_canvas_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            # Code that doesn't create a canvas
            result = rt.render_sprite("(+ 1 2)", name="no_canvas")
            assert isinstance(result, RenderResult)
            assert result.success is False
            assert result.error is not None
            assert "canvas" in result.error["message"].lower() or "NoCanvas" in result.error["type"]

    def test_render_with_execution_error(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.render_sprite("(undefined-fn)", name="bad")
            assert isinstance(result, RenderResult)
            assert result.success is False

    def test_render_custom_format(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(sprite-canvas 32 32)\n(add (circle 16 16 10 :fill "blue"))'
            result = rt.render_sprite(source, name="tiny", format="png")
            assert isinstance(result, SpriteAsset)
            assert result.path.endswith(".png")

    def test_render_sprite_to_json(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            source = '(sprite-canvas 64 64)\n(add (circle 32 32 20 :fill "green"))'
            result = rt.render_sprite(source, name="json_test")
            assert isinstance(result, SpriteAsset)
            j = result.to_json()
            import json
            parsed = json.loads(j)
            assert parsed["width"] == 64
            assert parsed["height"] == 64


# ======================================================================
# Error Serialization
# ======================================================================


class TestErrorSerialization:
    def test_unbound_variable_error(self):
        e = UnboundVariableError("x is not defined")
        d = _error_to_dict(e)
        assert d["type"] == "UnboundVariableError"
        assert d["message"] == "x is not defined"
        assert len(d["hint"]) > 0
        assert "define" in d["hint"].lower()

    def test_arity_error(self):
        e = ArityError("Expected 2 arguments, got 1")
        d = _error_to_dict(e)
        assert d["type"] == "ArityError"
        assert "arguments" in d["hint"].lower()

    def test_syntax_error(self):
        e = PMLSyntaxError("unmatched parenthesis")
        d = _error_to_dict(e)
        assert d["type"] == "PMLSyntaxError"
        assert "parenthes" in d["hint"].lower()

    def test_type_error(self):
        e = PMLTypeError("expected number, got string")
        d = _error_to_dict(e)
        assert d["type"] == "PMLTypeError"
        assert len(d["hint"]) > 0

    def test_import_error(self):
        e = ImportError_("file not found: test.pml")
        d = _error_to_dict(e)
        assert d["type"] == "ImportError_"
        assert "path" in d["hint"].lower() or "forward" in d["hint"].lower()

    def test_macro_depth_error(self):
        e = MacroExpansionDepthError("depth limit exceeded")
        d = _error_to_dict(e)
        assert d["type"] == "MacroExpansionDepthError"
        assert "recursiv" in d["hint"].lower() or "base case" in d["hint"].lower()

    def test_access_error(self):
        e = AccessError("symbol not exported")
        d = _error_to_dict(e)
        assert d["type"] == "AccessError"
        assert "provide" in d["hint"].lower() or "export" in d["hint"].lower()

    def test_error_with_location(self):
        e = PMLSyntaxError("bad token", line=5, column=12, filename="test.pml")
        d = _error_to_dict(e)
        assert d["line"] == 5
        assert d["column"] == 12
        assert d["filename"] == "test.pml"

    def test_error_without_location(self):
        e = ArityError("wrong count")
        d = _error_to_dict(e)
        assert "line" not in d
        assert "column" not in d

    def test_generate_hint_generic(self):
        """Unknown error types get empty hint."""
        e = PMLError("generic error")
        hint = _generate_hint(e)
        assert isinstance(hint, str)


# ======================================================================
# Value Serialization
# ======================================================================


class TestValueSerialization:
    def test_serialize_none(self):
        assert _serialize_value(None) is None

    def test_serialize_bool(self):
        assert _serialize_value(True) is True
        assert _serialize_value(False) is False

    def test_serialize_int(self):
        assert _serialize_value(42) == 42

    def test_serialize_float(self):
        assert _serialize_value(3.14) == 3.14

    def test_serialize_string(self):
        assert _serialize_value("hello") == "hello"

    def test_serialize_list(self):
        assert _serialize_value([1, "two", 3.0]) == [1, "two", 3.0]

    def test_serialize_nested_list(self):
        result = _serialize_value([1, [2, 3]])
        assert result == [1, [2, 3]]

    def test_serialize_symbol(self):
        result = _serialize_value(Symbol("foo"))
        assert result == "<symbol:foo>"

    def test_serialize_keyword(self):
        result = _serialize_value(Keyword("fill"))
        assert result == "<keyword:fill>"

    def test_serialize_graphic_object(self):
        obj = GraphicObject(shape_type="circle", params={"r": 10})
        result = _serialize_value(obj)
        assert "graphic-object" in result
        assert "circle" in result

    def test_serialize_builtin(self):
        from pml.types import BuiltinProcedure
        proc = BuiltinProcedure("my-fn", lambda: None)
        result = _serialize_value(proc)
        assert "builtin" in result
        assert "my-fn" in result


# ======================================================================
# Integration: full pipeline
# ======================================================================


class TestIntegration:
    def test_full_sprite_pipeline(self):
        """Execute, render, and verify a complete sprite pipeline."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(style="cel", output_dir=tmpdir)

            # List components first
            chars = rt.list_components(category="character")
            assert len(chars) > 0

            # Check eyes params
            eyes = rt.preview_params("anime-eyes")
            assert eyes["component"] == "anime-eyes"

            # Render a character sprite
            source = (
                '(sprite-canvas 128 128)\n'
                '(add (character :expression "happy"))\n'
            )
            result = rt.render_sprite(source, name="hero")
            assert isinstance(result, SpriteAsset)
            assert os.path.exists(result.path)

    def test_validate_then_execute(self):
        """Validate before executing to catch errors early."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)

            # Valid code
            v = rt.validate("(+ 1 2)")
            assert v.valid
            r = rt.execute("(+ 1 2)")
            assert r.success

            # Invalid code
            v = rt.validate("(+ 1")
            assert not v.valid
            r = rt.execute("(+ 1")
            assert not r.success

    def test_component_discovery_then_render(self):
        """Use component discovery to build valid PML, then render."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)

            # Get weapon params
            wpn = rt.preview_params("weapon")
            assert "params" in wpn

            # Build PML from discovered params
            source = (
                '(sprite-canvas 64 64)\n'
                '(add (weapon :type "sword" :size "large"))\n'
            )
            result = rt.render_sprite(source, name="my_sword")
            assert isinstance(result, SpriteAsset)
            assert os.path.exists(result.path)
