"""PML Shader System — post-processing, pixel shaders, and procedural effects."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Callable

from pml.environment import Environment
from pml.types import BuiltinProcedure


# ======================================================================
# Shader model
# ======================================================================


ShaderFunc = Callable[..., Any]
"""A shader implementation.

For post-process shaders: ``fn(image: PIL.Image, **kwargs) -> PIL.Image``.
For pixel shaders: ``fn(r: int, g: int, b: int, a: int) -> tuple[int, int, int, int]``.
"""


@dataclass
class Shader:
    """A named, parameterized shader effect."""

    name: str
    """Unique shader name, e.g. ``\"sepia\"``."""

    impl: ShaderFunc
    """Callable that implements the shader effect."""

    shader_type: str = "post-process"
    """``\"post-process\"`` (whole-image filter) or ``\"pixel\"`` (per-pixel function)."""

    description: str = ""
    """Human-readable summary."""

    params: list[dict[str, Any]] = field(default_factory=list)
    """Parameter descriptors for the shader."""


# ======================================================================
# Global shader registry
# ======================================================================

_shader_registry: dict[str, Shader] = {}


def register_shader(shader: Shader) -> None:
    _shader_registry[shader.name] = shader


def get_shader(name: str) -> Shader | None:
    return _shader_registry.get(name)


def list_shader_names() -> list[str]:
    return sorted(_shader_registry.keys())


def list_shaders() -> list[dict[str, Any]]:
    return [
        {
            "name": s.name,
            "type": s.shader_type,
            "description": s.description,
            "params": s.params,
        }
        for s in sorted(_shader_registry.values(), key=lambda x: x.name)
    ]


# ======================================================================
# Post-process pipeline (attached to Canvas)
# ======================================================================


def create_post_process_chain() -> list[dict[str, Any]]:
    """Create an empty post-process chain.

    Each entry: ``{\"name\": str, \"kwargs\": dict}``.
    """
    return []


def apply_post_process_chain(
    image: Any,
    chain: list[dict[str, Any]],
) -> Any:
    """Run a chain of post-process shaders on a PIL Image.

    Each step looks up the shader by name and calls its implementation
    with the image and any keyword arguments.
    """
    from pml.shaders import get_shader

    for step in chain:
        shader = get_shader(step["name"])
        if shader is not None:
            image = shader.impl(image, **step.get("kwargs", {}))
    return image


# ======================================================================
# Pixel shader helpers
# ======================================================================


def pixel_shader_from_lambda(
    fn: Callable[[int, int, int, int], tuple[int, int, int, int]],
    image: Any,
) -> Any:
    """Apply a per-pixel PML lambda to a PIL Image.

    This is the slow fallback path. Built-in shaders use
    Pillow/numpy optimized implementations.
    """
    import numpy as np

    arr = np.array(image.convert("RGBA"), dtype=np.int32)
    h, w = arr.shape[:2]

    for y in range(h):
        for x in range(w):
            r, g, b, a = arr[y, x]
            nr, ng, nb, na = fn(int(r), int(g), int(b), int(a))
            arr[y, x] = (max(0, min(255, nr)),
                         max(0, min(255, ng)),
                         max(0, min(255, nb)),
                         max(0, min(255, na)))

    from PIL import Image
    return Image.fromarray(arr.astype("uint8"), "RGBA")


# ======================================================================
# Registration
# ======================================================================


def register_shaders(env: Environment) -> None:
    """Register shader-related builtins and built-in shader effects."""
    # Register PML builtins (define-shader, post-process, shader, list-shaders)
    register_pml_builtins(env)

    # Register built-in shader effects
    from pml.shaders.builtins import register_builtin_shaders
    register_builtin_shaders()


def register_pml_builtins(env: Environment) -> None:
    """Register PML-level shader builtins."""
    from pml.shaders.builtins_pml import (
        _define_shader,
        _post_process,
        _clear_post_process,
        _shader_wrap,
        _list_shaders,
    )

    env.define(
        "define-shader",
        BuiltinProcedure("define-shader", _define_shader, accepts_kwargs=True),
    )
    env.define(
        "post-process",
        BuiltinProcedure("post-process", _post_process, accepts_kwargs=True),
    )
    env.define(
        "clear-post-process",
        BuiltinProcedure("clear-post-process", _clear_post_process),
    )
    env.define(
        "shader",
        BuiltinProcedure("shader", _shader_wrap, accepts_kwargs=True),
    )
    env.define(
        "list-shaders",
        BuiltinProcedure("list-shaders", _list_shaders),
    )