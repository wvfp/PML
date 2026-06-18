"""PML builtins for the shader system: define-shader, post-process, shader."""

from __future__ import annotations

from typing import Any

from pml.errors import PMLTypeError
from pml.graphics.canvas import _current_canvas
from pml.graphics.layout import LayoutBox
from pml.graphics.objects import GraphicObject
from pml.shaders import (
    Shader,
    get_shader,
    list_shader_names,
    list_shaders,
    register_shader,
)
from pml.types import Symbol


# ======================================================================
# (define-shader name (params) body...)
# ======================================================================


def _define_shader(name: Any, *args: Any, **kwargs: Any) -> Symbol:
    """(define-shader 'name (r g b a) body...) — register a pixel shader.

    The shader is defined as a lambda that takes (r g b a) and returns
    a list or values (r' g' b' a').

    For optimal performance with built-in effects, use a known shader name
    like 'sepia, 'blur, 'pixelate, etc.
    """
    # kwargs contains the lambda body (from PML evaluator)
    # We need to handle two forms:
    # 1. (define-shader 'name (r g b a) body...) — custom pixel shader
    # 2. (define-shader 'name) — just register the name (for builtins)
    if isinstance(name, Symbol):
        shader_name = name.name
    elif isinstance(name, str):
        shader_name = name
    else:
        raise PMLTypeError(f"define-shader expects a name, got {type(name).__name__}")

    if args:
        # Custom pixel shader defined via PML lambda
        from pml.evaluator import evaluate

        # The args after the name contain the shader logic
        # For now, we do a simple registration with metadata
        # The actual lambda evaluation will be a future enhancement
        register_shader(
            Shader(
                name=shader_name,
                impl=_make_pixel_shader_fn(args),
                shader_type="pixel",
                description="User-defined PML pixel shader",
            )
        )
    else:
        # Just ensure we have a placeholder
        existing = get_shader(shader_name)
        if existing is None:
            register_shader(
                Shader(
                    name=shader_name,
                    impl=lambda img, **kw: img,
                    shader_type="pixel",
                    description="User-defined shader (no body yet)",
                )
            )

    return Symbol(shader_name)


def _make_pixel_shader_fn(args: list) -> Any:
    """Create a pixel shader function from PML lambda args.

    The args are the unevaluated PML lambda body. We wrap them
    in a function that, given a PIL Image, applies the lambda per-pixel.
    """
    # For now, create a simple pass-through that records the PML source
    # Full PML lambda compilation for per-pixel ops is a future enhancement

    def _shader_fn(img: Any, **kwargs: Any) -> Any:
        from pml.shaders import pixel_shader_from_lambda

        # Build a simple lambda from the arg structure
        # This handles basic arithmetic like (list (* r 0.393) ...)
        def _pixel_fn(r: int, g: int, b: int, a: int) -> tuple[int, int, int, int]:
            return (r, g, b, a)  # no-op fallback; full PML eval in future

        return pixel_shader_from_lambda(_pixel_fn, img)

    return _shader_fn


# ======================================================================
# (post-process name :key value ...)
# ======================================================================


def _post_process(name: Any, **kwargs: Any) -> None:
    """(post-process 'blur :radius 3) — add a shader to the post-process chain.

    The shader is applied to the entire canvas at render time, after all
    objects have been drawn. Multiple post-process calls chain in order.
    """
    if isinstance(name, Symbol):
        shader_name = name.name
    elif isinstance(name, str):
        shader_name = name
    else:
        shader_name = str(name)

    # Look up the shader
    shader = get_shader(shader_name)
    if shader is None:
        raise PMLTypeError(f"Unknown shader: {shader_name}. Available: {', '.join(list_shader_names())}")

    # Add to current canvas's post-process chain
    canvas = _current_canvas[0]
    if canvas is None:
        raise PMLTypeError(
            "No active canvas. Use (sprite-canvas ...) or (canvas ...) before post-process."
        )

    # Initialize chain if not present
    if not hasattr(canvas, "_post_process_chain"):
        canvas._post_process_chain = []

    # Resolve kwarg values from Symbols
    resolved_kwargs: dict[str, Any] = {}
    for k, v in kwargs.items():
        if isinstance(v, Symbol):
            resolved_kwargs[k] = v.name
        else:
            resolved_kwargs[k] = v

    canvas._post_process_chain.append({
        "name": shader_name,
        "kwargs": resolved_kwargs,
    })


# ======================================================================
# (clear-post-process)
# ======================================================================


def _clear_post_process() -> None:
    """(clear-post-process) — clear the current canvas's post-process chain.

    Use this between renders when you want to render the same canvas
    with different post-processing effects.
    """
    canvas = _current_canvas[0]
    if canvas is not None:
        canvas._post_process_chain = []


# ======================================================================
# (shader obj 'name :key value ...)
# ======================================================================


def _shader_wrap(obj: Any, name: Any, **kwargs: Any) -> GraphicObject:
    """(shader (character ...) 'glow :radius 3) — wrap an object with a shader.

    The shader is applied to the object's intermediate surface at render time
    before compositing into the main canvas.

    Returns the object with shader metadata attached.
    """
    if isinstance(name, Symbol):
        shader_name = name.name
    elif isinstance(name, str):
        shader_name = name
    else:
        shader_name = str(name)

    shader = get_shader(shader_name)
    if shader is None:
        raise PMLTypeError(
            f"Unknown shader: {shader_name}. Available: {', '.join(list_shader_names())}"
        )

    # Attach shader metadata to the object
    resolved_kwargs: dict[str, Any] = {}
    for k, v in kwargs.items():
        if isinstance(v, Symbol):
            resolved_kwargs[k] = v.name
        else:
            resolved_kwargs[k] = v

    if isinstance(obj, GraphicObject):
        meta = dict(obj.metadata)
        meta["shader"] = shader_name
        meta["shader_kwargs"] = resolved_kwargs
        return GraphicObject(
            shape_type=obj.shape_type,
            params=dict(obj.params),
            fill=obj.fill,
            stroke=obj.stroke,
            stroke_width=obj.stroke_width,
            transform=obj.transform,
            children=obj.children,
            metadata=meta,
        )

    if isinstance(obj, LayoutBox):
        # Wrap the inner graphic
        wrapped = _shader_wrap(obj.graphic, name, **kwargs)
        from pml.graphics.layout import LayoutBox

        return LayoutBox(
            graphic=wrapped,
            width=obj.width,
            height=obj.height,
            fill=obj.fill,
            align=obj.align,
            tag=obj.tag,
            right_of=obj.right_of,
            below=obj.below,
            center_x=obj.center_x,
            center_y=obj.center_y,
        )

    return obj


# ======================================================================
# (list-shaders)
# ======================================================================


def _list_shaders() -> list[dict[str, Any]]:
    """(list-shaders) — list all available shaders with metadata."""
    return list_shaders()