"""PML MCP Server — Model Context Protocol interface for PML.

Provides AI agents with tools to execute PML code, render sprites,
discover components, and validate source code.

Run: uv run python -m pml.mcp_server
"""

from __future__ import annotations

import json

from mcp.server.fastmcp import FastMCP

from pml.api import PMLRuntime, SpriteAsset

# Create MCP server
mcp = FastMCP("pml-mcp")

# Global runtime instance (reused across requests)
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


def main() -> None:
    """Run the MCP server (stdio transport)."""
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
