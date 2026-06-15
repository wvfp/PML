"""PML sprite subsystem — registration entry point."""

from __future__ import annotations

from pml.environment import Environment


def register_sprites(env: Environment) -> None:
    """Register all sprite-related builtins."""
    from pml.sprites.style import register_style
    from pml.sprites.palette import register_palette
    from pml.sprites.registry import register_components

    register_style(env)
    register_palette(env)
    register_components(env)
