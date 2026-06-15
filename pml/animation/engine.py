"""PML animation engine — Animation class hierarchy.

Provides PropertyAnimation (single property tween), ParallelAnimation
(multiple simultaneous), and SequenceAnimation (chained playback).
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any, Callable

from pml.animation.easing import get_easing
from pml.animation.interpolate import lerp_value


class Animation(ABC):
    """Abstract base for all animation types."""

    @abstractmethod
    def get_duration(self) -> float:
        """Total duration in seconds."""

    @abstractmethod
    def get_value_at(
        self, t: float
    ) -> list[tuple[int, str, Any]]:
        """Evaluate all property animations at time t.

        Returns a list of (target_id, property, interpolated_value) tuples.
        """


class PropertyAnimation(Animation):
    """Animate a single property of a GraphicObject over time.

    Attributes:
        target_id: id() of the target GraphicObject.
        property: Property path — 'x', 'y', 'fill', 'transform.tx', etc.
        from_value: Start value.
        to_value: End value.
        _duration: Duration in seconds.
        ease_fn: Easing function (t → t).
        delay: Delay before animation starts (seconds).
        repeat: Number of repetitions (int or 'infinite').
    """

    def __init__(
        self,
        target_id: int,
        property: str,
        from_value: Any,
        to_value: Any,
        duration: float,
        ease_fn: Callable[[float], float] | None = None,
        delay: float = 0.0,
        repeat: int | str = 1,
    ) -> None:
        self.target_id = target_id
        self.property = property
        self.from_value = from_value
        self.to_value = to_value
        self._duration = max(0.001, float(duration))
        self.ease_fn = ease_fn or get_easing("linear")
        self.delay = float(delay)
        self.repeat = repeat

    def get_duration(self) -> float:
        """Total duration including delay and repeats."""
        if self.repeat == "infinite" or self.repeat == float("inf"):
            return float("inf")
        n = int(self.repeat) if self.repeat else 1
        return self.delay + self._duration * n

    def get_value_at(self, t: float) -> list[tuple[int, str, Any]]:
        """Compute the interpolated property value at time t."""
        # Before delay — return from_value
        if t < self.delay:
            return [(self.target_id, self.property, self.from_value)]

        elapsed = t - self.delay

        # Handle repeat
        if self.repeat == "infinite" or self.repeat == float("inf"):
            # Infinite loop: wrap within duration
            local_t = elapsed % self._duration
        else:
            n = max(1, int(self.repeat))
            total_play = self._duration * n
            if elapsed >= total_play:
                # Past end — return to_value
                return [(self.target_id, self.property, self.to_value)]
            local_t = elapsed % self._duration

        # Normalized time within one cycle
        norm = local_t / self._duration
        # Clamp to [0, 1]
        norm = max(0.0, min(1.0, norm))

        # Apply easing
        eased = self.ease_fn(norm)

        # Interpolate
        value = lerp_value(self.from_value, self.to_value, eased)

        return [(self.target_id, self.property, value)]


class ParallelAnimation(Animation):
    """Multiple animations playing simultaneously.

    Total duration equals the longest child animation.
    """

    def __init__(self, children: list[Animation]) -> None:
        self.children = children

    def get_duration(self) -> float:
        if not self.children:
            return 0.0
        durations = [c.get_duration() for c in self.children]
        return max(durations)

    def get_value_at(self, t: float) -> list[tuple[int, str, Any]]:
        result: list[tuple[int, str, Any]] = []
        for child in self.children:
            result.extend(child.get_value_at(t))
        return result


class SequenceAnimation(Animation):
    """Animations playing one after another.

    Each child starts when the previous one finishes.
    Total duration equals the sum of all child durations.
    """

    def __init__(self, children: list[Animation]) -> None:
        self.children = children
        # Precompute offsets
        self._offsets: list[float] = []
        offset = 0.0
        for child in children:
            self._offsets.append(offset)
            offset += child.get_duration()

    def get_duration(self) -> float:
        return sum(c.get_duration() for c in self.children)

    def get_value_at(self, t: float) -> list[tuple[int, str, Any]]:
        result: list[tuple[int, str, Any]] = []
        for i, child in enumerate(self.children):
            child_dur = child.get_duration()
            offset = self._offsets[i]
            # Adjust time relative to this child's start
            local_t = t - offset
            if local_t >= 0:
                result.extend(child.get_value_at(local_t))
        return result
