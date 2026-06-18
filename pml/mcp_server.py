"""PML MCP Server — Model Context Protocol interface for PML.

Provides AI agents with tools to execute PML code, render sprites,
discover components, and validate source code.

Run: uv run python -m pml.mcp_server
"""

from __future__ import annotations

import json
import os

from mcp.server.fastmcp import FastMCP

from pml.api import PMLRuntime, SpriteAsset
from pml.sprites.palette import _PREDEFINED_PALETTES
from pml.sprites.style import _PREDEFINED_STYLES

# Create MCP server
mcp = FastMCP("pml-mcp")

# Global runtime instance (reused across calls)
_runtime: PMLRuntime | None = None


def _get_runtime() -> PMLRuntime:
    global _runtime
    if _runtime is None:
        _runtime = PMLRuntime()
    return _runtime


@mcp.tool()
def execute_pml(source: str) -> str:
    """Execute PML source code and return the result.

    Use this to run any PML code: define variables, call functions,
    create graphics, set up animation timelines, etc.
    Environment persists across calls — previous definitions are visible.
    """
    result = _get_runtime().execute(source)
    return json.dumps({
        "success": result.success,
        "value": result.value,
        "error": result.error,
    }, indent=2, default=str)


@mcp.tool()
def render_sprite(source: str, name: str = "sprite") -> str:
    """Execute PML source and render the canvas to a PNG sprite file.

    The source should set up a canvas with (sprite-canvas ...),
    add graphics with (add ...), then this tool handles rendering.
    Returns the output file path and metadata.
    """
    result = _get_runtime().render_sprite(source, name=name)
    if isinstance(result, SpriteAsset):
        return json.dumps({
            "success": True,
            "file": result.path,
            "width": result.width,
            "height": result.height,
            "format": result.format,
            "meta": result.meta_path,
        }, indent=2, default=str)
    return json.dumps({
        "success": False,
        "error": result.error,
    }, indent=2, default=str)


@mcp.tool()
def render_animation(source: str, name: str = "animation", fps: int = 24) -> str:
    """Execute PML source with animation and render as animated GIF.

    The source should set up a canvas, add graphics, register animation tracks,
    and call (play). This tool handles the render step automatically.
    Returns the output file path and frame count.

    Example source::

        (sprite-canvas 128 256 :bg "transparent")
        (define c (character :expression "happy"))
        (add c)
        (blink c)
        (play)
    """
    runtime = _get_runtime()
    # Reset canvas and timeline, then execute
    from pml.graphics.canvas import _current_canvas, get_current_canvas
    from pml.graphics.render import _render
    from pml.animation.timeline import Timeline

    _current_canvas[0] = None
    Timeline.reset()

    exec_result = runtime.execute(source)
    if not exec_result.success:
        return json.dumps({
            "success": False,
            "error": exec_result.error,
        }, indent=2, default=str)

    canvas = get_current_canvas()
    if canvas is None:
        return json.dumps({
            "success": False,
            "error": {
                "type": "NoCanvas",
                "message": "No canvas was created. Use (sprite-canvas ...) first.",
            },
        }, indent=2, default=str)

    filename = os.path.join(runtime._output_dir, f"{name}.gif")
    _render(filename)

    timeline = Timeline.get_current()
    num_frames = max(1, int(timeline.get_total_duration() * fps)) if timeline.animations else 1

    return json.dumps({
        "success": True,
        "file": filename,
        "width": canvas.width,
        "height": canvas.height,
        "format": "GIF",
        "frames": num_frames,
    }, indent=2, default=str)


@mcp.tool()
def validate(source: str) -> str:
    """Validate PML source code without executing it.

    Checks lexing, parsing, and macro expansion for errors.
    Safe to use on untrusted or incomplete code.
    """
    result = _get_runtime().validate(source)
    return json.dumps(result.to_dict(), indent=2, default=str)


@mcp.tool()
def list_components(category: str | None = None) -> str:
    """List available sprite components with parameter specs.

    Optionally filter by category: 'character', 'items', 'ui', or 'scene'.
    Returns component names, categories, and their parameters.
    """
    components = _get_runtime().list_components(category=category)
    return json.dumps(components, indent=2, default=str)


@mcp.tool()
def preview_params(component: str) -> str:
    """Get the full parameter specification for a specific component.

    Includes types, default values, and allowed values for every parameter.
    Use this before generating PML code that uses a component.
    """
    params = _get_runtime().preview_params(component)
    return json.dumps(params, indent=2, default=str)


@mcp.tool()
def list_palettes() -> str:
    """List all available color palettes with their color keys.

    Palettes provide preset color schemes for characters.
    Use (define-palette ...) to add custom palettes.
    """
    result = []
    for name, palette in sorted(_PREDEFINED_PALETTES.items()):
        result.append({
            "name": name,
            "colors": dict(palette.colors),
        })
    return json.dumps(result, indent=2, default=str)


@mcp.tool()
def list_styles() -> str:
    """List all available visual styles with their parameters.

    Styles control outline width, shading mode, and colorization.
    Apply with (use-style '<name>) or :style keyword on character.
    """
    result = []
    for name, style in sorted(_PREDEFINED_STYLES.items()):
        result.append({
            "name": name,
            "outline_width": style.outline_width,
            "outline_style": style.outline_style,
            "shading": style.shading,
            "outline_color": style.outline_color,
        })
    return json.dumps(result, indent=2, default=str)


