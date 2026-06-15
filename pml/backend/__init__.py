"""PML render backend — abstract base class."""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any


class RenderBackend(ABC):
    """Abstract base for rendering backends (Pillow, Cairo, SVG, ...)."""

    @abstractmethod
    def create_surface(self, width: int, height: int, bg_color: str) -> Any:
        """Create a drawing surface."""

    @abstractmethod
    def draw(self, surface: Any, obj: Any) -> None:
        """Draw a GraphicObject (recursively for groups)."""

    @abstractmethod
    def save_image(self, surface: Any, path: str, format: str) -> None:
        """Save surface as a static image file."""

    def save_animation(
        self, frames: list[Any], path: str, format: str, fps: int
    ) -> None:
        """Save frames as an animated file (GIF/MP4). Override in subclass."""
        raise NotImplementedError(
            f"{type(self).__name__} does not support animation output"
        )
