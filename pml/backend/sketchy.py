"""Hand-drawn / sketch rendering backend — pencil, watercolor, hatch.

Uses numpy for Perlin-style noise, organic distortions, and painterly compositing.
All functions take (img: PIL.Image, obj: GraphicObject) and draw in-place.
"""

from __future__ import annotations

import math
from typing import Any

import numpy as np
from PIL import Image, ImageDraw

from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# Noise utilities
# ======================================================================


def _value_noise_2d(w: int, h: int, scale: float = 12.0, seed: int = 0) -> np.ndarray:
    """Generate smooth 2D value noise (bilinear-interpolated grid)."""
    rng = np.random.RandomState(seed)
    gw = max(2, int(w / scale) + 3)
    gh = max(2, int(h / scale) + 3)
    grid = rng.rand(gh, gw)

    xs = np.linspace(0.0, gw - 1, w)
    ys = np.linspace(0.0, gh - 1, h)
    X, Y = np.meshgrid(xs, ys)

    x0 = np.floor(X).astype(int)
    y0 = np.floor(Y).astype(int)
    x1 = np.minimum(x0 + 1, gw - 1)
    y1 = np.minimum(y0 + 1, gh - 1)

    fx = X - x0
    fy = Y - y0
    fx = fx * fx * (3.0 - 2.0 * fx)  # smoothstep
    fy = fy * fy * (3.0 - 2.0 * fy)

    c00 = grid[y0, x0]
    c10 = grid[y0, x1]
    c01 = grid[y1, x0]
    c11 = grid[y1, x1]

    top = c00 + (c10 - c00) * fx
    bot = c01 + (c11 - c01) * fx
    return top + (bot - top) * fy


def _warp_points(
    points: list[tuple[float, float]],
    roughness: float,
    size: tuple[int, int],
    seed: int = 0,
) -> list[tuple[float, float]]:
    """Apply smooth warping to a list of (x, y) points."""
    if roughness <= 0 or not points:
        return points
    w, h = size
    amplitude = max(w, h) * roughness * 0.04
    noise_x = _value_noise_2d(w, h, scale=20.0, seed=seed)
    noise_y = _value_noise_2d(w, h, scale=20.0, seed=seed + 100)
    result = []
    for px, py in points:
        ix = int(np.clip(px, 0, w - 1))
        iy = int(np.clip(py, 0, h - 1))
        dx = (noise_x[iy, ix] - 0.5) * 2 * amplitude
        dy = (noise_y[iy, ix] - 0.5) * 2 * amplitude
        result.append((px + dx, py + dy))
    return result


# ======================================================================
# Pencil — variable-width hand-drawn line with wobble
# ======================================================================


def draw_pencil(img: Image.Image, obj: GraphicObject) -> None:
    """Draw a hand-drawn line with positional wobble and width variation.

    Params: x1, y1, x2, y2
    Kwargs:
      :stroke          — color (default #000)
      :stroke-width    — base width (default 2.0)
      :roughness       — wobble amount 0–1 (default 0.3)
      :variance        — width variation 0–1 (default 0.4)
      :seed            — noise seed for reproducibility (default auto)
    """
    t: AffineTransform = obj.transform
    p1 = t.apply(obj.params["x1"], obj.params["y1"])
    p2 = t.apply(obj.params["x2"], obj.params["y2"])

    stroke = _parse_sketch_color(obj.stroke, (0, 0, 0, 255))
    base_width = max(0.5, obj.stroke_width)
    roughness = float(obj.params.get("roughness", 0.3))
    variance = float(obj.params.get("variance", 0.4))
    seed = int(obj.params.get("seed", hash((p1, p2)) % 2**31))
    w, h = img.size

    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    length = math.hypot(dx, dy)
    if length < 1:
        return

    # Number of segments — more for longer / rougher lines
    num_seg = max(4, int(length * max(0.5, roughness) * 0.4))
    draw = ImageDraw.Draw(img, "RGBA")
    rng = np.random.RandomState(seed)

    # Generate noise for wobble and width
    wobble_noise = rng.randn(num_seg + 1) * roughness * max(w, h) * 0.015
    width_noise = rng.rand(num_seg + 1) * 2 - 1  # [-1, 1]

    udx, udy = -dy / length, dx / length  # perpendicular unit

    for i in range(num_seg):
        t0 = i / num_seg
        t1 = (i + 1) / num_seg

        # Position along line
        bx0 = p1[0] + dx * t0
        by0 = p1[1] + dy * t0
        bx1 = p1[0] + dx * t1
        by1 = p1[1] + dy * t1

        # Wobble offset
        wo0 = wobble_noise[i]
        wo1 = wobble_noise[i + 1]

        # Width at each endpoint
        half_w0 = base_width * 0.5 * (1 + variance * width_noise[i])
        half_w1 = base_width * 0.5 * (1 + variance * width_noise[i + 1])

        # Four corners of the segment quad
        quad = [
            (bx0 + udx * wo0 - udx * half_w0, by0 + udy * wo0 - udy * half_w0),
            (bx0 + udx * wo0 + udx * half_w0, by0 + udy * wo0 + udy * half_w0),
            (bx1 + udx * wo1 + udx * half_w1, by1 + udy * wo1 + udy * half_w1),
            (bx1 + udx * wo1 - udx * half_w1, by1 + udy * wo1 - udy * half_w1),
        ]

        draw.polygon(quad, fill=stroke)


