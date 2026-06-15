"""PML canvas and sprite-canvas setup."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.environment import Environment
from pml.types import BuiltinProcedure


@dataclass
class Canvas:
    """A drawing surface that collects GraphicObjects for rendering."""

    width: int
    height: int
    bg_color: str = "#FFFFFF"
    objects: list[Any] = field(default_factory=list)
    anchor: str = "top-left"
    padding: int = 0
    is_sprite: bool = False

    def add(self, obj: Any) -> None:
        """Add a graphic object to the canvas."""
        self.objects.append(obj)


# Global canvas reference — set by (canvas ...) and used by (render ...)
_current_canvas: list[Canvas | None] = [None]


def _canvas(w: float, h: float, **kwargs: Any) -> Canvas:
    bg = kwargs.get("bg", "#FFFFFF")
    c = Canvas(width=int(w), height=int(h), bg_color=bg)
    _current_canvas[0] = c
    return c


def _sprite_canvas(w: float, h: float, **kwargs: Any) -> Canvas:
    from pml.types import Symbol

    bg = kwargs.get("bg", "transparent")
    anchor = kwargs.get("anchor", Symbol("center-bottom"))
    padding = kwargs.get("padding", 0)

    anchor_str = anchor.name if isinstance(anchor, Symbol) else str(anchor)

    c = Canvas(
        width=int(w),
        height=int(h),
        bg_color=bg,
        anchor=anchor_str,
        padding=int(padding),
        is_sprite=True,
    )
    _current_canvas[0] = c
    return c


def get_current_canvas() -> Canvas | None:
    return _current_canvas[0]


def register_canvas(env: Environment) -> None:
    env.define("canvas", BuiltinProcedure("canvas", _canvas, accepts_kwargs=True))
    env.define(
        "sprite-canvas",
        BuiltinProcedure("sprite-canvas", _sprite_canvas, accepts_kwargs=True),
    )

    # Override 'add' in the global env so PML code can add objects to canvas
    def _add(obj: Any) -> None:
        c = _current_canvas[0]
        if c is None:
            c = Canvas(width=256, height=256, bg_color="transparent")
            _current_canvas[0] = c
        c.add(obj)

    env.define("add", BuiltinProcedure("add", _add))
