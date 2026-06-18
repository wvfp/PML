"""Tests for shader system: registry, post-process pipeline, PML builtins."""

from __future__ import annotations

import tempfile

import numpy as np
import pytest
from PIL import Image

from pml.api import PMLRuntime
from pml.graphics.canvas import _current_canvas, get_current_canvas
from pml.shaders import (
    Shader,
    apply_post_process_chain,
    get_shader,
    list_shader_names,
    pixel_shader_from_lambda,
    register_shader,
    register_shaders,
)
from pml.shaders.builtins import register_builtin_shaders


# ======================================================================
# Shader registration
# ======================================================================


class TestShaderRegistry:
    def setup_method(self) -> None:
        # Fresh slate by reloading builtins
        # (Registry is global, so we just verify idempotency)
        register_builtin_shaders()

    def test_registered_count(self) -> None:
        """All 18 built-in shaders should be registered."""
        names = list_shader_names()
        assert len(names) == 18

    def test_all_names(self) -> None:
        """Known shader names should be present."""
        expected = {
            "bloom", "blur", "brightness", "color-grade", "contour",
            "contrast", "crt-scanline", "edge-detect", "emboss",
            "grayscale", "invert", "noise", "oil-paint", "pixelate",
            "sepia", "sharpen", "smooth", "vignette",
        }
        registered = set(list_shader_names())
        assert registered == expected

    def test_get_shader_returns_none_for_unknown(self) -> None:
        assert get_shader("nonexistent") is None

    def test_get_shader_returns_shader(self) -> None:
        s = get_shader("sepia")
        assert s is not None
        assert s.name == "sepia"
        assert s.shader_type == "post-process"
        assert callable(s.impl)

    def test_shader_params(self) -> None:
        """Shaders with params should expose them."""
        blur = get_shader("blur")
        assert blur is not None
        assert len(blur.params) == 1
        assert blur.params[0]["name"] == "radius"
        assert blur.params[0]["type"] == "float"

        bloom = get_shader("bloom")
        assert bloom is not None
        assert len(bloom.params) == 2

    def test_custom_shader_registration(self) -> None:
        """Can register a custom shader and retrieve it."""
        fn = lambda img, **kw: img  # noqa: E731
        shader = Shader("test-custom", fn, "post-process", "Test shader")
        register_shader(shader)
        retrieved = get_shader("test-custom")
        assert retrieved is not None
        assert retrieved.name == "test-custom"
        assert retrieved.impl is fn

    def test_register_overwrites(self) -> None:
        """Registering same name overwrites previous."""
        fn1 = lambda img, **kw: img  # noqa: E731
        fn2 = lambda img, **kw: img  # noqa: E731
        register_shader(Shader("overwrite-test", fn1))
        register_shader(Shader("overwrite-test", fn2))
        s = get_shader("overwrite-test")
        assert s is not None
        assert s.impl is fn2


# ======================================================================
# Post-process chain
# ======================================================================