# ======================================================================
# Charcoal — rough stroke with scattered particles
# ======================================================================


def draw_charcoal(img: Image.Image, obj: GraphicObject) -> None:
    """Draw a rough charcoal stroke with particle scattering.

    Params: x1, y1, x2, y2
    Kwargs:
      :stroke          — color (default #222)
      :stroke-width    — base width (default 4.0)
      :roughness       — wobble 0–1 (default 0.5)
      :scatter         — particle scatter 0–1 (default 0.3)
      :seed
    """
    t: AffineTransform = obj.transform
    p1 = t.apply(obj.params["x1"], obj.params["y1"])
    p2 = t.apply(obj.params["x2"], obj.params["y2"])

    stroke = _parse_sketch_color(obj.stroke, (34, 34, 34, 255))
    base_width = max(1.0, obj.stroke_width)
    roughness = float(obj.params.get("roughness", 0.5))
    scatter = float(obj.params.get("scatter", 0.3))
    seed = int(obj.params.get("seed", hash((p1, p2)) % 2**31))
    w, h = img.size

    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    length = math.hypot(dx, dy)
    if length < 1:
        return

    rng = np.random.RandomState(seed)
    draw = ImageDraw.Draw(img, "RGBA")
    w, h = img.size

    # Main stroke — similar to pencil but thicker with more wobble
    udx, udy = -dy / length, dx / length
    num_seg = max(6, int(length * roughness * 0.5))
    wobble = rng.randn(num_seg + 1) * roughness * max(w, h) * 0.02
    width_var = rng.rand(num_seg + 1) * 2 - 1

    for i in range(num_seg):
        t0 = i / num_seg
        t1 = (i + 1) / num_seg
        bx0 = p1[0] + dx * t0
        by0 = p1[1] + dy * t0
        bx1 = p1[0] + dx * t1
        by1 = p1[1] + dy * t1
        hw0 = base_width * 0.4 * (1 + 0.5 * width_var[i])
        hw1 = base_width * 0.4 * (1 + 0.5 * width_var[i + 1])
        quad = [
            (bx0 + udx * wobble[i] - udx * hw0, by0 + udy * wobble[i] - udy * hw0),
            (bx0 + udx * wobble[i] + udx * hw0, by0 + udy * wobble[i] + udy * hw0),
            (bx1 + udx * wobble[i + 1] + udx * hw1, by1 + udy * wobble[i + 1] + udy * hw1),
            (bx1 + udx * wobble[i + 1] - udx * hw1, by1 + udy * wobble[i + 1] - udy * hw1),
        ]
        draw.polygon(quad, fill=stroke)

    # Particle scatter — sprinkle dots along the line
    if scatter > 0:
        num_particles = int(length * scatter * 0.5)
        for _ in range(num_particles):
            t = rng.rand()
            bx = p1[0] + dx * t
            by = p1[1] + dy * t
            offset_x = rng.randn() * scatter * base_width * 2
            offset_y = rng.randn() * scatter * base_width * 2
            pr = rng.rand() * base_width * 0.3 * scatter
            draw.ellipse(
                [bx + offset_x - pr, by + offset_y - pr,
                 bx + offset_x + pr, by + offset_y + pr],
                fill=stroke,
            )


# ======================================================================
# Watercolor fill — layered translucent fills with bleeding edges
# ======================================================================


