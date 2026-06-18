"""PML scene/layer system — multi-layer compositing for iterative editing."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.graphics.canvas import Canvas, _current_canvas, get_current_canvas
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# Scene / Layer data model
# ======================================================================


@dataclass
class Layer:
    """A named layer with its own canvas for compositing."""

    name: str
    canvas: Canvas


@dataclass
class Scene:
    """A multi-layer compositing container.

    Layers are rendered in insertion order (background first, foreground last).
    """

    width: int
    height: int
    layers: list[Layer] = field(default_factory=list)
    bg_color: str = "#FFFFFF"

    def add_layer(self, layer: Layer) -> None:
        self.layers.append(layer)

    def get_layer(self, name: str) -> Layer | None:
        for l in self.layers:
            if l.name == name:
                return l
        return None


# Singleton: the active scene
_current_scene: list[Scene | None] = [None]
_layer_stack: list[str] = []  # for with-layer nesting


# ======================================================================
# Scene builtins
# ======================================================================


def _scene(w: float, h: float, **kwargs: Any) -> Scene:
    """(scene :w 400 :h 300 :bg \"#fff\" (layer ...) ...)"""
    scene = Scene(
        width=int(w),
        height=int(h),
        bg_color=str(kwargs.get("bg", kwargs.get("bg_color", "#FFFFFF"))),
    )
    _current_scene[0] = scene
    _layer_stack.clear()
    return scene


def _layer(**kwargs: Any) -> Canvas:
    """(layer :name \"bg\" ... body ...)

    Creates a new layer canvas, registers it with the active scene,
    and sets it as the current canvas for subsequent (add ...) calls.
    """
    name = str(kwargs.get("name", "layer"))
    scene = _current_scene[0]
    if scene is None:
        from pml.errors import PMLError
        raise PMLError("layer requires an active scene — use (scene ...) first")

    # Create a canvas matching the scene dimensions
    ln = Canvas(width=scene.width, height=scene.height, bg_color="transparent")
    scene.add_layer(Layer(name=name, canvas=ln))
    _current_canvas[0] = ln
    _layer_stack.append(name)
    return ln


def _with_layer(**kwargs: Any) -> None:
    """(with-layer \"bg\" ...) — execute body with the named layer as active canvas.

    After execution, restores the previous layer or scene canvas.
    """
    name = str(kwargs.get("name", ""))
    scene = _current_scene[0]
    if scene is None:
        from pml.errors import PMLError
        raise PMLError("with-layer requires an active scene")

    layer = scene.get_layer(name)
    if layer is None:
        from pml.errors import PMLError
        raise PMLError(f"layer '{name}' not found in scene")

    # The body expressions are *already evaluated* by the PML evaluator
    # before this function is called. This builtin just switches the
    # active canvas so subsequent (add ...) calls target this layer.
    _current_canvas[0] = layer.canvas


def _composite_scene(scene: Scene) -> GraphicObject:
    """Render all layers of a scene into a single composite GraphicObject.

    Returns a ``group`` GraphicObject containing all layer objects,
    with layers ordered bg → fg for proper compositing.
    """
    from pml.backend.pillow import PillowBackend
    from PIL import Image

    backend = PillowBackend()
    # Create base surface from scene bg
    base = backend.create_surface(scene.width, scene.height, scene.bg_color)

    for layer in scene.layers:
        canvas = layer.canvas
        if not canvas.objects:
            continue
        # Render each layer onto a transparent surface, then composite
        layer_surface = backend.create_surface(scene.width, scene.height, "transparent")
        for obj in canvas.objects:
            if isinstance(obj, GraphicObject):
                backend.draw(layer_surface, obj)
        # Composite layer onto base
        base.alpha_composite(layer_surface)

    # Return a group wrapping the composited result
    return GraphicObject(
        shape_type="group",
        params={"w": scene.width, "h": scene.height},
        children=(),
        metadata={"scene": True, "composited": True},
    )


def _render_scene(filename: str, **kwargs: Any) -> str:
    """Render the active scene layers into a composited output file.

    Usage: (render \"output.png\")  or  (render \"output.gif\" :fps 30)
    """
    from pml.graphics.render import _resolve_output_path

    scene = _current_scene[0]
    if scene is None:
        from pml.errors import PMLError
        raise PMLError("no active scene to render")

    from pml.backend.pillow import PillowBackend

    out_path = _resolve_output_path(filename)
    fmt = str(kwargs.get("format", "")).upper() or (
        "GIF" if filename.lower().endswith(".gif") else "PNG"
    )

    backend = PillowBackend()
    base = backend.create_surface(scene.width, scene.height, scene.bg_color)

    for layer in scene.layers:
        canvas = layer.canvas
        if not canvas.objects:
            continue
        layer_surface = backend.create_surface(scene.width, scene.height, "transparent")
        for obj in canvas.objects:
            if isinstance(obj, GraphicObject):
                backend.draw(layer_surface, obj)
        base.alpha_composite(layer_surface)

    backend.save_image(base, out_path, fmt)
    return out_path


# ======================================================================
# Registration
# ======================================================================


def register_scene(env: Any) -> None:
    """Register scene/layer builtins into the environment."""
    from pml.types import BuiltinProcedure

    env.define("scene", BuiltinProcedure("scene", _scene, accepts_kwargs=True))
    env.define("layer", BuiltinProcedure("layer", _layer, accepts_kwargs=True))
    env.define(
        "with-layer",
        BuiltinProcedure("with-layer", _with_layer, accepts_kwargs=True),
    )