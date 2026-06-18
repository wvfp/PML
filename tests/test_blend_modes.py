"""Tests for blend mode compositing in the Pillow backend."""

from __future__ import annotations

import numpy as np
from PIL import Image

from pml.backend.pillow import (
    PillowBackend,
    _BLEND_MODES,
    composite_with_blend,
)
from pml.graphics.objects import GraphicObject


def _norm(img: Image.Image) -> np.ndarray:
    return np.array(img, dtype=np.float32) / 255.0


def test_blend_modes_registered() -> None:
    """All expected blend modes are registered."""
    expected = {
        "normal", "multiply", "screen", "overlay",
        "darken", "lighten", "color-dodge", "color-burn",
        "soft-light", "hard-light", "difference", "exclusion",
    }
    assert expected.issubset(_BLEND_MODES.keys())


def test_multiply_white_is_transparent() -> None:
    """multiply with white src passes dst through unchanged."""
    dst = Image.new("RGBA", (10, 10), (200, 100, 50, 255))
    src = Image.new("RGBA", (10, 10), (255, 255, 255, 255))
    composite_with_blend(dst, src, "multiply")
    # Every pixel should still be (200, 100, 50)
    arr = np.array(dst)
    assert (arr[:, :, 0] == 200).all()
    assert (arr[:, :, 1] == 100).all()
    assert (arr[:, :, 2] == 50).all()


def test_multiply_black_is_black() -> None:
    """multiply with black src → black."""
    dst = Image.new("RGBA", (10, 10), (200, 100, 50, 255))
    src = Image.new("RGBA", (10, 10), (0, 0, 0, 255))
    composite_with_blend(dst, src, "multiply")
    arr = np.array(dst)
    assert (arr[:, :, :3] == 0).all()


def test_screen_black_is_transparent() -> None:
    """screen with black src passes dst through."""
    dst = Image.new("RGBA", (10, 10), (200, 100, 50, 255))
    src = Image.new("RGBA", (10, 10), (0, 0, 0, 255))
    composite_with_blend(dst, src, "screen")
    arr = np.array(dst)
    assert (arr[:, :, 0] == 200).all()
    assert (arr[:, :, 1] == 100).all()
    assert (arr[:, :, 2] == 50).all()


def test_screen_white_is_white() -> None:
    """screen with white src → white."""
    dst = Image.new("RGBA", (10, 10), (200, 100, 50, 255))
    src = Image.new("RGBA", (10, 10), (255, 255, 255, 255))
    composite_with_blend(dst, src, "screen")
    arr = np.array(dst)
    assert (arr[:, :, :3] == 255).all()


def test_difference_identical_is_black() -> None:
    """difference of identical pixels → black."""
    dst = Image.new("RGBA", (10, 10), (120, 80, 200, 255))
    src = Image.new("RGBA", (10, 10), (120, 80, 200, 255))
    composite_with_blend(dst, src, "difference")
    arr = np.array(dst)
    assert (arr[:, :, :3] == 0).all()


def test_difference_complement() -> None:
    """difference of (255,0,0) and (0,255,0) → (255,255,0)."""
    dst = Image.new("RGBA", (10, 10), (255, 0, 0, 255))
    src = Image.new("RGBA", (10, 10), (0, 255, 0, 255))
    composite_with_blend(dst, src, "difference")
    arr = np.array(dst)
    assert (arr[:, :, 0] == 255).all()
    assert (arr[:, :, 1] == 255).all()
    assert (arr[:, :, 2] == 0).all()


def test_darken_min() -> None:
    """darken picks min per channel."""
    dst = Image.new("RGBA", (10, 10), (100, 200, 50, 255))
    src = Image.new("RGBA", (10, 10), (200, 100, 150, 255))
    composite_with_blend(dst, src, "darken")
    arr = np.array(dst)
    assert (arr[:, :, 0] == 100).all()
    assert (arr[:, :, 1] == 100).all()
    assert (arr[:, :, 2] == 50).all()


def test_lighten_max() -> None:
    """lighten picks max per channel."""
    dst = Image.new("RGBA", (10, 10), (100, 200, 50, 255))
    src = Image.new("RGBA", (10, 10), (200, 100, 150, 255))
    composite_with_blend(dst, src, "lighten")
    arr = np.array(dst)
    assert (arr[:, :, 0] == 200).all()
    assert (arr[:, :, 1] == 200).all()
    assert (arr[:, :, 2] == 150).all()


def test_overlay_midpoint() -> None:
    """overlay where dst=0.5 → src (neutral)."""
    dst = Image.new("RGBA", (10, 10), (128, 128, 128, 255))
    src = Image.new("RGBA", (10, 10), (64, 192, 0, 255))
    composite_with_blend(dst, src, "overlay")
    arr = np.array(dst)
    # When dst ≈ 0.5, overlay ≈ src
    assert abs(int(arr[0, 0, 0]) - 64) <= 1
    assert abs(int(arr[0, 0, 1]) - 192) <= 1