def _make_noise_alpha_mask(
    w: int, h: int,
    base_alpha: np.ndarray,
    bleed: float = 0.3,
    layers: int = 3,
    seed: int = 0,
) -> np.ndarray:
    """Build a noise-distorted alpha mask for watercolor edge bleeding."""
    # Start with the base alpha mask
    mask = base_alpha.astype(np.float32)

    for layer in range(layers):
        scale = max(8, 20 - layers * 3)
        noise = _value_noise_2d(w, h, scale=scale, seed=seed + layer * 50)
        # Erode/dilate edges based on noise × bleed
        # Morphological dilation of base_alpha for edge region
        try:
            from scipy.ndimage import binary_dilation, binary_erosion
            dilated = binary_dilation(base_alpha > 0.5, iterations=3).astype(float)
            eroded = binary_erosion(base_alpha > 0.5, iterations=2).astype(float)
            edge_region = dilated - eroded
        except ImportError:
            # Fallback: simple edge detection via gradient
            edge_region = np.zeros_like(mask)
            edge_region[1:, :] += np.abs(mask[1:, :] - mask[:-1, :])
            edge_region[:, 1:] += np.abs(mask[:, 1:] - mask[:, :-1])
            edge_region = np.clip(edge_region, 0, 1)

        # Noise drives edge bleed — positive noise pushes alpha out, negative pulls in
        noise_shift = (noise - 0.5) * bleed * 2
        edge_bleed = edge_region * noise_shift
        mask = mask + edge_bleed * (1.0 / layers)

    return np.clip(mask, 0, 1)


def draw_watercolor_rect(img: Image.Image, obj: GraphicObject) -> None:
    """Draw a rectangle with watercolor edge bleeding.

    Params: x, y, w, h
    Kwargs:
      :fill            — color
      :bleed           — edge bleeding 0–1 (default 0.3)
      :layers          — translucent overlay passes (default 3)
      :seed
    """
    t: AffineTransform = obj.transform
    x, y = obj.params["x"], obj.params["y"]
    w = float(obj.params["w"])
    h = float(obj.params["h"])

    fill = _parse_sketch_color(obj.fill, (200, 100, 50, 200))
    bleed = float(obj.params.get("bleed", 0.3))
    layers = int(obj.params.get("layers", 3))
    seed = int(obj.params.get("seed", 42))

    # Create a temporary surface for the base shape and noise mask
    sw, sh = img.size
    temp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    temp_draw = ImageDraw.Draw(temp, "RGBA")

    # Draw solid rect on temp
    corners = [t.apply(x, y), t.apply(x + w, y),
               t.apply(x + w, y + h), t.apply(x, y + h)]
    temp_draw.polygon(corners, fill=(255, 255, 255, 255))
    temp_arr = np.array(temp)

    base_alpha = temp_arr[:, :, 3].astype(np.float32) / 255.0

    # Generate noise-distorted alphas for each layer
    alpha_mask = _make_noise_alpha_mask(sw, sh, base_alpha, bleed, layers, seed)

    # Each layer has slightly different color and opacity
    rng = np.random.RandomState(seed)
    for layer in range(layers):
        color_shift = (rng.rand(3) - 0.5) * 20
        base_color = (
            max(0, min(255, fill[0] + int(color_shift[0]))),
            max(0, min(255, fill[1] + int(color_shift[1]))),
            max(0, min(255, fill[2] + int(color_shift[2]))),
        )
        layer_scale = 0.3 + rng.rand() * 0.3
        per_pixel_alpha = np.clip(alpha_mask * layer_scale * 255, 0, 255).astype(np.uint8)

        layer_arr = np.zeros((sh, sw, 4), dtype=np.uint8)
        layer_arr[:, :, :3] = base_color
        layer_arr[:, :, 3] = per_pixel_alpha
        img.alpha_composite(Image.fromarray(layer_arr, "RGBA"))


