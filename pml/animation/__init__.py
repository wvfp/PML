"""PML animation subsystem — registration and builtins.

Registers animate, parallel, sequence, play, stop, pause, seek,
animation-state, and every-frame as PML builtins.
"""

from __future__ import annotations

from typing import Any

from pml.animation.easing import get_easing
from pml.animation.engine import (
    Animation,
    ParallelAnimation,
    PropertyAnimation,
    SequenceAnimation,
)
from pml.animation.timeline import Timeline
from pml.environment import Environment
from pml.types import BuiltinProcedure, Symbol


def _animate(
    target: Any,
    property: Any,
    from_val: Any,
    to_val: Any,
    duration: Any,
    **kwargs: Any,
) -> Animation:
    """(animate <target> <property> <from> <to> <duration>
                [:ease <symbol>] [:repeat <int-or-symbol>] [:delay <number>])"""
    # Extract property name from Symbol
    prop_name = property.name if isinstance(property, Symbol) else str(property)

    # Extract easing
    ease = kwargs.get("ease", "linear")
    ease_fn = get_easing(ease)

    # Extract repeat
    repeat: int | str = kwargs.get("repeat", 1)
    if isinstance(repeat, Symbol):
        if repeat.name == "infinite":
            repeat = "infinite"
        else:
            repeat = 1
    else:
        repeat = max(1, int(repeat))

    # Extract delay
    delay = float(kwargs.get("delay", 0))

    # Target is a GraphicObject — use its id()
    target_id = id(target)

    anim = PropertyAnimation(
        target_id=target_id,
        property=prop_name,
        from_value=from_val,
        to_value=to_val,
        duration=float(duration),
        ease_fn=ease_fn,
        delay=delay,
        repeat=repeat,
    )

    # Register on the global timeline
    timeline = Timeline.get_current()
    timeline.add(anim)

    return anim


def _parallel(*anims: Any) -> ParallelAnimation:
    """(parallel <anim> ...) — play all animations simultaneously."""
    children = [a for a in anims if isinstance(a, Animation)]
    combined = ParallelAnimation(children)

    # Replace individual animations with the combined one
    timeline = Timeline.get_current()
    for child in children:
        if child in timeline.animations:
            timeline.animations.remove(child)
    timeline.add(combined)

    return combined


def _sequence(*anims: Any) -> SequenceAnimation:
    """(sequence <anim> ...) — play animations one after another."""
    children = [a for a in anims if isinstance(a, Animation)]
    combined = SequenceAnimation(children)

    timeline = Timeline.get_current()
    for child in children:
        if child in timeline.animations:
            timeline.animations.remove(child)
    timeline.add(combined)

    return combined


def _play(anim: Any = None) -> None:
    """(play) or (play <anim>) — start playback."""
    timeline = Timeline.get_current()
    timeline.play()


def _stop(anim: Any = None) -> None:
    """(stop) or (stop <anim>) — stop and reset."""
    timeline = Timeline.get_current()
    timeline.stop()


def _pause(anim: Any = None) -> None:
    """(pause) or (pause <anim>) — pause playback."""
    timeline = Timeline.get_current()
    timeline.pause()


def _seek(anim: Any, time: Any = None) -> None:
    """(seek <anim> <time>) — jump to a specific time."""
    t = float(time) if time is not None else float(anim)
    timeline = Timeline.get_current()
    timeline.seek(t)


def _animation_state(anim: Any = None) -> Symbol:
    """(animation-state) — return current playback state as a Symbol."""
    timeline = Timeline.get_current()
    return Symbol(timeline.state)


def _every_frame(thunk: Any) -> None:
    """(every-frame <thunk>) — register a per-frame callback."""
    timeline = Timeline.get_current()
    timeline.add_frame_hook(thunk)


def register_animation(env: Environment) -> None:
    """Register all animation builtins in the environment."""
    env.define(
        "animate",
        BuiltinProcedure("animate", _animate, accepts_kwargs=True),
    )
    env.define("parallel", BuiltinProcedure("parallel", _parallel))
    env.define("sequence", BuiltinProcedure("sequence", _sequence))
    env.define("play", BuiltinProcedure("play", _play))
    env.define("stop", BuiltinProcedure("stop", _stop))
    env.define("pause", BuiltinProcedure("pause", _pause))
    env.define("seek", BuiltinProcedure("seek", _seek))
    env.define(
        "animation-state",
        BuiltinProcedure("animation-state", _animation_state),
    )
    env.define("every-frame", BuiltinProcedure("every-frame", _every_frame))