@mcp.tool()
def preview_component(component: str, params_json: str = "{}") -> str:
    """Render a visual preview of a single sprite component.

    Creates a 128x128 canvas, renders the component with given params,
    and returns the file path. Use this to see what a component looks like
    before incorporating it into a larger scene.

    Args:
        component: Component name (e.g. 'head', 'body', 'weapon').
        params_json: JSON string of keyword parameters for the component.
    """
    import tempfile
    from pathlib import Path

    with tempfile.TemporaryDirectory() as tmpdir:
        try:
            params = json.loads(params_json) if params_json.strip() else {}
        except json.JSONDecodeError as e:
            return json.dumps({
                "success": False,
                "error": {"type": "InvalidJSON", "message": str(e)},
            }, indent=2, default=str)

        # Build PML source to render the component
        param_parts = [f':{k} "{v}"' if isinstance(v, str) else f':{k} {v}' for k, v in params.items()]
        param_str = " ".join(param_parts)
        path = Path(tmpdir, f"{component}_{abs(hash(params_json))}.png").as_posix()
        source = f"""
            (sprite-canvas 128 128 :bg "transparent")
            (add ({component} {param_str}))
            (render "{path}")
        """

        result = _get_runtime().execute(source)
        if not result.success:
            return json.dumps({
                "success": False,
                "error": result.error,
            }, indent=2, default=str)

        if os.path.exists(path):
            return json.dumps({
                "success": True,
                "file": path,
                "component": component,
                "width": 128,
                "height": 128,
            }, indent=2, default=str)
        return json.dumps({
            "success": False,
            "error": {"type": "RenderError", "message": "Component rendered but no file produced"},
        }, indent=2, default=str)


@mcp.tool()
def list_builtins(kind: str | None = None) -> str:
    """List available builtin functions and special forms registered in the environment.

    Optionally filter by kind: 'procedure', 'macro', 'shader', 'style', 'palette'.
    Returns names and descriptions of all registered builtins.
    """
    from pml.types import BuiltinProcedure, Macro
    from pml.shaders import get_all_shaders
    from pml.environment import Environment

    runtime = _get_runtime()
    env = runtime.get_env()

    collected: dict[str, list[dict[str, str]]] = {}
    seen: set[str] = set()

    def _walk(e: Environment) -> None:
        for name, val in e.bindings.items():
            if name in seen:
                continue
            seen.add(name)
            entry = {"name": name}
            if isinstance(val, BuiltinProcedure):
                entry["kind"] = "procedure"
                collected.setdefault("procedure", []).append(entry)
            elif isinstance(val, Macro):
                entry["kind"] = "macro"
                collected.setdefault("macro", []).append(entry)

        if e.parent:
            _walk(e.parent)

    _walk(env)

    shaders = get_all_shaders()
    for s in shaders:
        if s.name not in seen:
            seen.add(s.name)
            collected.setdefault("shader", []).append({
                "name": s.name,
                "kind": "shader",
                "type": s.shader_type,
                "description": s.description,
            })

    shader_names = {s.name for s in shaders}

    categories = {
        "arithmetic": ["+", "-", "*", "/", "//", "%", "**"],
        "comparison": ["=", "<", ">", "<=", ">=", "eq?"],
        "string": ["string-append", "string-length", "string-split", "string-join"],
        "list": ["car", "cdr", "cons", "list", "append", "map", "filter", "reduce"],
        "type": ["symbol?", "number?", "string?", "list?"],
        "io": ["print", "read", "load", "slurp"],
        "special": ["define", "set!", "if", "cond", "lambda", "let", "let*", "begin",
                     "define-macro", "quote", "quasiquote", "unquote", "unquote-splicing",
                     "and", "or", "do", "defmacro", "provide", "import"],
        "graphics": ["canvas", "sprite-canvas", "rect", "circle", "ellipse", "line",
                      "polygon", "path", "add", "render", "render-spritesheet", "group",
                      "transform", "translate", "rotate", "scale", "anchor",
                      "pencil", "charcoal", "watercolor-rect", "watercolor-circle",
                      "hatch", "sketchify"],
        "shader_list": sorted(shader_names),
    }

    proc_names = {e["name"] for e in collected.get("procedure", [])}
    macro_names = {e["name"] for e in collected.get("macro", [])}

    result = []
    for cat, names in categories.items():
        for name in names:
            if name in shader_names:
                continue
            result.append({"name": name, "category": cat})
            seen.discard(name)

    for name in sorted(proc_names | macro_names):
        if name not in {r["name"] for r in result}:
            cat = "other"
            if "skel" in name or "ik" in name or "joint" in name or "bone" in name:
                cat = "skeleton"
            elif "animate" in name or "play" in name or "timeline" in name or "ease" in name:
                cat = "animation"
            elif "color" in name or "palette" in name:
                cat = "color"
            elif name in shader_names:
                cat = "shader"
            result.append({"name": name, "category": cat})

    if kind:
        result = [r for r in result if r.get("category") == kind]

    return json.dumps(result, indent=2, default=str)


def main() -> None:
    """Run the MCP server (stdio transport)."""
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