def draw_watercolor_circle(img: Image.Image, obj: GraphicObject) -> None:
    """Draw a circle with watercolor edge bleeding.

    Params: cx, cy, r
    Kwargs:
      :fill, :bleed, :layers, :seed
    """
    t: AffineTransform = obj.transform
    cx, cy = t.apply(obj.params["cx"], obj.params["cy"])
    r = float(obj.params["r"]) * math.sqrt(abs(t.a * t.d - t.b * t.c))

    fill = _parse_sketch_color(obj.fill, (200, 100, 50, 200))
    bleed = float(obj.params.get("bleed", 0.3))
    layers = int(obj.params.get("layers", 3))
    seed = int(obj.params.get("seed", 42))

    sw, sh = img.size
    temp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    temp_draw = ImageDraw.Draw(temp, "RGBA")
    temp_draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(255, 255, 255, 255))
    temp_arr = np.array(temp)
    base_alpha = temp_arr[:, :, 3].astype(np.float32) / 255.0

    alpha_mask = _make_noise_alpha_mask(sw, sh, base_alpha, bleed, layers, seed)

    rng = np.random.RandomState(seed)
    for layer in range(layers):
        color_shift = (rng.rand(3) - 0.5) * 20
        base_color = (
            max(0, min(255, fill[0] + int(color_shift[0]))),
            max(0, min(255, fill[1] + int(color_shift[1]))),
            max(0, min(255, fill[2] + int(color_shift[2]))),
        )
        layer_scale = 0.3 + rng.rand() * 0.3
        per_pixel_alpha = np.clip(alpha_mask * layer_scale * 255, 0, 255).astype(np.uint8)

        layer_arr = np.zeros((sh, sw, 4), dtype=np.uint8)
        layer_arr[:, :, :3] = base_color
        layer_arr[:, :, 3] = per_pixel_alpha
        img.alpha_composite(Image.fromarray(layer_arr, "RGBA"))


# ======================================================================
# Hatch — hatching / cross-hatching fill within a bounding area
# ======================================================================