class TestPostProcessChain:
    def test_empty_chain_returns_same_image(self) -> None:
        img = Image.new("RGBA", (10, 10), (255, 0, 0, 255))
        result = apply_post_process_chain(img, [])
        assert result is img  # no copy for empty chain

    def test_chain_runs_shader(self) -> None:
        """A grayscale shader should reduce saturation."""
        img = Image.new("RGBA", (10, 10), (255, 0, 0, 255))
        chain = [{"name": "grayscale", "kwargs": {}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        # All channels should be roughly equal (gray)
        assert abs(int(arr[0, 0, 0]) - int(arr[0, 0, 1])) < 5
        assert abs(int(arr[0, 0, 0]) - int(arr[0, 0, 2])) < 5

    def test_chain_multiple_effects(self) -> None:
        """Multiple shaders chain in order."""
        img = Image.new("RGBA", (10, 10), (128, 200, 50, 255))
        chain = [
            {"name": "invert", "kwargs": {}},
            {"name": "grayscale", "kwargs": {}},
        ]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        # After invert: (127, 55, 205) → after grayscale: gray
        r, g, b = int(arr[0, 0, 0]), int(arr[0, 0, 1]), int(arr[0, 0, 2])
        assert abs(r - g) < 5
        assert abs(r - b) < 5

    def test_chain_with_params(self) -> None:
        """Shaders with kwargs should apply correctly."""
        img = Image.new("RGBA", (20, 20), (100, 100, 100, 255))
        chain = [{"name": "brightness", "kwargs": {"factor": 2.0}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        # Brightness 2x → 200
        assert int(arr[0, 0, 0]) == 200

    def test_unknown_shader_skipped(self) -> None:
        """Unknown shader in chain should be silently skipped."""
        img = Image.new("RGBA", (10, 10), (255, 0, 0, 255))
        chain = [{"name": "nope-nonexistent", "kwargs": {}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        assert int(arr[0, 0, 0]) == 255

    def test_blur_changes_pixels(self) -> None:
        """Gaussian blur should blend neighboring pixels."""
        # Create sharp edge
        img = Image.new("RGBA", (20, 20), (255, 255, 255, 255))
        arr = np.array(img)
        arr[10:, :, :] = (0, 0, 0, 255)
        img = Image.fromarray(arr, "RGBA")

        chain = [{"name": "blur", "kwargs": {"radius": 3}}]
        result = apply_post_process_chain(img, chain)
        result_arr = np.array(result)
        # Edge pixel should be intermediate (blended)
        edge_pixel = result_arr[10, 5]
        assert 50 < int(edge_pixel[0]) < 200

    def test_pixelate_downscales(self) -> None:
        """Pixelate should reduce effective resolution."""
        img = Image.new("RGBA", (32, 32), (255, 0, 0, 255))
        chain = [{"name": "pixelate", "kwargs": {"size": 8}}]
        result = apply_post_process_chain(img, chain)
        assert result.size == (32, 32)
        # All pixels in an 8x8 block should match
        arr = np.array(result)
        assert np.array_equal(arr[0, 0], arr[7, 7])

    def test_vignette_darkens_corners(self) -> None:
        """Vignette should make corners darker than center."""
        img = Image.new("RGBA", (50, 50), (200, 200, 200, 255))
        chain = [{"name": "vignette", "kwargs": {"strength": 0.8}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        center_brightness = int(arr[25, 25, 0])
        corner_brightness = int(arr[0, 0, 0])
        assert corner_brightness < center_brightness

    def test_bloom_creates_glow(self) -> None:
        """Bloom should brighten bright areas."""
        # Create image with one bright spot
        img = Image.new("RGBA", (30, 30), (50, 50, 50, 255))
        arr = np.array(img)
        arr[14:16, 14:16] = (255, 255, 255, 255)
        img = Image.fromarray(arr, "RGBA")

        chain = [{"name": "bloom", "kwargs": {"radius": 3, "strength": 0.5}}]
        result = apply_post_process_chain(img, chain)
        result_arr = np.array(result)
        # Pixels near the bright spot should be brighter than before
        assert int(result_arr[12, 12, 0]) > 50

    def test_oil_paint_preserves_size(self) -> None:
        """Oil paint effect should not change dimensions."""
        img = Image.new("RGBA", (20, 20), (100, 150, 200, 255))
        chain = [{"name": "oil-paint", "kwargs": {"range": 2, "levels": 4}}]
        result = apply_post_process_chain(img, chain)
        assert result.size == (20, 20)

    def test_crt_scanline_darkens_rows(self) -> None:
        """CRT scanline should darken every other row."""
        img = Image.new("RGBA", (10, 10), (200, 200, 200, 255))
        chain = [{"name": "crt-scanline", "kwargs": {"strength": 0.5}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(result)
        # Row 0 should be darker than row 1
        assert int(arr[0, 0, 0]) < int(arr[1, 0, 0])

    def test_noise_changes_pixels(self) -> None:
        """Noise should add grain (pixels differ from input)."""
        img = Image.new("RGBA", (10, 10), (128, 128, 128, 255))
        chain = [{"name": "noise", "kwargs": {"amount": 0.5}}]
        result = apply_post_process_chain(img, chain)
        arr = np.array(img)
        result_arr = np.array(result)
        # At least some pixels should differ
        assert not np.array_equal(arr[:, :, :3], result_arr[:, :, :3])


# ======================================================================
# Pixel shader
# ======================================================================


class TestPixelShader:
    def test_pixel_shader_rgba_invert(self) -> None:
        """Pixel shader that inverts colors."""
        img = Image.new("RGBA", (5, 5), (100, 150, 200, 255))

        def invert_fn(r: int, g: int, b: int, a: int) -> tuple[int, int, int, int]:
            return (255 - r, 255 - g, 255 - b, a)

        result = pixel_shader_from_lambda(invert_fn, img)
        arr = np.array(result)
        assert int(arr[0, 0, 0]) == 155
        assert int(arr[0, 0, 1]) == 105
        assert int(arr[0, 0, 2]) == 55
        assert int(arr[0, 0, 3]) == 255

    def test_pixel_shader_preserves_alpha(self) -> None:
        """Alpha channel should be preserved by default passthrough."""
        img = Image.new("RGBA", (5, 5), (255, 0, 0, 128))

        def passthrough(r: int, g: int, b: int, a: int) -> tuple[int, int, int, int]:
            return (r, g, b, a)

        result = pixel_shader_from_lambda(passthrough, img)
        arr = np.array(result)
        assert int(arr[0, 0, 3]) == 128

    def test_pixel_shader_clamps_values(self) -> None:
        """Out-of-range values should be clamped to 0-255."""
        img = Image.new("RGBA", (3, 3), (100, 100, 100, 255))

        def overflow_fn(r: int, g: int, b: int, a: int) -> tuple[int, int, int, int]:
            return (300, -50, 9999, a)

        result = pixel_shader_from_lambda(overflow_fn, img)
        arr = np.array(result)
        assert int(arr[0, 0, 0]) == 255
        assert int(arr[0, 0, 1]) == 0
        assert int(arr[0, 0, 2]) == 255


# ======================================================================
# PML builtins integration
# ======================================================================


class TestPMLShaders:
    def test_list_shaders_builtin(self) -> None:
        """(list-shaders) should return meta for all shaders."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (print (list-shaders))
            """)
            assert result.success, result.error

    def test_post_process_basic(self) -> None:
        """(post-process 'sepia) adds shader to canvas chain."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "white")
                (add (rect 0 0 40 30 :fill "red"))
                (post-process 'sepia)
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert len(canvas._post_process_chain) == 1
            assert canvas._post_process_chain[0]["name"] == "sepia"

    def test_post_process_with_kwargs(self) -> None:
        """(post-process 'blur :radius 5) passes kwargs."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "white")
                (add (rect 0 0 40 30 :fill "red"))
                (post-process 'blur :radius 5)
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert canvas._post_process_chain[0]["name"] == "blur"
            assert canvas._post_process_chain[0]["kwargs"]["radius"] == 5

    def test_multiple_post_process_chain(self) -> None:
        """Multiple (post-process ...) calls chain in order."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "white")
                (add (rect 0 0 40 30 :fill "red"))
                (post-process 'grayscale)
                (post-process 'blur :radius 2)
            """)
            assert result.success, result.error
            canvas = get_current_canvas()
            assert canvas is not None
            assert len(canvas._post_process_chain) == 2
            assert canvas._post_process_chain[0]["name"] == "grayscale"
            assert canvas._post_process_chain[1]["name"] == "blur"

    def test_render_with_post_process(self) -> None:
        """(render ...) with (post-process ...) works end-to-end."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "white")
                (add (rect 0 0 40 30 :fill "red"))
                (post-process 'sepia)
                (render "shader-test.png")
            """)
            assert result.success, result.error
            assert "shader-test" in str(result.value)

    def test_render_with_multiple_effects(self) -> None:
        """(render ...) with chained effects works end-to-end."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "white")
                (add (rect 0 0 40 30 :fill "red"))
                (post-process 'pixelate :size 4)
                (post-process 'vignette :strength 0.3)
                (render "chain-test.png")
            """)
            assert result.success, result.error
            assert "chain-test" in str(result.value)

    def test_post_process_without_canvas_errors(self) -> None:
        """(post-process ...) without active canvas should raise."""
        from pml.graphics.canvas import _current_canvas
        _current_canvas[0] = None  # Reset for this test
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (post-process 'sepia)
            """)
            assert not result.success

    def test_post_process_unknown_shader_errors(self) -> None:
        """(post-process 'nonexistent) should raise."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64)
                (post-process 'totally-not-real)
            """)
            assert not result.success

    def test_shader_wrap_object(self) -> None:
        """(shader obj 'name) attaches metadata to graphic object."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 0 0 :bg "transparent")
                (define glow-box (shader (rect 10 10 30 30 :fill "red") 'bloom :radius 3))
                (add glow-box)
                (render "shader-obj-test.png")
            """)
            assert result.success, result.error

    def test_define_shader(self) -> None:
        """(define-shader 'name) registers a placeholder shader."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (define-shader 'my-shader)
            """)
            assert result.success, result.error
            s = get_shader("my-shader")
            assert s is not None
            assert s.name == "my-shader"


# ======================================================================
# Per-shader effect tests (actual pixel verification)
# ======================================================================


class TestShaderEffects:
    """Per-shader pixel-level tests using direct Python API (no PML rendering)."""

    def _apply_shader(self, shader_name: str, img: Image.Image, **kwargs: Any) -> Image.Image:
        """Apply a named shader via the post-process pipeline."""
        from pml.shaders import apply_post_process_chain
        return apply_post_process_chain(img, [{"name": shader_name, "kwargs": kwargs}])

    def test_sepia_produces_warm_tone(self) -> None:
        """Sepia should shift colors toward warm brown."""
        img = Image.new("RGBA", (16, 16), (255, 102, 0, 255))  # #ff6600
        result = self._apply_shader("sepia", img)
        arr = np.array(result)
        r, g, b = int(arr[4, 4, 0]), int(arr[4, 4, 1]), int(arr[4, 4, 2])
        # Expected: ~179, ~159, ~124 — red dominates, g and b close
        assert r >= b  # Warm tone: red >= blue
        assert abs(g - b) <= 40  # Green and blue are similar

    def test_invert_swaps_colors(self) -> None:
        """Invert should flip all color channels."""
        img = Image.new("RGBA", (16, 16), (255, 102, 0, 255))  # #ff6600
        result = self._apply_shader("invert", img)
        arr = np.array(result)
        r, g, b = int(arr[4, 4, 0]), int(arr[4, 4, 1]), int(arr[4, 4, 2])
        # #ff6600 → #0099ff
        assert r < 30
        assert 140 < g < 170
        assert b > 200

    def test_blur_smooths_edges(self) -> None:
        """Blur should reduce sharp transitions."""
        img = Image.new("RGBA", (16, 16), (0, 0, 0, 255))
        arr = np.array(img)
        arr[:, 8:, :] = (255, 255, 255, 255)  # sharp white/black edge
        img = Image.fromarray(arr, "RGBA")
        result = self._apply_shader("blur", img, radius=3)
        arr_r = np.array(result)
        edge_pixel = int(arr_r[8, 8, 0])
        assert 30 < edge_pixel < 220

    def test_brightness_doubles(self) -> None:
        """Brightness 2.0 should double pixel values."""
        img = Image.new("RGBA", (16, 16), (68, 68, 68, 255))  # ~0x44
        result = self._apply_shader("brightness", img, factor=2.0)
        arr = np.array(result)
        assert int(arr[4, 4, 0]) >= 120  # 68*2 ≈ 136

    def test_edge_detect_finds_edges(self) -> None:
        """Edge detect should highlight transitions."""
        img = Image.new("RGBA", (16, 16), (0, 0, 0, 255))
        arr = np.array(img)
        arr[2:14, 2:14] = (255, 255, 255, 255)
        img = Image.fromarray(arr, "RGBA")
        result = self._apply_shader("edge-detect", img)
        arr_r = np.array(result)
        center = int(arr_r[8, 8, 0])
        edge_area = int(arr_r[2, 2, 0])
        assert center < 50 or edge_area > center

    def test_contour_preserves_size(self) -> None:
        """Contour should produce thin outlines without changing size."""
        img = Image.new("RGBA", (16, 16), (255, 255, 255, 255))
        result = self._apply_shader("contour", img)
        assert result.size == (16, 16)
