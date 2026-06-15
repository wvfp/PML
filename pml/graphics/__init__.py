"""PML graphics subsystem — registration entry point."""

from __future__ import annotations

from pml.environment import Environment


def register_graphics(env: Environment) -> None:
    """Register all graphics-related builtins into the environment."""
    from pml.graphics.builtins_graphics import register_primitives
    from pml.graphics.canvas import register_canvas
    from pml.graphics.render import register_render

    register_primitives(env)
    register_canvas(env)
    register_render(env)
