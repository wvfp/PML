"""PML Graphic primitives — GraphicObject data model."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from pml.transform import AffineTransform


@dataclass(frozen=True)
class GraphicObject:
    """An immutable graphic element in the PML scene graph.

    All primitives (circle, rect, etc.) produce GraphicObjects.
    Modification returns new objects (supports animation interpolation).
    """

    shape_type: str
    params: dict[str, Any] = field(default_factory=dict)
    fill: str | None = None
    stroke: str | None = None
    stroke_width: float = 1.0
    opacity: float = 1.0
    """Per-object opacity 0.0–1.0. Applied as alpha multiplier at render time
    by PillowBackend compositing. Stacked with hex alpha in fill/stroke colors."""
    blend_mode: str = "normal"
    """Compositing blend mode: normal, multiply, screen, overlay, darken,
    lighten, color-dodge, color-burn, soft-light, hard-light, difference,
    exclusion."""
    transform: AffineTransform = field(default_factory=AffineTransform.identity)
    children: tuple[GraphicObject, ...] = ()
    metadata: dict[str, Any] = field(default_factory=dict)

    def with_opacity(self, value: float) -> GraphicObject:
        """Return a copy with a new opacity."""
        return GraphicObject(
            shape_type=self.shape_type,
            params=dict(self.params),
            fill=self.fill,
            stroke=self.stroke,
            stroke_width=self.stroke_width,
            opacity=value,
            blend_mode=self.blend_mode,
            transform=self.transform,
            children=self.children,
            metadata=dict(self.metadata),
        )

    def with_transform(self, t: AffineTransform) -> GraphicObject:
        """Return a copy with a new transform applied."""
        return GraphicObject(
            shape_type=self.shape_type,
            params=dict(self.params),
            fill=self.fill,
            stroke=self.stroke,
            stroke_width=self.stroke_width,
            opacity=self.opacity,
            blend_mode=self.blend_mode,
            transform=t,
            children=self.children,
            metadata=dict(self.metadata),
        )

    def with_fill(self, color: str) -> GraphicObject:
        return GraphicObject(
            shape_type=self.shape_type,
            params=dict(self.params),
            fill=color,
            stroke=self.stroke,
            stroke_width=self.stroke_width,
            opacity=self.opacity,
            blend_mode=self.blend_mode,
            transform=self.transform,
            children=self.children,
            metadata=dict(self.metadata),
        )

    def with_stroke(self, color: str) -> GraphicObject:
        """Return a copy with updated stroke color."""
        return GraphicObject(
            shape_type=self.shape_type,
            params=dict(self.params),
            fill=self.fill,
            stroke=color,
            stroke_width=self.stroke_width,
            opacity=self.opacity,
            blend_mode=self.blend_mode,
            transform=self.transform,
            children=self.children,
            metadata=dict(self.metadata),
        )

    def with_param(self, key: str, value: Any) -> GraphicObject:
        """Return a copy with one param updated."""
        new_params = dict(self.params)
        new_params[key] = value
        return GraphicObject(
            shape_type=self.shape_type,
            params=new_params,
            fill=self.fill,
            stroke=self.stroke,
            stroke_width=self.stroke_width,
            opacity=self.opacity,
            blend_mode=self.blend_mode,
            transform=self.transform,
            children=self.children,
            metadata=dict(self.metadata),
        )

    def __repr__(self) -> str:
        parts = [f"<GraphicObject {self.shape_type}"]
        if self.fill:
            parts.append(f" fill={self.fill}")
        if self.stroke:
            parts.append(f" stroke={self.stroke}")
        if self.opacity < 1.0:
            parts.append(f" opacity={self.opacity}")
        if self.children:
            parts.append(f" children={len(self.children)}")
        parts.append(">")
        return "".join(parts)
