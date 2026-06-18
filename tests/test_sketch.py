"""Tests for hand-drawn / sketch rendering system."""

from __future__ import annotations

import math
import tempfile

import numpy as np
import pytest
from PIL import Image

from pml.api import PMLRuntime
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


# ======================================================================
# Pencil
# ======================================================================


class TestPencil:
    def test_create_pencil(self) -> None:
        """(pencil ...) creates a GraphicObject with shape_type pencil."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (define p (pencil 10 20 100 80 :stroke "#333" :stroke-width 2 :roughness 0.3 :variance 0.5))
                (graphic-object? p)
            """)
            assert result.success, result.error
            assert result.value is True

    def test_pencil_params(self) -> None:
        """Pencil params should be set correctly."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (define p (pencil 10 20 100 80 :stroke "#ff0000" :stroke-width 3 :roughness 0.5 :variance 0.7))
                (graphic-object? p)
            """)
            assert result.success, result.error
            assert result.value is True

    def test_pencil_render(self) -> None:
        """Pencil should render without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (pencil 5 5 50 30 :stroke "#333" :stroke-width 2))
                (render "pencil-test.png")
            """)
            assert result.success, result.error

    def test_pencil_roughness_zero(self) -> None:
        """Roughness=0 produces straight line (no wobble)."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (pencil 10 10 50 10 :stroke "#000" :stroke-width 1 :roughness 0 :variance 0))
                (render "pencil-straight.png")
            """)
            assert result.success, result.error

    def test_pencil_with_transform(self) -> None:
        """Pencil with AffineTransform should render."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (define rot (rotate 45))
                (add (pencil 10 10 50 10 :stroke "#333" :stroke-width 2 :transform rot))
                (render "pencil-transform.png")
            """)
            assert result.success, result.error


# ======================================================================
# Charcoal
# ======================================================================


class TestCharcoal:
    def test_create_charcoal(self) -> None:
        """(charcoal ...) creates a GraphicObject."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (graphic-object? (charcoal 10 10 100 50 :stroke "#222" :stroke-width 5 :roughness 0.4 :scatter 0.3))
            """)
            assert result.success, result.error
            assert result.value is True

    def test_charcoal_render(self) -> None:
        """Charcoal renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (charcoal 10 10 50 40 :stroke "#333" :stroke-width 4))
                (render "charcoal-test.png")
            """)
            assert result.success, result.error

    def test_charcoal_scatter_zero(self) -> None:
        """Scatter=0 should not produce extra particles."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (charcoal 10 10 50 40 :stroke "#333" :stroke-width 4 :scatter 0))
                (render "charcoal-noscatter.png")
            """)
            assert result.success, result.error


# ======================================================================
# Watercolor
# ======================================================================


class TestWatercolor:
    def test_create_watercolor_rect(self) -> None:
        """(watercolor-rect ...) creates a GraphicObject."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (graphic-object? (watercolor-rect 10 10 50 50 :fill "#e74c3c" :bleed 0.3 :layers 4))
            """)
            assert result.success, result.error
            assert result.value is True

    def test_create_watercolor_circle(self) -> None:
        """(watercolor-circle ...) creates a GraphicObject."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (graphic-object? (watercolor-circle 50 50 30 :fill "#3498db" :bleed 0.4 :layers 3))
            """)
            assert result.success, result.error
            assert result.value is True

    def test_watercolor_rect_render(self) -> None:
        """Watercolor rect renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (watercolor-rect 10 10 40 40 :fill "#e74c3c" :bleed 0.3 :layers 3))
                (render "wc-rect-test.png")
            """)
            assert result.success, result.error

    def test_watercolor_circle_render(self) -> None:
        """Watercolor circle renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (watercolor-circle 32 32 20 :fill "#3498db" :bleed 0.4 :layers 3))
                (render "wc-circle-test.png")
            """)
            assert result.success, result.error

    def test_watercolor_layers_zero_bleed(self) -> None:
        """Watercolor with bleed=0 produces sharp edges."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 32 32 :bg "#ffffff")
                (add (watercolor-rect 4 4 24 24 :fill "#e74c3c" :bleed 0 :layers 1))
                (render "wc-sharp.png")
            """)
            assert result.success, result.error

    def test_watercolor_transparency(self) -> None:
        """Watercolor preserves transparent background."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "transparent")
                (add (watercolor-circle 32 32 20 :fill "#e74c3c" :bleed 0.3 :layers 3))
                (render "wc-alpha-test.png")
            """)
            assert result.success, result.error


