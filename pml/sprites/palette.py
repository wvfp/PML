"""PML palette system — define-palette, palette-ref."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.environment import Environment
from pml.errors import PMLError
from pml.types import BuiltinProcedure


@dataclass
class Palette:
    """A named collection of colors."""

    name: str
    colors: dict[str, str] = field(default_factory=dict)

    def get(self, key: str) -> str:
        if key not in self.colors:
            print(f"Warning: palette '{self.name}' has no color '{key}', using #808080")
            return "#808080"
        return self.colors[key]


# Predefined palettes
_PREDEFINED_PALETTES: dict[str, Palette] = {
    "dark-hero": Palette(
        name="dark-hero",
        colors={
            "hair": "#2c2c2c",
            "skin": "#fce4c8",
            "skin-shadow": "#d4a574",
            "eyes": "#4a90d9",
            "outfit-primary": "#1a1a2e",
            "outfit-secondary": "#16213e",
            "outline": "#0a0a0a",
            "highlight": "#ffffff",
        },
    ),
    "warm-skin": Palette(
        name="warm-skin",
        colors={
            "skin": "#f5d0a9",
            "skin-shadow": "#d4a574",
            "skin-highlight": "#ffe8cc",
        },
    ),
}

# Global palette registry
_palette_registry: dict[str, Palette] = dict(_PREDEFINED_PALETTES)

# Currently active palette (set by character/item assembly)
_active_palette: list[Palette | None] = [None]


def set_active_palette(palette: Palette | None) -> None:
    _active_palette[0] = palette


def get_active_palette() -> Palette | None:
    return _active_palette[0]


def get_palette(name: str) -> Palette:
    if name in _palette_registry:
        return _palette_registry[name]
    print(f"Warning: unknown palette '{name}', using empty palette")
    return Palette(name=name)


def _define_palette(name: Any, color_list: list) -> None:
    """(define-palette "name" (list (list "key" "#color") ...))"""
    palette_name = str(name).strip('"')
    colors = {}
    for pair in color_list:
        if isinstance(pair, list) and len(pair) >= 2:
            colors[str(pair[0])] = str(pair[1])
    _palette_registry[palette_name] = Palette(name=palette_name, colors=colors)


def _palette_keyword(name: str, **kwargs: str) -> None:
    """Define a palette with keyword syntax.

    Usage: (palette "ocean" :primary "#2c3e50" :accent "#3498db")
    """
    palette_name = str(name).strip('"')
    colors: dict[str, str] = {}
    for k, v in kwargs.items():
        colors[k] = str(v)
    _palette_registry[palette_name] = Palette(name=palette_name, colors=colors)


def _use_palette(name: str) -> None:
    """Activate a palette by name for $key color resolution.

    Usage: (use-palette "ocean")
    """
    p = get_palette(name)
    set_active_palette(p)


def resolve_color(color: str) -> str:
    """Resolve a color string, handling ``$key`` shorthand.

    If *color* starts with ``$``, looks up the key in the active palette.
    Otherwise returns *color* unchanged.
    """
    if isinstance(color, str) and color.startswith("$"):
        key = color[1:]  # strip $
        palette = _active_palette[0]
        if palette is None:
            print(f"Warning: no active palette for key '{key}', using #808080")
            return "#808080"
        return palette.get(key)
    return str(color) if color else "#808080"


def _palette_ref(key: str) -> str:
    """(palette-ref "key") → look up in active palette."""
    return resolve_color(f"${key}")


def register_palette(env: Environment) -> None:
    env.define("define-palette", BuiltinProcedure("define-palette", _define_palette))
    env.define("palette-ref", BuiltinProcedure("palette-ref", _palette_ref))
    env.define("palette", BuiltinProcedure("palette", _palette_keyword, accepts_kwargs=True))
    env.define("use-palette", BuiltinProcedure("use-palette", _use_palette))
