"""PML API — LLM-facing runtime interface for sprite generation.

Provides PMLRuntime, a high-level API class that wraps the PML interpreter
pipeline (lex → parse → expand → evaluate → render) and exposes component
discovery, validation, and structured error feedback for LLM agents.
"""

from __future__ import annotations

import json
import os
from dataclasses import dataclass, field
from typing import Any

from pml.environment import Environment
from pml.errors import (
    AccessError,
    ArityError,
    CircularImportError,
    IKNoSolutionError,
    ImportError_,
    MacroExpansionDepthError,
    PMLError,
    PMLSyntaxError,
    PMLTypeError,
    ResourceError,
    UnboundVariableError,
)
from pml.expander import Expander
from pml.evaluator import evaluate
from pml.graphics.objects import GraphicObject
from pml.lexer import Lexer
from pml.parser import Parser
from pml.repl import create_global_env
from pml.sprites.registry import ComponentRegistry
from pml.types import BuiltinProcedure, Keyword, Symbol


# ======================================================================
# Result types
# ======================================================================


@dataclass
class ValidationResult:
    """Result of syntax/structure validation without execution."""

    valid: bool
    errors: list[dict[str, Any]] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "valid": self.valid,
            "errors": self.errors,
            "warnings": self.warnings,
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


@dataclass
class RenderResult:
    """Result of executing PML source code."""

    success: bool
    value: Any = None
    files: list[str] = field(default_factory=list)
    error: dict[str, Any] | None = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "success": self.success,
            "value": self.value,
            "files": self.files,
            "error": self.error,
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


