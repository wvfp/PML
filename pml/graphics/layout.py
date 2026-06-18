"""PML layout system — row, column, stack, place.

Provides declarative layout primitives for LLM-friendly spatial composition.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.graphics.canvas import _current_canvas
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# LayoutBox — a GraphicObject wrapped with layout constraints
# ======================================================================


@dataclass
class LayoutBox:
    """A positioned element within a layout container."""

    graphic: GraphicObject
    width: int | None = None
    height: int | None = None
    fill: int = 0
    align: str | None = None
    tag: str | None = None
    """Named tag for relative positioning references."""
    right_of: str | None = None
    """Place right of this tag name, with optional gap encoded as 'tag:gap'."""
    below: str | None = None
    """Place below this tag name, with optional gap encoded as 'tag:gap'."""
    center_x: bool = False
    """Center horizontally within the parent canvas."""
    center_y: bool = False
    """Center vertically within the parent canvas."""

    def __repr__(self) -> str:
        parts = [f"<LayoutBox {self.graphic.shape_type}"]
        if self.width:
            parts.append(f" w={self.width}")
        if self.height:
            parts.append(f" h={self.height}")
        if self.fill:
            parts.append(f" fill={self.fill}")
        if self.tag:
            parts.append(f" tag={self.tag}")
        if self.right_of:
            parts.append(f" right-of={self.right_of}")
        parts.append(">")
        return "".join(parts)


def is_layoutbox(obj: Any) -> bool:
    return isinstance(obj, LayoutBox)


def _as_layoutbox(obj: Any) -> LayoutBox:
    """Convert a raw GraphicObject to a default LayoutBox if needed."""
    if isinstance(obj, LayoutBox):
        return obj
    if isinstance(obj, GraphicObject):
        return LayoutBox(graphic=obj)
    return LayoutBox(graphic=obj)


# ======================================================================
# Size helpers
# ======================================================================


def _intrinsic_size(obj: Any) -> tuple[int, int]:
    """Guess the intrinsic size of a graphic object.

    Returns (width, height) in pixels.
    """
    if isinstance(obj, LayoutBox):
        obj = obj.graphic
    if not isinstance(obj, GraphicObject):
        return (64, 64)

    p = obj.params
    shape = obj.shape_type

    # Helper: try common key names
    def _p(key: str, *aliases: str, default: int = 32) -> int:
        for k in (key, *aliases):
            v = p.get(k)
            if v is not None:
                return int(v)
        return default

    # Primitives with explicit geometry
    if shape == "rect":
        return (_p("w", "width", default=32), _p("h", "height", default=32))
    if shape == "circle":
        r = _p("r", "radius", default=16)
        return (r * 2, r * 2)
    if shape == "ellipse":
        return (_p("rx", default=24) * 2, _p("ry", default=16) * 2)
    if shape in ("line",):
        return (32, 32)
    if shape == "text":
        return (_p("width", default=80), _p("size", default=14) + 4)

    # Components
    if shape == "character":
        return (128, 256)
    if shape in ("button",):
        return (_p("w", "width", default=80), _p("h", "height", default=32))
    if shape == "label":
        return (80, 20)
    if shape in ("icon",):
        return (32, 32)
    if shape in ("tile",):
        return (_p("size", default=32),) * 2
    if shape in ("health-bar",):
        return (_p("w", "width", default=80), _p("h", "height", default=10))
    if shape in ("panel",):
        return (_p("w", "width", default=160), _p("h", "height", default=120))
    if shape in ("potion",):
        return (24, 32)
    if shape in ("chest",):
        return (48, 32)
    if shape in ("weapon",):
        return (48, 48)
    if shape in ("outfit",):
        return (64, 64)
    if shape in ("decoration",):
        return (48, 48)
    if shape in ("background",):
        return (256, 192)
    if shape == "group":
        children = obj.children or ()
        if not children:
            return (32, 32)
        max_x = max_y = 0
        for c in children:
            t = c.transform
            cw, ch = _intrinsic_size(c)
            max_x = max(max_x, int(t.e) + cw)
            max_y = max(max_y, int(t.f) + ch)
        return (max_x or 32, max_y or 32)

    return (64, 64)


def _size_of(obj: Any) -> tuple[int, int]:
    """Return the resolved (width, height) for a LayoutBox or GraphicObject."""
    lb = _as_layoutbox(obj)
    w = lb.width
    h = lb.height
    if w is None or h is None:
        iw, ih = _intrinsic_size(lb.graphic)
    return (w or iw, h or ih)


# ======================================================================
# Relative positioning resolver
# ======================================================================


def _parse_tag_ref(ref: str) -> tuple[str, int]:
    """Parse a tag reference string.

    ``"tag:10"`` → ``("tag", 10)``
    ``"tag"`` → ``("tag", 0)``
    ``":last"`` → ``(":last", 0)``
    """
    tag = ref
    gap = 0
    idx = ref.find(":")
    if idx >= 0 and idx < len(ref) - 1:
        # Only split if there's something after the colon
        suffix = ref[idx + 1:]
        try:
            gap = int(suffix)
            tag = ref[:idx]
        except (ValueError, TypeError):
            pass  # colon wasn't a gap separator
    return (tag, gap)


def resolve_relative_positioning(
    lb: LayoutBox,
    canvas: Any,
) -> GraphicObject:
    """Resolve ``:right-of``, ``:below``, ``:center`` on a LayoutBox.

    Returns the GraphicObject with an appropriate AffineTransform applied.
    Falls back to identity if the reference is not found.
    """
    from pml.transform import AffineTransform

    obj = lb.graphic
    w, h = _size_of(lb)

    # Center within canvas
    if lb.center_x or lb.center_y:
        cx = (canvas.width - w) // 2 if lb.center_x else 0
        cy = (canvas.height - h) // 2 if lb.center_y else 0
        return obj.with_transform(AffineTransform.translate(cx, cy))

    # Right of a tagged element
    ref_tag = lb.right_of or lb.below
    if ref_tag:
        ref_tag_str, gap = _parse_tag_ref(ref_tag)
        ref_bounds = canvas._tagged.get(ref_tag_str)
        if ref_bounds is None:
            ref_bounds = canvas._tagged.get(":last")
        if ref_bounds:
            rx, ry, rw, rh = ref_bounds
            if lb.right_of:
                x = rx + rw + gap
                y = ry
            else:  # below
                x = rx
                y = ry + rh + gap
            return obj.with_transform(AffineTransform.translate(x, y))

    # Fallback: no positioning needed
    return obj


def update_tagged(canvas: Any, lb: LayoutBox, graphic: GraphicObject) -> None:
    """Update the canvas tag registry after placing a LayoutBox."""
    w, h = _size_of(lb)
    tx = float(graphic.transform.e) if hasattr(graphic.transform, "e") else 0.0
    ty = float(graphic.transform.f) if hasattr(graphic.transform, "f") else 0.0

    # Update last position
    canvas._last_position = (int(tx + w), int(ty + h))

    # Register by tag name
    if lb.tag:
        canvas._tagged[lb.tag] = (int(tx), int(ty), int(w), int(h))
    else:
        canvas._tagged[":last"] = (int(tx), int(ty), int(w), int(h))
# ======================================================================


def _resolve_row(
    children: list[Any],
    container_w: int,
    *,
    gap: int = 0,
    padding: int = 0,
    align: str = "center",
) -> list[tuple[int, int, GraphicObject]]:
    """Compute positions for children in a horizontal row.

    Returns list of (x, y, transformed_graphic).
    """
    avail_w = container_w - 2 * padding
    boxes = [_as_layoutbox(c) for c in children]

    # Compute sizes
    sizes: list[tuple[int, int]] = []
    total_fixed = 0
    total_fill = 0
    for b in boxes:
        w, h = _size_of(b)
        if b.fill > 0:
            total_fill += b.fill
        else:
            total_fixed += w
        sizes.append((w, h))

    # Distribute remaining space to fill items
    total_gap = gap * (len(boxes) - 1)
    fill_px = avail_w - total_fixed - total_gap
    fill_px = max(0, fill_px)

    # Compute final x positions and apply
    results: list[tuple[int, int, GraphicObject]] = []
    x = padding

    for i, b in enumerate(boxes):
        w, h = sizes[i]
        if b.fill > 0 and total_fill > 0:
            w = int(fill_px * b.fill / total_fill)

        child_align = b.align or align
        max_h = max((s[1] for s in sizes), default=0)

        if child_align == "center":
            y = padding + (max_h - h) // 2
        elif child_align == "end":
            y = padding + max_h - h
        else:
            y = padding

        results.append((x, y, b.graphic))
        x += w + gap

    return results


def _resolve_column(
    children: list[Any],
    container_h: int,
    *,
    gap: int = 0,
    padding: int = 0,
    align: str = "center",
) -> list[tuple[int, int, GraphicObject]]:
    """Compute positions for children in a vertical column."""
    avail_h = container_h - 2 * padding
    boxes = [_as_layoutbox(c) for c in children]

    sizes: list[tuple[int, int]] = []
    total_fixed = 0
    total_fill = 0
    for b in boxes:
        w, h = _size_of(b)
        if b.fill > 0:
            total_fill += b.fill
        else:
            total_fixed += h
        sizes.append((w, h))

    total_gap = gap * (len(boxes) - 1)
    fill_px = avail_h - total_fixed - total_gap
    fill_px = max(0, fill_px)

    results: list[tuple[int, int, GraphicObject]] = []
    # Compute max child width for cross-axis alignment
    max_w = max((s[0] for s in sizes), default=0)
    y = padding

    for i, b in enumerate(boxes):
        w, h = sizes[i]
        if b.fill > 0 and total_fill > 0:
            h = int(fill_px * b.fill / total_fill)

        child_align = b.align or align
        if child_align == "center":
            x = padding + (max_w - w) // 2
        elif child_align == "end":
            x = padding + max_w - w
        else:
            x = padding

        results.append((x, y, b.graphic))
        y += h + gap

    return results


def _resolve_stack(
    children: list[Any],
    *,
    align: str = "center",
) -> list[tuple[int, int, GraphicObject]]:
    """Stack children on the z-axis, each aligned within the max bounds."""
    boxes = [_as_layoutbox(c) for c in children]
    sizes = [_size_of(b) for b in boxes]
    max_w = max((s[0] for s in sizes), default=64)
    max_h = max((s[1] for s in sizes), default=64)

    results: list[tuple[int, int, GraphicObject]] = []
    for b in boxes:
        w, h = _size_of(b)
        child_align = b.align or align
        if child_align == "center":
            x = (max_w - w) // 2
            y = (max_h - h) // 2
        elif child_align == "start":
            x = y = 0
        elif child_align == "end":
            x = max_w - w
            y = max_h - h
        else:
            x = (max_w - w) // 2
            y = (max_h - h) // 2
        results.append((x, y, b.graphic))

    return results


# ======================================================================
# Builtin helpers (called from evaluator)
# ======================================================================


def pml_place(graphic: Any, **kwargs: Any) -> LayoutBox:
    """(place obj :width 60 :align 'center :right-of \"tag\") — wrap with layout hints."""
    if isinstance(graphic, GraphicObject):
        return LayoutBox(
            graphic=graphic,
            width=kwargs.get("width") or kwargs.get("w"),
            height=kwargs.get("height") or kwargs.get("h"),
            fill=kwargs.get("fill", 0),
            align=str(kwargs.get("align", "")).strip("'") if kwargs.get("align") else None,
            tag=str(kwargs.get("tag")).strip("'\"") if kwargs.get("tag") else None,
            right_of=str(kwargs.get("right-of")).strip("'\"") if kwargs.get("right-of") else None,
            below=str(kwargs.get("below")).strip("'\"") if kwargs.get("below") else None,
            center_x=_bool_kw(kwargs, "center-x") or _bool_kw(kwargs, "center"),
            center_y=_bool_kw(kwargs, "center-y") or _bool_kw(kwargs, "center"),
        )
    # If it's already a LayoutBox, merge kwargs
    if isinstance(graphic, LayoutBox):
        return LayoutBox(
            graphic=graphic.graphic,
            width=kwargs.get("width") or graphic.width,
            height=kwargs.get("height") or graphic.height,
            fill=kwargs.get("fill", graphic.fill),
            align=str(kwargs.get("align", "")).strip("'") if kwargs.get("align") else graphic.align,
            tag=str(kwargs.get("tag")).strip("'\"") if kwargs.get("tag") else graphic.tag,
            right_of=str(kwargs.get("right-of")).strip("'\"") if kwargs.get("right-of") else graphic.right_of,
            below=str(kwargs.get("below")).strip("'\"") if kwargs.get("below") else graphic.below,
            center_x=_bool_kw(kwargs, "center-x") or _bool_kw(kwargs, "center") or graphic.center_x,
            center_y=_bool_kw(kwargs, "center-y") or _bool_kw(kwargs, "center") or graphic.center_y,
        )
    return LayoutBox(graphic=graphic)


def _bool_kw(kwargs: dict, key: str) -> bool:
    return kwargs.get(key) is not None and str(kwargs.get(key)) not in ("#f", "false", "nil")


def pml_row(*children: Any, **kwargs: Any) -> None:
    """(row :gap 8 :align 'center children...) — horizontal layout.

    Each child should be a (place ...) or a raw GraphicObject.
    Positions are computed and children added to the current canvas.
    """
    canvas = _current_canvas[0]
    if canvas is None:
        from pml.errors import PMLError
        raise PMLError("row requires an active canvas — use (sprite-canvas ...) or (canvas ...) first")

    gap = int(kwargs.get("gap", 0))
    padding = int(kwargs.get("padding", 0))
    align = str(kwargs.get("align", "center")).strip("'")

    placements = _resolve_row(
        list(children),
        canvas.width,
        gap=gap,
        padding=padding,
        align=align,
    )

    for x, y, obj in placements:
        translated = obj.with_transform(AffineTransform.translate(x, y))
        if _current_canvas[0]:
            _current_canvas[0].add(translated)


def pml_column(*children: Any, **kwargs: Any) -> None:
    """(column :gap 8 :align 'center children...) — vertical layout."""
    canvas = _current_canvas[0]
    if canvas is None:
        from pml.errors import PMLError
        raise PMLError("column requires an active canvas")

    gap = int(kwargs.get("gap", 0))
    padding = int(kwargs.get("padding", 0))
    align = str(kwargs.get("align", "center")).strip("'")

    placements = _resolve_column(
        list(children),
        canvas.height,
        gap=gap,
        padding=padding,
        align=align,
    )

    for x, y, obj in placements:
        translated = obj.with_transform(AffineTransform.translate(x, y))
        if _current_canvas[0]:
            _current_canvas[0].add(translated)


def pml_stack(*children: Any, **kwargs: Any) -> None:
    """(stack :align 'center children...) — z-layer stack layout."""
    align = str(kwargs.get("align", "center")).strip("'")

    placements = _resolve_stack(list(children), align=align)

    for x, y, obj in placements:
        translated = obj.with_transform(AffineTransform.translate(x, y))
        if _current_canvas[0]:
            _current_canvas[0].add(translated)


# ======================================================================
# Registration
# ======================================================================


def register_layout(env: Any) -> None:
    """Register layout builtins into the environment."""
    from pml.types import BuiltinProcedure

    env.define("place", BuiltinProcedure("place", pml_place, accepts_kwargs=True))
    env.define("row", BuiltinProcedure("row", pml_row, accepts_kwargs=True))
    env.define("column", BuiltinProcedure("column", pml_column, accepts_kwargs=True))
    env.define("stack", BuiltinProcedure("stack", pml_stack, accepts_kwargs=True))