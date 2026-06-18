"""PML graphics subsystem — registration entry point."""

from __future__ import annotations

from pml.environment import Environment


def register_graphics(env: Environment) -> None:
    """Register all graphics-related builtins into the environment."""
    from pml.graphics.builtins_graphics import register_primitives
    from pml.graphics.canvas import register_canvas
    from pml.graphics.layout import register_layout
    from pml.graphics.render import register_render
    from pml.graphics.scene import register_scene

    register_primitives(env)
    register_canvas(env)
    register_layout(env)
    register_scene(env)
    register_render(env)