def test_normal_path_uses_pillow_paste() -> None:
    """normal blend mode uses Pillow's native paste (fast path)."""
    dst = Image.new("RGBA", (20, 20), (255, 0, 0, 255))
    src = Image.new("RGBA", (10, 10), (0, 255, 0, 128))
    composite_with_blend(dst, src, "normal", (5, 5))
    arr = np.array(dst)
    # Center pixel should be green-tinted (alpha blend)
    assert arr[10, 10, 0] < 255
    assert arr[10, 10, 1] > 0


def test_blend_with_partial_overlap() -> None:
    """blend with src smaller than dst and offset works correctly."""
    dst = Image.new("RGBA", (50, 50), (100, 100, 100, 255))
    src = Image.new("RGBA", (20, 20), (200, 200, 200, 255))
    composite_with_blend(dst, src, "screen", (40, 40))
    arr = np.array(dst)
    # Top-left corner (uncovered) unchanged
    assert (arr[0, 0, :3] == [100, 100, 100]).all()
    # Bottom-right corner (covered) is screened → should be lighter
    assert arr[49, 49, 0] > 100


def test_no_op_out_of_bounds() -> None:
    """src completely outside dst bounds → no-op."""
    dst = Image.new("RGBA", (10, 10), (100, 100, 100, 255))
    src = Image.new("RGBA", (5, 5), (255, 255, 255, 255))
    composite_with_blend(dst, src, "multiply", (20, 20))
    arr = np.array(dst)
    assert (arr[:, :, :3] == [100, 100, 100]).all()


def test_alpha_preserved_in_blend() -> None:
    """Alpha channel is preserved during overlay blend."""
    dst = Image.new("RGBA", (20, 20), (100, 100, 100, 200))
    src = Image.new("RGBA", (20, 20), (200, 200, 200, 100))
    composite_with_blend(dst, src, "overlay")
    arr = np.array(dst)
    # Alpha should remain > 0 (source-over compositing)
    assert arr[0, 0, 3] > 0


def test_exclusion_midgray() -> None:
    """exclusion with src=dst=midgray → midgray (0.5)."""
    dst = Image.new("RGBA", (10, 10), (128, 128, 128, 255))
    src = Image.new("RGBA", (10, 10), (128, 128, 128, 255))
    composite_with_blend(dst, src, "exclusion")
    arr = np.array(dst)
    # exclusion(x, x) = x + x - 2*x*x = 2x - 2x² = 2x(1-x)
    # For x=0.502 → ≈ 0.5, so 128 stays ~128
    assert abs(int(arr[0, 0, 0]) - 128) <= 2


def test_color_burn_src_dark() -> None:
    """color-burn with dark src darkens dst."""
    dst = Image.new("RGBA", (10, 10), (200, 200, 200, 255))
    src = Image.new("RGBA", (10, 10), (64, 64, 64, 255))
    composite_with_blend(dst, src, "color-burn")
    arr = np.array(dst)
    # Should be darker than original
    assert arr[0, 0, 0] < 200
    assert arr[0, 0, 1] < 200
    assert arr[0, 0, 2] < 200


def test_pillow_backend_draw_blend_mode() -> None:
    """PillowBackend.draw() dispatches blend mode correctly."""
    backend = PillowBackend()
    surface = Image.new("RGBA", (30, 30), (100, 100, 100, 255))

    # Draw a red rect with multiply blend on top of gray
    rect = GraphicObject(
        shape_type="rect",
        params={"x": 5, "y": 5, "w": 20, "h": 20},
        fill="#ffffff",
        blend_mode="multiply",
    )
    backend.draw(surface, rect)
    arr = np.array(surface)
    # multiply(white, gray) = gray → area stays gray
    assert arr[10, 10, 0] == 100

    # Draw black rect with multiply blend
    rect_black = GraphicObject(
        shape_type="rect",
        params={"x": 5, "y": 5, "w": 20, "h": 20},
        fill="#000000",
        blend_mode="multiply",
    )
    backend.draw(surface, rect_black)
    arr = np.array(surface)
    # multiply(black, gray) = black
    assert arr[10, 10, 0] == 0
    assert arr[10, 10, 1] == 0
    assert arr[10, 10, 2] == 0


def test_pillow_backend_normal_blend_no_temp() -> None:
    """Normal blend mode draws directly without temp surface."""
    backend = PillowBackend()
    surface = Image.new("RGBA", (20, 20), (100, 100, 100, 255))
    rect = GraphicObject(
        shape_type="rect",
        params={"x": 5, "y": 5, "w": 10, "h": 10},
        fill="#ff0000",
        blend_mode="normal",
    )
    backend.draw(surface, rect)
    arr = np.array(surface)
    assert int(arr[10, 10, 0]) == 255
    assert int(arr[10, 10, 1]) == 0
    assert int(arr[10, 10, 2]) == 0
