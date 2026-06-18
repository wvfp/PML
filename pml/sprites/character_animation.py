"""Character animation presets — blink, idle breath, walk cycle.

Each preset creates Animation tracks registered on the global Timeline.
They can be used from Python or from PML via registered builtins.

Usage from PML::

    (sprite-canvas 128 256 :bg "transparent")
    (define hero (character :expression "neutral" :style 'cel))
    (add hero)
    (blink hero)
    (idle-breath hero)
    (play)
    (render "hero.gif" :fps 30)
"""

from __future__ import annotations

import math
from typing import Any

from pml.animation.easing import get_easing
from pml.animation.engine import PropertyAnimation
from pml.animation.timeline import Timeline
from pml.graphics.objects import GraphicObject
from pml.transform import AffineTransform


def _find_component(root: GraphicObject, component: str) -> GraphicObject | None:
    """Walk a GraphicObject tree to find a child by ``component`` metadata."""
    if root.metadata.get("component") == component:
        return root
    for child in (root.children or ()):
        result = _find_component(child, component)
        if result is not None:
            return result
    return None


def _find_all_children(root: GraphicObject, component: str) -> list[GraphicObject]:
    """Find all GraphicObjects matching *component* via depth-first search."""
    results: list[GraphicObject] = []
    if root.metadata.get("component") == component:
        results.append(root)
    for child in (root.children or ()):
        results.extend(_find_all_children(child, component))
    return results


# ======================================================================
# Blink — close and open eyes
# ======================================================================


def blink(
    char: GraphicObject,
    interval: float = 3.0,
    close_duration: float = 0.08,
    hold_duration: float = 0.08,
    open_duration: float = 0.12,
) -> list[PropertyAnimation]:
    """Register a blink animation on the eyes component.

    Cycles: eyes open → close over *close_duration* → hold → open over
    *open_duration* → wait *interval* seconds → repeat.

    Returns the list of created PropertyAnimation objects.
    """
    eyes = _find_component(char, "eyes")
    if eyes is None:
        return []

    eye_parts = [c for c in (eyes.children or ()) if c.shape_type == "ellipse"]
    if not eye_parts:
        return []

    timeline = Timeline.get_current()
    created: list[PropertyAnimation] = []

    swoosh = get_easing("sine-in-out")

    for part in eye_parts:
        tid = id(part)
        original_ry = part.params.get("ry", 6.0)

        # Close
        close_a = PropertyAnimation(
            target_id=tid,
            property="ry",
            from_value=original_ry,
            to_value=0.3,
            duration=close_duration,
            ease_fn=swoosh,
        )
        timeline.add(close_a)
        created.append(close_a)

        # Hold closed
        hold_a = PropertyAnimation(
            target_id=tid,
            property="ry",
            from_value=0.3,
            to_value=0.3,
            duration=hold_duration,
            ease_fn=swoosh,
        )
        timeline.add(hold_a)
        created.append(hold_a)

        # Open
        open_a = PropertyAnimation(
            target_id=tid,
            property="ry",
            from_value=0.3,
            to_value=original_ry,
            duration=open_duration,
            ease_fn=swoosh,
        )
        timeline.add(open_a)
        created.append(open_a)

    return created


# ======================================================================
# Idle breath — subtle body oscillation
# ======================================================================