@dataclass
class SpriteAsset:
    """A rendered sprite asset with file paths and metadata."""

    path: str
    meta_path: str | None = None
    width: int = 0
    height: int = 0
    format: str = "PNG"

    def to_dict(self) -> dict[str, Any]:
        return {
            "path": self.path,
            "meta_path": self.meta_path,
            "width": self.width,
            "height": self.height,
            "format": self.format,
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


# ======================================================================
# Error serialization
# ======================================================================


def _error_to_dict(e: PMLError) -> dict[str, Any]:
    """Convert a PMLError to a structured dict with repair hints."""
    d: dict[str, Any] = {
        "type": type(e).__name__,
        "message": e.pml_message,
    }
    if e.line is not None:
        d["line"] = e.line
    if e.column is not None:
        d["column"] = e.column
    if e.filename is not None:
        d["filename"] = e.filename
    d["hint"] = _generate_hint(e)
    return d


def _generate_hint(e: PMLError) -> str:
    """Generate LLM-friendly repair hints based on error type."""

    # UnboundVariableError — variable not found in any scope
    if isinstance(e, UnboundVariableError):
        msg = e.pml_message.lower()
        # Check for common spelling patterns
        if "sprite-canvas" in msg or "sprite_canvas" in msg:
            return (
                "To create a sprite canvas, use (sprite-canvas width height :bg \"color\"). "
                "The function name is 'sprite-canvas' (with a hyphen), not 'sprite_canvas'."
            )
        if "render-sprite" in msg or "render_sprite" in msg:
            return (
                "To render the current canvas, use (render \"filename.png\") or "
                "(render-spritesheet ...). The function name is 'render' not 'render-sprite'."
            )
        return (
            "The variable is not defined in any scope. "
            "Did you misspell it? Check that you used (define name value) first. "
            "Built-in functions use hyphens between words (e.g. 'sprite-canvas', 'string-append'). "
            "If importing a module, use (import \"path.pml\" as prefix) then prefix/name."
        )

    # ArityError — wrong argument count
    if isinstance(e, ArityError):
        msg = e.pml_message
        # Try to extract expected vs actual from the message
        if "expected" in msg and "got" in msg:
            return (
                "Fix the number of arguments. "
                "Review the function's signature — optional keyword arguments like "
                ":bg, :fill, :stroke are passed as :key value pairs. "
                "If you meant to pass keyword args, make sure they come after positional args."
            )
        return (
            "Wrong number of arguments passed to this function. "
            "Check the function signature — keyword arguments use :key value syntax "
            "and come after all positional arguments."
        )

    # PMLSyntaxError — lexer/parser errors
    if isinstance(e, PMLSyntaxError):
        msg = e.pml_message.lower()
        if "unmatched" in msg or "parenthes" in msg:
            return (
                "Unbalanced parentheses. "
                "Every '(' needs a matching ')'. Count open/close parens — "
                "they should be equal. For S-expressions, each form is (function arg arg ...)."
            )
        if "unterminated string" in msg or "unterminated" in msg:
            return (
                "A string value is missing its closing quote (\"). "
                "All strings must be enclosed in double quotes: \"like this\". "
                "Check for missing \" at the end of a string."
            )
        if "token" in msg or "illegal" in msg:
            return (
                "Invalid syntax detected. "
                "Check for special characters outside strings, "
                "unquoted symbols with spaces, or malformed numbers."
            )
        return (
            "Syntax error in the PML source. Check for balanced parentheses, "
            "proper string quoting, and correct S-expression structure."
        )

    # PMLTypeError — argument type or arity mismatch
    if isinstance(e, PMLTypeError):
        msg = e.pml_message.lower()
        if "keyword" in msg and "no following value" in msg:
            return (
                "A keyword (starting with :) is missing its value. "
                "Keywords must be followed by a value: (:key value). "
                "For example: (circle 0 0 30 :fill \"#ff0000\")."
            )
        if "cannot evaluate" in msg:
            return (
                "The expression could not be evaluated. "
                "Make sure you're calling a defined function or special form. "
                "Function calls start with a symbol: (function-name arg1 arg2)."
            )
        if "expected" in msg:
            return (
                "A type mismatch occurred. Check that you passed the right type — "
                "strings need quotes (\"text\"), symbols use \'prefix (like \'center), "
                "and numbers are written directly (42, 3.14)."
            )
        return (
            "Argument type mismatch. Check that each argument has the correct type: "
            "strings need \"quotes\", symbols use \'prefix, numbers are bare. "
            "Keywords (:key) must be followed by their value."
        )

    # CircularImportError
    if isinstance(e, CircularImportError):
        return (
            "Circular dependency detected. Break the cycle by removing one of the "
            "mutual imports, or restructure into separate modules."
        )

    # ImportError_
    if isinstance(e, ImportError_):
        return (
            "Module not found or import failed. "
            "Check the file path — use forward slashes (/), not backslashes. "
            "Paths are resolved relative to the importing file's directory. "
            "Ensure the file has a .pml extension."
        )

    # MacroExpansionDepthError
    if isinstance(e, MacroExpansionDepthError):
        return (
            "A macro is expanding recursively without a base case. "
            "Add a termination condition (like an 'if' check) or restructure "
            "to avoid infinite expansion. Macros should expand to code that "
            "does not further trigger the same macro call."
        )

    # AccessError
    if isinstance(e, AccessError):
        return (
            "This symbol is not exported from the module. "
            "Open the module file and add the symbol name to (provide symbol-name ...)."
        )

    # ResourceError
    if isinstance(e, ResourceError):
        return (
            "A resource file (image, font, data) was not found. "
            "Check that the file path is correct and the file exists. "
            "Use forward slashes (/) in paths."
        )

    # IKNoSolutionError
    if isinstance(e, IKNoSolutionError):
        return (
            "The IK solver could not reach the target position. "
            "Try increasing :iterations (e.g., :iterations 50), "
            "relaxing joint angle constraints (:min/:max), "
            "or moving the target position closer to the chain."
        )

    # Generic PMLError fallback
    msg = e.pml_message.lower()
    if "not defined" in msg or "unbound" in msg:
        return (
            "A variable or function name was not found. "
            "Check spelling — PML uses hyphens in multi-word names "
            "(e.g. 'string-append', not 'string_append' or 'stringAppend'). "
            "Use (define name value) to define your own variables."
        )

    return (
        "Check the PML code for errors. "
        "Common issues: missing ) to close a form, "
        "using :keyword without a following value, "
        "or calling a function with the wrong argument types."
    )


# ======================================================================
# Value serialization
# ======================================================================


def _serialize_value(val: Any) -> Any:
    """Convert PML runtime values to JSON-serializable Python types."""
    if val is None:
        return None
    if isinstance(val, bool):
        return val
    if isinstance(val, (int, float)):
        return val
    if isinstance(val, str):
        return val
    if isinstance(val, list):
        return [_serialize_value(v) for v in val]
    if isinstance(val, dict):
        return {str(k): _serialize_value(v) for k, v in val.items()}
    if isinstance(val, Symbol):
        return f"<symbol:{val.name}>"
    if isinstance(val, Keyword):
        return f"<keyword:{val.name}>"
    if isinstance(val, GraphicObject):
        return f"<graphic-object:{val.shape_type}>"
    if isinstance(val, BuiltinProcedure):
        return f"<builtin:{val.name}>"
    # Procedure, Macro, Module, StyleDescriptor, Canvas, etc.
    return repr(val)


# ======================================================================
# PMLRuntime
# ======================================================================


class PMLRuntime:
    """High-level API for LLM agents to interact with PML.

    Wraps the full interpreter pipeline (lex → parse → expand → evaluate)
    and provides component discovery, validation, and sprite rendering.

    Usage::

        rt = PMLRuntime(output_dir="./sprites")
        result = rt.execute('(+ 1 2)')
        assert result.success and result.value == 3

        info = rt.list_components(category="character")
        params = rt.preview_params("anime-eyes")

        sprite = rt.render_sprite(
            '(sprite-canvas 128 128)\\n(add (character))',
            name="hero",
        )
    """

    def __init__(
        self,
        style: str = "cel",
        output_dir: str = "./output",
    ) -> None:
        """Initialize the PML runtime.

        Args:
            style: Default visual style (cel, pixel, flat).
            output_dir: Directory for rendered sprite output files.
        """
        self._style = style
        self._output_dir = os.path.abspath(output_dir)
        os.makedirs(self._output_dir, exist_ok=True)
        self._env = create_global_env()

        # Apply default style
        try:
            self.execute(f'(use-style "{style}")')
        except Exception:
            pass  # Style might not exist; that's fine

    def reset(self) -> None:
        """Reset to a fresh environment, discarding all definitions."""
        self._env = create_global_env()
        # Reset animation timeline
        from pml.animation.timeline import Timeline
        Timeline.reset()
        # Clear skeleton skin bindings
        from pml.skeleton import clear_skin_bindings
        clear_skin_bindings()
        try:
            self.execute(f'(use-style "{self._style}")')
        except Exception:
            pass

    # ------------------------------------------------------------------
    # Core execution
    # ------------------------------------------------------------------

    def execute(self, source: str) -> RenderResult:
        """Execute PML source code and return the result.

        The environment persists across calls — definitions from a previous
        execute() are visible in the next.
        """
        try:
            tokens = Lexer(source).tokenize()
            ast = Parser(tokens).parse()
            ast = Expander(self._env).expand_all(ast)
            result: Any = None
            for expr in ast:
                result = evaluate(expr, self._env)
            return RenderResult(
                success=True,
                value=_serialize_value(result),
                files=[],
                error=None,
            )
        except PMLError as e:
            return RenderResult(
                success=False,
                value=None,
                files=[],
                error=_error_to_dict(e),
            )
        except Exception as e:
            return RenderResult(
                success=False,
                value=None,
                files=[],
                error={"type": "InternalError", "message": str(e), "hint": ""},
            )

    # ------------------------------------------------------------------
    # Sprite rendering
    # ------------------------------------------------------------------

    def render_sprite(
        self,
        source: str,
        **options: Any,
    ) -> SpriteAsset | RenderResult:
        """Execute PML source and render the canvas to a sprite asset.

        Args:
            source: PML source code that sets up a canvas and adds graphics.
            **options:
                name (str): Output filename without extension (default "sprite").
                format (str): Image format, "png" or "jpg" (default "png").

        Returns:
            SpriteAsset on success, RenderResult on failure.
        """
        from pml.graphics.canvas import _current_canvas, get_current_canvas
        from pml.graphics.render import _render
        from pml.animation.timeline import Timeline

        name = str(options.get("name", "sprite"))
        fmt = str(options.get("format", "png"))
        filename = os.path.join(self._output_dir, f"{name}.{fmt}")

        # Reset canvas and timeline before execution
        _current_canvas[0] = None
        Timeline.reset()

        exec_result = self.execute(source)
        if not exec_result.success:
            return exec_result

        # Render the canvas to file
        canvas = get_current_canvas()
        if canvas is None:
            return RenderResult(
                success=False,
                value=None,
                files=[],
                error={
                    "type": "NoCanvas",
                    "message": "No canvas was created. Use (canvas ...) or (sprite-canvas ...) before rendering.",
                    "hint": "Add (sprite-canvas width height) at the start of your PML code.",
                },
            )

        _render(filename)

        meta_path = os.path.splitext(filename)[0] + ".meta.json"
        return SpriteAsset(
            path=filename,
            meta_path=meta_path if os.path.exists(meta_path) else None,
            width=canvas.width,
            height=canvas.height,
            format=fmt.upper(),
        )

    # ------------------------------------------------------------------
    # Validation
    # ------------------------------------------------------------------

    def validate(self, source: str) -> ValidationResult:
        """Validate PML source code without executing it.

        Checks lexing, parsing, and macro expansion. Does not evaluate
        expressions or render graphics.
        """
        errors: list[dict[str, Any]] = []
        warnings: list[str] = []

        # Lex
        try:
            tokens = Lexer(source).tokenize()
        except PMLError as e:
            errors.append(_error_to_dict(e))
            return ValidationResult(valid=False, errors=errors, warnings=warnings)

        # Parse
        try:
            ast = Parser(tokens).parse()
        except PMLError as e:
            errors.append(_error_to_dict(e))
            return ValidationResult(valid=False, errors=errors, warnings=warnings)

        # Macro expansion (catches expansion errors without evaluating)
        try:
            Expander(self._env).expand_all(ast)
        except PMLError as e:
            errors.append(_error_to_dict(e))

        return ValidationResult(
            valid=len(errors) == 0,
            errors=errors,
            warnings=warnings,
        )

    # ------------------------------------------------------------------
    # Component discovery
    # ------------------------------------------------------------------

    def list_components(self, category: str | None = None) -> list[dict[str, Any]]:
        """List available semantic components with their parameter specs.

        Args:
            category: Optional filter — "character", "items", "ui", or "scene".

        Returns:
            List of component info dicts with component, category, and params keys.
        """
        registry = ComponentRegistry.get_instance()
        names = registry.list_components()
        result: list[dict[str, Any]] = []

        for name in names:
            cat = registry.get_category(name)
            if category is not None and cat != category:
                continue

            info: dict[str, Any] = {
                "component": name,
                "category": cat or "unknown",
            }
            schema = registry.get_schema(name)
            if schema is not None:
                info["params"] = schema.to_dict()["params"]
            result.append(info)

        return result

    def preview_params(self, component: str) -> dict[str, Any]:
        """Return the parameter specification for a specific component.

        Returns a dict with component name, category, and params list
        describing each parameter's type, default, and allowed values.
        """
        registry = ComponentRegistry.get_instance()
        schema = registry.get_schema(component)

        if schema is None:
            return {
                "error": f"Unknown component: {component}",
                "available": registry.list_components(),
            }

        cat = registry.get_category(component)
        return {
            "component": component,
            "category": cat or "unknown",
            "params": schema.to_dict()["params"],
        }

    # ------------------------------------------------------------------
    # Utility
    # ------------------------------------------------------------------

    def get_env(self) -> Environment:
        """Return the internal environment (for advanced use/testing)."""
        return self._env

    def set_output_dir(self, path: str) -> None:
        """Change the output directory for sprite rendering."""
        self._output_dir = os.path.abspath(path)
        os.makedirs(self._output_dir, exist_ok=True)
