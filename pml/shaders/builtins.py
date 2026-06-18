"""Built-in PML shader effects — Pillow + numpy backend."""

from __future__ import annotations

from typing import Any

import numpy as np
from PIL import Image, ImageFilter

from pml.shaders import Shader, register_shader


def _ensure_rgba(img: Image.Image) -> Image.Image:
    """Convert image to RGBA if needed."""
    if img.mode != "RGBA":
        return img.convert("RGBA")
    return img


# ======================================================================
# Color transforms
# ======================================================================


def _sepia(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Apply sepia tone."""
    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    r, g, b, a = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2], arr[:, :, 3]

    nr = np.clip(r * 0.393 + g * 0.769 + b * 0.189, 0, 255)
    ng = np.clip(r * 0.349 + g * 0.686 + b * 0.168, 0, 255)
    nb = np.clip(r * 0.272 + g * 0.534 + b * 0.131, 0, 255)

    result = np.stack([nr, ng, nb, a], axis=2).astype(np.uint8)
    return Image.fromarray(result, "RGBA")


def _grayscale(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Convert to grayscale."""
    gray = img.convert("L")
    return gray.convert("RGBA")


def _invert(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Invert colors."""
    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.uint8)
    arr[:, :, :3] = 255 - arr[:, :, :3]  # invert R, G, B; keep alpha
    return Image.fromarray(arr, "RGBA")


def _brightness(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Adjust brightness. ``:factor 1.2`` = +20%."""
    factor = float(kwargs.get("factor", 1.0))
    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    arr[:, :, :3] = np.clip(arr[:, :, :3] * factor, 0, 255)
    return Image.fromarray(arr.astype(np.uint8), "RGBA")


def _contrast(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Adjust contrast. ``:factor 1.5`` = +50%."""
    factor = float(kwargs.get("factor", 1.0))
    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    # (value - 128) * factor + 128
    arr[:, :, :3] = np.clip((arr[:, :, :3] - 128) * factor + 128, 0, 255)
    return Image.fromarray(arr.astype(np.uint8), "RGBA")


def _color_grade(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Color grading via tint. ``:tint \"#ff8800\" :strength 0.3``."""
    tint_hex = str(kwargs.get("tint", "#ffffff"))
    strength = float(kwargs.get("strength", 0.3))

    # Parse tint color
    tint_hex = tint_hex.lstrip("#")
    tr = int(tint_hex[0:2], 16)
    tg = int(tint_hex[2:4], 16)
    tb = int(tint_hex[4:6], 16)

    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    r, g, b, a = arr[:, :, 0], arr[:, :, 1], arr[:, :, 2], arr[:, :, 3]

    arr[:, :, 0] = np.clip(r * (1 - strength) + tr * strength, 0, 255)
    arr[:, :, 1] = np.clip(g * (1 - strength) + tg * strength, 0, 255)
    arr[:, :, 2] = np.clip(b * (1 - strength) + tb * strength, 0, 255)

    return Image.fromarray(arr.astype(np.uint8), "RGBA")


# ======================================================================
# Filters (Pillow ImageFilter)
# ======================================================================


def _blur(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Gaussian blur. ``:radius 3``."""
    radius = float(kwargs.get("radius", 2))
    return img.filter(ImageFilter.GaussianBlur(radius=radius))


def _sharpen(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Sharpen filter."""
    factor = float(kwargs.get("factor", 2.0))
    sharpened = img.filter(ImageFilter.SHARPEN)
    # Blend original and sharpened based on factor
    return Image.blend(img, sharpened, min(factor / 5, 1.0))


def _edge_detect(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Edge detection."""
    edge = img.filter(ImageFilter.FIND_EDGES)
    return edge


def _emboss(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Emboss effect."""
    return img.filter(ImageFilter.EMBOSS)


def _contour(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Contour effect."""
    return img.filter(ImageFilter.CONTOUR)


def _smooth(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Smoothing/blur. ``:iterations 1``."""
    iterations = int(kwargs.get("iterations", 1))
    result = img
    for _ in range(iterations):
        result = result.filter(ImageFilter.SMOOTH)
    return result


# ======================================================================
# Stylization
# ======================================================================


def _pixelate(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Pixelate effect. ``:size 8`` pixels per block."""
    size = int(kwargs.get("size", 8))
    w, h = img.size
    # Downscale to pixel-grid resolution, then upscale back
    small = img.resize((max(1, w // size), max(1, h // size)), Image.NEAREST)
    return small.resize((w, h), Image.NEAREST)


def _vignette(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Vignette darkening. ``:strength 0.5``."""
    strength = float(kwargs.get("strength", 0.4))
    img = _ensure_rgba(img)
    w, h = img.size

    # Create radial gradient
    X, Y = np.meshgrid(np.linspace(-1, 1, w), np.linspace(-1, 1, h))
    dist = np.sqrt(X**2 + Y**2)
    mask = np.clip(1 - dist * strength, 0, 1)

    arr = np.array(img, dtype=np.float32)
    for c in range(3):
        arr[:, :, c] = arr[:, :, c] * mask
    return Image.fromarray(arr.astype(np.uint8), img.mode)


def _bloom(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Bloom/glow effect. ``:radius 5 :strength 0.3``."""
    radius = float(kwargs.get("radius", 5))
    strength = float(kwargs.get("strength", 0.3))

    img = _ensure_rgba(img)
    # Extract bright areas
    arr = np.array(img, dtype=np.float32)
    gray = np.mean(arr[:, :, :3], axis=2)
    bright_mask = gray > 180

    # Create glow layer from bright areas
    glow = np.zeros_like(arr)
    glow[bright_mask] = arr[bright_mask]
    glow_img = Image.fromarray(glow.astype(np.uint8), "RGBA")

    # Blur the glow
    glow_blurred = glow_img.filter(ImageFilter.GaussianBlur(radius=radius))

    # Composite: img + glow * strength
    img_arr = np.array(img, dtype=np.float32)
    glow_arr = np.array(glow_blurred, dtype=np.float32)
    result = img_arr + glow_arr * strength
    result[:, :, :3] = np.clip(result[:, :, :3], 0, 255)

    return Image.fromarray(result.astype(np.uint8), "RGBA")


def _oil_paint(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Oil paint / cartoon effect. ``:range 3 :levels 4``."""
    rng = int(kwargs.get("range", 3))
    levels = int(kwargs.get("levels", 4))

    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.int32)
    h, w = arr.shape[:2]
    result = np.zeros_like(arr)

    # Simplified oil paint: posterize + median in small window
    # Step 1: Posterize (reduce color levels)
    bin_size = 256 // levels
    posterized = (arr // bin_size) * bin_size + bin_size // 2
    posterized = np.clip(posterized, 0, 255)

    # Step 2: Apply median-like local averaging in range window
    for dy in range(-rng, rng + 1):
        for dx in range(-rng, rng + 1):
            shifted = np.roll(np.roll(posterized, dy, axis=0), dx, axis=1)
            result += shifted

    denom = (2 * rng + 1) ** 2
    result = (result // denom).astype(np.uint8)

    return Image.fromarray(result, "RGBA")


def _crt_scanline(img: Image.Image, **kwargs: Any) -> Image.Image:
    """CRT monitor scanline effect. ``:strength 0.2``."""
    strength = float(kwargs.get("strength", 0.15))
    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    h = arr.shape[0]

    # Darken every other row
    for y in range(0, h, 2):
        arr[y, :, :3] *= (1 - strength)

    return Image.fromarray(arr.astype(np.uint8), "RGBA")


def _noise(img: Image.Image, **kwargs: Any) -> Image.Image:
    """Add grain/noise. ``:amount 0.1 :monochrome #t``."""
    amount = float(kwargs.get("amount", 0.05))
    mono = bool(kwargs.get("monochrome", False))

    img = _ensure_rgba(img)
    arr = np.array(img, dtype=np.float32)
    h, w = arr.shape[:2]

    if mono:
        noise = np.random.normal(0, amount * 255, (h, w))
        for c in range(3):
            arr[:, :, c] = np.clip(arr[:, :, c] + noise, 0, 255)
    else:
        noise = np.random.normal(0, amount * 255, (h, w, 3))
        arr[:, :, :3] = np.clip(arr[:, :, :3] + noise, 0, 255)

    return Image.fromarray(arr.astype(np.uint8), "RGBA")


# ======================================================================
# Registration
# ======================================================================


def register_builtin_shaders() -> None:
    """Register all built-in shader effects."""
    builtins = [
        Shader("sepia", _sepia, "post-process",
               "Warm sepia tone effect"),
        Shader("grayscale", _grayscale, "post-process",
               "Convert to grayscale"),
        Shader("invert", _invert, "post-process",
               "Invert all colors"),
        Shader("brightness", _brightness, "post-process",
               "Adjust brightness", [{"name": "factor", "type": "float", "default": 1.0}]),
        Shader("contrast", _contrast, "post-process",
               "Adjust contrast", [{"name": "factor", "type": "float", "default": 1.0}]),
        Shader("color-grade", _color_grade, "post-process",
               "Color grading via tint", [
                   {"name": "tint", "type": "color", "default": "#ffffff"},
                   {"name": "strength", "type": "float", "default": 0.3},
               ]),
        Shader("blur", _blur, "post-process",
               "Gaussian blur", [{"name": "radius", "type": "float", "default": 2}]),
        Shader("sharpen", _sharpen, "post-process",
               "Sharpen image", [{"name": "factor", "type": "float", "default": 2.0}]),
        Shader("edge-detect", _edge_detect, "post-process",
               "Edge detection filter"),
        Shader("emboss", _emboss, "post-process",
               "Emboss relief effect"),
        Shader("contour", _contour, "post-process",
               "Contour outline effect"),
        Shader("smooth", _smooth, "post-process",
               "Smooth/blur", [{"name": "iterations", "type": "int", "default": 1}]),
        Shader("pixelate", _pixelate, "post-process",
               "Pixelation effect", [{"name": "size", "type": "int", "default": 8}]),
        Shader("vignette", _vignette, "post-process",
               "Vignette darkening", [{"name": "strength", "type": "float", "default": 0.4}]),
        Shader("bloom", _bloom, "post-process",
               "Bloom/glow effect", [
                   {"name": "radius", "type": "float", "default": 5},
                   {"name": "strength", "type": "float", "default": 0.3},
               ]),
        Shader("oil-paint", _oil_paint, "post-process",
               "Oil paint / cartoon effect", [
                   {"name": "range", "type": "int", "default": 3},
                   {"name": "levels", "type": "int", "default": 4},
               ]),
        Shader("crt-scanline", _crt_scanline, "post-process",
               "CRT monitor scanline overlay", [{"name": "strength", "type": "float", "default": 0.15}]),
        Shader("noise", _noise, "post-process",
               "Add film grain / noise", [
                   {"name": "amount", "type": "float", "default": 0.05},
                   {"name": "monochrome", "type": "bool", "default": False},
               ]),
    ]

    for shader in builtins:
        register_shader(shader)