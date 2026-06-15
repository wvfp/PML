"""PML style system — StyleDescriptor, define-style, use-style."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.environment import Environment
from pml.errors import PMLError
from pml.types import BuiltinProcedure, Symbol


@dataclass
class StyleDescriptor:
    """Visual style parameters that propagate to child components."""

    outline_width: float = 2.0
    outline_color: str = "#000000"
    outline_style: str = "solid"  # solid, dashed, dotted, none
    shading: str = "flat"  # flat, cel, gradient, pixel
    shadow: bool = False
    highlight: bool = True
    pixel_size: int = 1
    anti_alias: bool = True
    corner_radius: float = 0.0

    def to_kwargs(self) -> dict[str, Any]:
        """Convert to a flat dict for passing to graphic primitives."""
        return {
            "outline-width": self.outline_width,
            "outline-color": self.outline_color,
            "outline-style": self.outline_style,
            "shading": self.shading,
            "shadow": self.shadow,
            "highlight": self.highlight,
            "pixel-size": self.pixel_size,
            "anti-alias": self.anti_alias,
            "corner-radius": self.corner_radius,
        }

    def merge(self, overrides: dict[str, Any]) -> StyleDescriptor:
        """Return a new StyleDescriptor with overrides applied."""
        import copy
        result = copy.copy(self)
        mapping = {
            "outline-width": "outline_width",
            "outline-color": "outline_color",
            "outline-style": "outline_style",
            "shading": "shading",
            "shadow": "shadow",
            "highlight": "highlight",
            "pixel-size": "pixel_size",
            "anti-alias": "anti_alias",
            "corner-radius": "corner_radius",
        }
        for k, v in overrides.items():
            attr = mapping.get(k, k)
            if hasattr(result, attr):
                setattr(result, attr, v)
        return result


# Predefined styles
_PREDEFINED_STYLES: dict[str, StyleDescriptor] = {
    "cel": StyleDescriptor(
        outline_width=2.5,
        outline_color="#1a1a1a",
        shading="cel",
        highlight=True,
        anti_alias=True,
    ),
    "pixel": StyleDescriptor(
        outline_width=1.0,
        outline_color="#000000",
        shading="pixel",
        highlight=False,
        pixel_size=2,
        anti_alias=False,
    ),
    "flat": StyleDescriptor(
        outline_width=0.0,
        outline_style="none",
        shading="flat",
        highlight=False,
        anti_alias=True,
    ),
}

# Global style registry
_style_registry: dict[str, StyleDescriptor] = dict(_PREDEFINED_STYLES)


def get_style(name: str) -> StyleDescriptor:
    """Get a named style. Falls back to 'cel' if not found."""
    if name in _style_registry:
        return _style_registry[name]
    print(f"Warning: unknown style '{name}', falling back to 'cel'")
    return _style_registry["cel"]


def _define_style(name: Any, **kwargs: Any) -> None:
    """(define-style name :outline-width ... :shading ...)"""
    style_name = name.name if isinstance(name, Symbol) else str(name)
    style = StyleDescriptor(
        outline_width=kwargs.get("outline-width", 2.0),
        outline_color=kwargs.get("outline-color", "#000000"),
        outline_style=kwargs.get("outline-style", "solid"),
        shading=kwargs.get("shading", "flat"),
        shadow=kwargs.get("shadow", False),
        highlight=kwargs.get("highlight", True),
        pixel_size=kwargs.get("pixel-size", 1),
        anti_alias=kwargs.get("anti-alias", True),
        corner_radius=kwargs.get("corner-radius", 0.0),
    )
    _style_registry[style_name] = style


def _use_style(name: Any) -> StyleDescriptor:
    """(use-style name) → StyleDescriptor"""
    style_name = name.name if isinstance(name, Symbol) else str(name)
    return get_style(style_name)


def resolve_style(style_ref: Any) -> StyleDescriptor:
    """Resolve a style reference to a StyleDescriptor.

    Accepts: Symbol, str, or StyleDescriptor.
    """
    if isinstance(style_ref, StyleDescriptor):
        return style_ref
    if isinstance(style_ref, Symbol):
        return get_style(style_ref.name)
    if isinstance(style_ref, str):
        return get_style(style_ref)
    return _PREDEFINED_STYLES["cel"]


def register_style(env: Environment) -> None:
    env.define("define-style", BuiltinProcedure("define-style", _define_style, accepts_kwargs=True))
    env.define("use-style", BuiltinProcedure("use-style", _use_style))
