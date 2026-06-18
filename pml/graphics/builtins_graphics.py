"""PML graphics primitive functions — circle, rect, line, etc."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import PMLTypeError
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform
from pml.types import BuiltinProcedure, Macro, Symbol


def _extract_transform(kwargs: dict[str, Any]) -> AffineTransform:
    """Extract :transform keyword, default to identity."""
    t = kwargs.get("transform")
    if t is None:
        return AffineTransform.identity()
    if isinstance(t, AffineTransform):
        return t
    raise PMLTypeError(f":transform expects AffineTransform, got {type(t).__name__}")


def _extract_blend_mode(kwargs: dict[str, Any]) -> str:
    """Extract :blend-mode keyword, default to 'normal'."""
    return kwargs.get("blend-mode", "normal")


def _extract_opacity(kwargs: dict[str, Any]) -> float:
    """Extract :opacity keyword, default to 1.0."""
    return kwargs.get("opacity", 1.0)


def _circle(x: float, y: float, r: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="circle",
        params={"x": x, "y": y, "r": r},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _rect(x: float, y: float, w: float, h: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="rect",
        params={"x": x, "y": y, "w": w, "h": h},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _ellipse(cx: float, cy: float, rx: float, ry: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="ellipse",
        params={"cx": cx, "cy": cy, "rx": rx, "ry": ry},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _line(x1: float, y1: float, x2: float, y2: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="line",
        params={"x1": x1, "y1": y1, "x2": x2, "y2": y2},
        fill=None,
        stroke=kwargs.get("stroke", "#000000"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _polygon(points: list, **kwargs: Any) -> GraphicObject:
    """points: list of (x y) pairs → list of [x, y] lists."""
    if not isinstance(points, list):
        raise PMLTypeError("polygon: points must be a list of (x y) pairs")
    pts = []
    for p in points:
        if not isinstance(p, list) or len(p) != 2:
            raise PMLTypeError(f"polygon: expected (x y), got {p!r}")
        pts.append((p[0], p[1]))
    return GraphicObject(
        shape_type="polygon",
        params={"points": pts},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _path(d: str | list, **kwargs: Any) -> GraphicObject:
    """SVG path data (string or flat coordinate list)."""
    if isinstance(d, list):
        # Convert flat list [x1,y1,x2,y2,...] to SVG path "M x1 y1 L x2 y2 ..."
        parts: list[str] = []
        for i in range(0, len(d), 2):
            if i == 0:
                parts.extend(["M", str(d[i]), str(d[i + 1])])
            else:
                parts.extend(["L", str(d[i]), str(d[i + 1])])
        d = " ".join(parts)
    return GraphicObject(
        shape_type="path",
        params={"d": d},
        fill=kwargs.get("fill"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _text(s: str, x: float, y: float, **kwargs: Any) -> GraphicObject:
    return GraphicObject(
        shape_type="text",
        params={
            "text": s,
            "x": x,
            "y": y,
            "font_family": kwargs.get("font-family", "default"),
            "font_size": kwargs.get("font-size", 12),
        },
        fill=kwargs.get("fill", "#000000"),
        stroke=kwargs.get("stroke"),
        stroke_width=kwargs.get("stroke-width", 1.0),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _image(src: str, x: float, y: float, *args: Any, **kwargs: Any) -> GraphicObject:
    w = args[0] if len(args) > 0 else None
    h = args[1] if len(args) > 1 else None
    return GraphicObject(
        shape_type="image",
        params={"src": src, "x": x, "y": y, "w": w, "h": h},
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def _group(*children: Any, **kwargs: Any) -> GraphicObject:
    """Group multiple graphic objects."""
    objs = []
    for child in children:
        if isinstance(child, GraphicObject):
            objs.append(child)
        else:
            raise PMLTypeError(f"group: expected GraphicObject, got {type(child).__name__}")
    return GraphicObject(
        shape_type="group",
        children=tuple(objs),
        opacity=_extract_opacity(kwargs),
        blend_mode=_extract_blend_mode(kwargs),
        transform=_extract_transform(kwargs),
    )


def register_primitives(env: Environment) -> None:
    """Register all graphic primitive functions."""
    # These accept keyword args
    kw_fns = {
        "circle": _circle,
        "rect": _rect,
        "ellipse": _ellipse,
        "line": _line,
        "polygon": _polygon,
        "path": _path,
        "text": _text,
        "image": _image,
        "group": _group,
    }
    for name, fn in kw_fns.items():
        env.define(name, BuiltinProcedure(name, fn, accepts_kwargs=True))

    # graphic-object? predicate
    env.define(
        "graphic-object?",
        BuiltinProcedure("graphic-object?", lambda x: isinstance(x, GraphicObject)),
    )

    # gradient constructor
    def _gradient(*args: Any, **kwargs: Any) -> str:
        """Create a gradient fill string for use with :fill.

        Canonical form:
          (gradient :type 'linear :angle 45 :stops '((0 "#ff0000") (1 "#0000ff")))

        Shorthand forms (no keywords):
          (gradient 90 "#a" "#b" "#c")  — linear, 90°, 3 stops
          (gradient "#a" "#b")           — linear, 0°, 2 stops

        Mixed form — positional stops + keyword overrides:
          (gradient "#f00" "#00f" :angle 90)
          (gradient 90 "#a" "#b" "#c" :type 'radial)

        In all cases, keyword arguments (:angle, :stops, :type) take
        priority over positional shorthand.
        """
        stops = kwargs.get("stops")
        grad_type = kwargs.get("type", "linear")
        angle = kwargs.get("angle")  # None if not provided; resolved after positional

        # Process positional args (shorthand)
        if args:
            if isinstance(args[0], (int, float)):
                if angle is None:
                    angle = args[0]
                shorthand_stops = list(args[1:])
            else:
                shorthand_stops = list(args)

            if stops is None:
                stops = shorthand_stops

        if angle is None:
            angle = 0.0

        if stops is None or len(stops) < 2:
            raise PMLTypeError("gradient: at least 2 color stops required")

        grad_type = str(grad_type).strip("'").strip('"')
        if grad_type not in ("linear", "radial"):
            raise PMLTypeError(f"gradient: unknown type '{grad_type}' (expected linear or radial)")

        parts: list[str] = []
        if grad_type == "linear" and angle != 0.0:
            # Format angle cleanly: int if whole number, float otherwise
            if isinstance(angle, float) and angle == int(angle):
                parts.append(f"{int(angle)}deg")
            else:
                parts.append(f"{angle}deg")

        for stop in stops:
            if isinstance(stop, list) and len(stop) == 2:
                pos, color = stop
                if isinstance(pos, (int, float)):
                    parts.append(f"{color} {pos}")
                else:
                    parts.append(str(color))
            elif isinstance(stop, str):
                parts.append(stop)
            else:
                parts.append(str(stop))

        return f"{grad_type}-gradient({', '.join(parts)})"

    env.define("gradient", BuiltinProcedure("gradient", _gradient, accepts_kwargs=True))

    # ------------------------------------------------------------------
    # grid-of macro
    # (grid-of rect (c r) (10 5) :x (* c 25) :y (* r 20) :w 20 :h 15 :fill "#ffd700")
    # Expands to nested do loops with (add (rect ...)) calls
    # ------------------------------------------------------------------
    class GridOfMacro(Macro):
        """(grid-of shape-type (col-var row-var) (ncols nrows) kwarg...)

        Flat LLM-friendly grid generation. Expands to nested do loops
        that call (add (shape-type ...)) for each cell.
        """

        def __init__(self) -> None:
            super().__init__("grid-of", [], [], None)

        def expand(self, args: list) -> list:
            if len(args) < 3:
                raise PMLTypeError(
                    "grid-of: expected (shape-type (col-var row-var) (ncols nrows) kwargs...)"
                )

            shape_type = args[0]
            bindings = args[1] if isinstance(args[1], list) else [Symbol("c"), Symbol("r")]
            dims = args[2] if isinstance(args[2], list) else [args[2], 1]
            body = args[3:]

            col_var = bindings[0] if len(bindings) > 0 else Symbol("c")
            row_var = bindings[1] if len(bindings) > 1 else Symbol("r")
            ncols = dims[0] if len(dims) > 0 else 1
            nrows = dims[1] if len(dims) > 1 else 1

            # Build: (add (shape-type :x ... :y ... :w ...))
            add_call = [Symbol("add"), [shape_type] + body]

            # Build inner col loop: (do ((col 0 (+ col 1))) ((= col ncols)) add-call)
            inner_do = [
                Symbol("do"),
                [[col_var, 0, [Symbol("+"), col_var, 1]]],
                [[Symbol("="), col_var, ncols]],
                add_call,
            ]

            # Wrap in outer row loop if nrows > 1
            if nrows == 1 or nrows == 0:
                return inner_do

            outer_do = [
                Symbol("do"),
                [[row_var, 0, [Symbol("+"), row_var, 1]]],
                [[Symbol("="), row_var, nrows]],
                inner_do,
            ]
            return outer_do

    env.define("grid-of", GridOfMacro())