def idle_breath(
    char: GraphicObject,
    cycle_duration: float = 2.4,
    amplitude: float = 1.2,
) -> list[PropertyAnimation]:
    """Register a subtle breathing animation on the body.

    The body moves up/down by *amplitude* pixels over *cycle_duration*
    seconds. Repeats infinitely.
    """
    body = _find_component(char, "body")
    if body is None:
        return []

    timeline = Timeline.get_current()
    tid = id(body)
    created: list[PropertyAnimation] = []

    # Get current y position from transform
    current_y = body.transform.f if body.transform else 0.0

    sine = get_easing("sine-in-out")

    # Breath in — move up slightly
    in_a = PropertyAnimation(
        target_id=tid,
        property="transform.ty",
        from_value=current_y,
        to_value=current_y - amplitude,
        duration=cycle_duration * 0.4,
        ease_fn=sine,
    )
    timeline.add(in_a)
    created.append(in_a)

    # Breath out — move down slightly
    out_a = PropertyAnimation(
        target_id=tid,
        property="transform.ty",
        from_value=current_y - amplitude,
        to_value=current_y,
        duration=cycle_duration * 0.6,
        ease_fn=sine,
    )
    timeline.add(out_a)
    created.append(out_a)

    # Slight chest scale (sx) oscillation
    torso_w = body.metadata.get("torso_width", 50)
    sx_a = PropertyAnimation(
        target_id=tid,
        property="transform.sx",
        from_value=1.0,
        to_value=1.015,
        duration=cycle_duration * 0.4,
        ease_fn=sine,
        repeat="infinite",
    )
    timeline.add(sx_a)
    created.append(sx_a)

    return created


# ======================================================================
# Walk cycle — x movement + body bounce + arm sway
# ======================================================================


def walk_cycle(
    char: GraphicObject,
    distance: float = 80.0,
    duration: float = 1.0,
    bounce: float = 3.0,
) -> list[PropertyAnimation]:
    """Register a walk cycle animation.

    Moves the character *distance* pixels to the right over *duration*
    seconds, with a vertical bounce and rotation sway.
    Repeats infinitely (wrap mode).
    """
    timeline = Timeline.get_current()
    tid = id(char)
    created: list[PropertyAnimation] = []

    quad = get_easing("quad-in-out")

    # Forward movement
    move = PropertyAnimation(
        target_id=tid,
        property="transform.tx",
        from_value=0.0,
        to_value=distance,
        duration=duration,
        ease_fn=get_easing("linear"),
        repeat="infinite",
    )
    timeline.add(move)
    created.append(move)

    # Vertical bounce — sine wave
    bounce_a = PropertyAnimation(
        target_id=tid,
        property="transform.ty",
        from_value=0.0,
        to_value=-bounce,
        duration=duration * 0.5,
        ease_fn=quad,
        repeat="infinite",
    )
    timeline.add(bounce_a)
    created.append(bounce_a)

    # Tilt sway — slight rotation
    body = _find_component(char, "body")
    if body is not None:
        bid = id(body)
        tilt = PropertyAnimation(
            target_id=bid,
            property="transform.rot",
            from_value=-0.04,
            to_value=0.04,
            duration=duration * 0.5,
            ease_fn=get_easing("sine-in-out"),
            repeat="infinite",
        )
        timeline.add(tilt)
        created.append(tilt)

    return created


# ======================================================================
# PML builtins registration
# ======================================================================


def _pml_blink(char: Any) -> list[PropertyAnimation]:
    """(blink <character>) — add blink animation to a character."""
    if not isinstance(char, GraphicObject):
        return []
    return blink(char)


def _pml_idle_breath(char: Any) -> list[PropertyAnimation]:
    """(idle-breath <character>) — add idle breathing animation."""
    if not isinstance(char, GraphicObject):
        return []
    return idle_breath(char)


def _pml_walk_cycle(char: Any, **kwargs: Any) -> list[PropertyAnimation]:
    """(walk-cycle <character> [:distance 80] [:duration 1.0] [:bounce 3]) —
    add a walk cycle animation."""
    if not isinstance(char, GraphicObject):
        return []
    return walk_cycle(
        char,
        distance=float(kwargs.get("distance", 80)),
        duration=float(kwargs.get("duration", 1.0)),
        bounce=float(kwargs.get("bounce", 3)),
    )


def register_character_animation(env: Any) -> None:
    """Register character animation builtins in the environment."""
    from pml.types import BuiltinProcedure

    env.define("blink", BuiltinProcedure("blink", _pml_blink))
    env.define("idle-breath", BuiltinProcedure("idle-breath", _pml_idle_breath))
    env.define(
        "walk-cycle",
        BuiltinProcedure("walk-cycle", _pml_walk_cycle, accepts_kwargs=True),
    )