def draw_hatch(img: Image.Image, obj: GraphicObject) -> None:
    """Draw hatching lines within a bounded area.

    Params: x, y, w, h  (bounding box)
    Kwargs:
      :stroke          — line color (default #333)
      :stroke-width    — line thickness (default 1.0)
      :density         — line spacing 0–1 (default 0.4)
      :angle           — degrees (default 45)
      :cross           — cross-hatch? (default #f)
      :cross-angle     — second angle if cross (default 135)
      :seed
    """
    t: AffineTransform = obj.transform
    x, y = obj.params["x"], obj.params["y"]
    w = float(obj.params["w"])
    h = float(obj.params["h"])

    stroke = _parse_sketch_color(obj.stroke, (51, 51, 51, 255))
    line_width = max(0.5, obj.stroke_width)
    density = float(obj.params.get("density", 0.4))
    angle = float(obj.params.get("angle", 45.0))
    cross_h = _is_truthy(obj.params.get("cross", False))
    cross_angle = float(obj.params.get("cross-angle", 135.0))
    seed = int(obj.params.get("seed", 42))

    # Transform bounding box corners
    corners = [
        t.apply(x, y),
        t.apply(x + w, y),
        t.apply(x + w, y + h),
        t.apply(x, y + h),
    ]
    xs = [c[0] for c in corners]
    ys = [c[1] for c in corners]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)

    rng = np.random.RandomState(seed)
    draw = ImageDraw.Draw(img, "RGBA")

    def _hatch_lines(ang: float, seed_offset: int) -> None:
        rad = math.radians(ang)
        spacing = max(2, (1 - density) * 30 + 2)
        diag = math.hypot(max_x - min_x, max_y - min_y) * 1.5
        cx = (min_x + max_x) / 2
        cy = (min_y + max_y) / 2
        local_rng = np.random.RandomState(seed + seed_offset)

        num_lines = int(diag / spacing) + 2
        for i in range(num_lines):
            offset = (i - num_lines // 2) * spacing + local_rng.rand() * spacing * 0.2
            px = cx + offset * math.cos(rad + math.pi / 2)
            py = cy + offset * math.sin(rad + math.pi / 2)
            lx1 = px + diag * math.cos(rad)
            ly1 = py + diag * math.sin(rad)
            lx2 = px - diag * math.cos(rad)
            ly2 = py - diag * math.sin(rad)

            wobble = local_rng.randn() * max(w, h) * 0.005
            lw = line_width + local_rng.rand() * 0.5

            draw.line(
                [lx1 + wobble, ly1 + wobble, lx2 + wobble * 2, ly2 + wobble * 2],
                fill=stroke,
                width=max(1, int(lw)),
            )

    _hatch_lines(angle, 0)
    if cross_h:
        _hatch_lines(cross_angle, 1000)


# ======================================================================
# Sketch modifier — apply pencil-style warping to any GraphicObject
# ======================================================================


def sketchify_graphic(img: Image.Image, obj: GraphicObject) -> None:
    """Apply sketch distortion to a GraphicObject after normal rendering.

    This is called AFTER the normal draw pass. It renders the object to
    a temp surface, applies noise-based warping, and composites back.

    Params: base-shape (GraphicObject), roughness
    """
    # Extract wrapped shape from params
    base_obj = obj.params.get("base-shape")
    if base_obj is None or not isinstance(base_obj, GraphicObject):
        return
    roughness = float(obj.params.get("roughness", 0.2))
    seed = int(obj.params.get("seed", 42))

    # Render the base object to a temp surface
    sw, sh = img.size
    temp = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    from pml.backend.pillow import PillowBackend
    backend = PillowBackend()
    backend.draw(temp, base_obj)

    # Apply warp using noise displacement map
    if roughness > 0:
        temp_arr = np.array(temp).astype(np.float32)
        h, w = temp_arr.shape[:2]
        noise_x = (_value_noise_2d(w, h, scale=15, seed=seed) - 0.5) * roughness * max(w, h) * 0.05
        noise_y = (_value_noise_2d(w, h, scale=15, seed=seed + 200) - 0.5) * roughness * max(w, h) * 0.05

        # Create meshgrid for remap
        X, Y = np.meshgrid(np.arange(w), np.arange(h))
        X_warp = np.clip(X + noise_x, 0, w - 1).astype(np.float32)
        Y_warp = np.clip(Y + noise_y, 0, h - 1).astype(np.float32)

        try:
            from scipy.ndimage import map_coordinates
            warped = np.zeros_like(temp_arr)
            for c in range(4):
                warped[:, :, c] = map_coordinates(
                    temp_arr[:, :, c], [Y_warp, X_warp], order=1
                )
            temp = Image.fromarray(warped.astype(np.uint8), "RGBA")
        except ImportError:
            pass  # Skip warp if scipy unavailable

    img.alpha_composite(temp)


# ======================================================================
# Noise fill — polygon with noise-modulated color
# ======================================================================


def draw_noise_fill(img: Image.Image, obj: GraphicObject) -> None:
    """Fill a polygon with noise-modulated color variation.

    Params: vertices (list of [x, y] pairs)
    Kwargs:
      :fill            — base hex color
      :intensity       — noise strength 0.0–1.0 (default 0.15)
      :scale           — noise grid scale (default 12.0)
      :seed            — random seed (default 0)
    """
    from pml.backend.pillow import parse_color

    vertices_raw = obj.params.get("vertices", [])
    if not vertices_raw:
        return

    # Convert PML lists to (x, y) tuples
    vertices = [(float(v[0]), float(v[1])) for v in vertices_raw]

    # Parse base color
    rgba = parse_color(obj.fill)
    if rgba is None:
        return
    base_r, base_g, base_b, base_a = rgba

    intensity = float(obj.params.get("intensity", 0.15))
    scale = float(obj.params.get("scale", 12.0))
    seed = int(obj.params.get("seed", 0))

    w, h = img.size

    # Create polygon mask
    mask = Image.new("L", (w, h), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.polygon(vertices, fill=255)

    # Generate smooth value noise across the canvas
    noise = _value_noise_2d(w, h, scale=scale, seed=seed)

    # Modulate base color with noise: factor ranges [-intensity, +intensity]
    factor = (noise - 0.5) * 2.0 * intensity

    nr = np.clip(base_r * (1.0 + factor), 0, 255).astype(np.uint8)
    ng = np.clip(base_g * (1.0 + factor), 0, 255).astype(np.uint8)
    nb = np.clip(base_b * (1.0 + factor), 0, 255).astype(np.uint8)
    na = np.full_like(nr, base_a, dtype=np.uint8)

    result = np.stack([nr, ng, nb, na], axis=2)
    noise_img = Image.fromarray(result, "RGBA")

    # Composite only within the polygon mask
    img.paste(noise_img, (0, 0), mask)


# ======================================================================
# Color / truthy helpers
# ======================================================================


def _parse_sketch_color(
    color: str | None, default: tuple[int, int, int, int]
) -> tuple[int, int, int, int]:
    """Parse color string to RGBA, fall back to default."""
    if color is None:
        return default
    from pml.backend.pillow import parse_color
    result = parse_color(color)
    if result is None:
        return default
    return result


def _is_truthy(val: Any) -> bool:
    """Check if a PML value is truthy."""
    if val is None:
        return False
    if isinstance(val, bool):
        return val
    s = str(val).lower()
    return s not in ("#f", "false", "nil", "0", "")