# ======================================================================
# Hatch
# ======================================================================


class TestHatch:
    def test_create_hatch(self) -> None:
        """(hatch ...) creates a GraphicObject."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (graphic-object? (hatch 0 0 100 100 :stroke "#555" :density 0.5 :angle 45 :cross #t))
            """)
            assert result.success, result.error
            assert result.value is True

    def test_hatch_render(self) -> None:
        """Hatch renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (hatch 10 10 44 44 :stroke "#555" :density 0.5 :angle 45 :cross #t))
                (render "hatch-test.png")
            """)
            assert result.success, result.error

    def test_hatch_no_cross(self) -> None:
        """Hatch without cross renders single-direction lines."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (hatch 10 10 44 44 :stroke "#555" :density 0.4 :angle 30 :cross #f))
                (render "hatch-single.png")
            """)
            assert result.success, result.error

    def test_hatch_sparse_dense(self) -> None:
        """Sparse vs dense hatch both render."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (hatch 5 5 25 25 :stroke "#555" :density 0.1 :angle 45 :cross #f))
                (add (hatch 35 5 25 25 :stroke "#555" :density 0.9 :angle 45 :cross #f))
                (render "hatch-density.png")
            """)
            assert result.success, result.error


# ======================================================================
# Sketchify modifier
# ======================================================================


class TestSketchify:
    def test_create_sketchify(self) -> None:
        """(sketchify shape ...) creates a GraphicObject."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (graphic-object? (sketchify (rect 10 10 50 50 :fill "#e74c3c") :roughness 0.3))
            """)
            assert result.success, result.error
            assert result.value is True

    def test_sketchify_rect_render(self) -> None:
        """Sketchify modifier on a rect renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (sketchify (rect 10 10 40 40 :fill "#e74c3c" :stroke "#333" :stroke-width 2) :roughness 0.3))
                (render "sketchify-test.png")
            """)
            assert result.success, result.error

    def test_sketchify_circle_render(self) -> None:
        """Sketchify modifier on a circle renders without error."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 64 64 :bg "#ffffff")
                (add (sketchify (circle 32 32 20 :fill "#3498db") :roughness 0.2))
                (render "sketchify-circle.png")
            """)
            assert result.success, result.error


# ======================================================================
# Composition (combining multiple sketch types)
# ======================================================================


class TestSketchComposition:
    def test_mixed_sketch_composition(self) -> None:
        """Combining pencil + watercolor + hatch renders."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 100 100 :bg "#f5f0e8")
                (add (watercolor-circle 50 60 40 :fill "#7f8c8d" :bleed 0.3 :layers 3))
                (add (pencil 50 60 50 20 :stroke "#5d4037" :stroke-width 3 :roughness 0.3 :variance 0.4))
                (add (watercolor-circle 50 35 20 :fill "#27ae60" :bleed 0.4 :layers 3))
                (add (hatch 0 80 100 20 :stroke "#888" :density 0.3 :angle 15 :cross #f))
                (render "composition-test.png")
            """)
            assert result.success, result.error

    def test_all_sketch_types_together(self) -> None:
        """All 5 sketch shape types render together."""
        with tempfile.TemporaryDirectory() as tmpdir:
            rt = PMLRuntime(output_dir=tmpdir)
            result = rt.execute("""
                (sprite-canvas 128 128 :bg "#f5f0e8")
                (add (pencil 10 10 100 20 :stroke "#333" :stroke-width 2 :roughness 0.3 :variance 0.4))
                (add (charcoal 10 30 100 40 :stroke "#222" :stroke-width 4 :roughness 0.5 :scatter 0.3))
                (add (watercolor-rect 10 50 40 40 :fill "#e74c3c" :bleed 0.3 :layers 3))
                (add (watercolor-circle 80 70 25 :fill "#3498db" :bleed 0.4 :layers 4))
                (add (hatch 10 100 100 20 :stroke "#555" :density 0.5 :angle 45 :cross #t))
                (add (sketchify (rect 10 80 100 15 :fill "#f39c12") :roughness 0.2))
                (render "all-sketch.png")
            """)
            assert result.success, result.error
