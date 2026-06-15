"""PML animation timeline — global animation manager and frame renderer.

Manages registered animations, playback state, and frame-by-frame
rendering for GIF/video output.
"""

from __future__ import annotations

from typing import Any, Callable

from pml.animation.engine import Animation
from pml.graphics.objects import GraphicObject


class Timeline:
    """Global animation timeline.

    Tracks registered animations and playback state.
    Singleton managed via module-level list (same pattern as canvas).
    """

    _instance: list[Timeline | None] = [None]

    def __init__(self) -> None:
        self.animations: list[Animation] = []
        self.frame_hooks: list[Callable[[], None]] = []
        self.state: str = "idle"  # idle | playing | paused | finished
        self.current_time: float = 0.0
        self._finished_callbacks: list[Callable[[], None]] = []

    @classmethod
    def get_current(cls) -> Timeline:
        """Get the current timeline, creating one if needed."""
        if cls._instance[0] is None:
            cls._instance[0] = cls()
        return cls._instance[0]

    @classmethod
    def reset(cls) -> None:
        """Reset the global timeline."""
        cls._instance[0] = None

    def add(self, anim: Animation) -> None:
        """Register an animation on the timeline."""
        self.animations.append(anim)

    def add_frame_hook(self, hook: Callable[[], None]) -> None:
        """Register a function to call before each frame render."""
        self.frame_hooks.append(hook)

    def play(self) -> None:
        self.state = "playing"

    def stop(self) -> None:
        self.state = "stopped"
        self.current_time = 0.0

    def pause(self) -> None:
        if self.state == "playing":
            self.state = "paused"

    def seek(self, time: float) -> None:
        self.current_time = max(0.0, float(time))

    def on_finished(self, callback: Callable[[], None]) -> None:
        self._finished_callbacks.append(callback)

    def get_total_duration(self) -> float:
        """Total duration from all registered animations."""
        if not self.animations:
            return 0.0
        durations = [a.get_duration() for a in self.animations]
        finite = [d for d in durations if d != float("inf")]
        if not finite:
            return 1.0  # Default 1 second for infinite-only animations
        return max(finite)

    def evaluate_at(self, t: float) -> list[tuple[int, str, Any]]:
        """Evaluate all animations at time t.

        Returns list of (target_id, property, value) tuples.
        """
        result: list[tuple[int, str, Any]] = []
        for anim in self.animations:
            result.extend(anim.get_value_at(t))
        return result

    def render_frames(
        self,
        canvas: Any,
        fps: int,
        backend: Any,
        width: int,
        height: int,
        is_sprite: bool = False,
        bg_color: str = "transparent",
    ) -> list[Any]:
        """Render all animation frames as images.

        Args:
            canvas: The Canvas object with the base scene.
            fps: Frames per second.
            backend: PillowBackend instance.
            width: Canvas width.
            height: Canvas height.
            is_sprite: Whether to apply sprite auto-centering.
            bg_color: Background color string.

        Returns:
            List of PIL Image objects (one per frame).
        """
        total_duration = self.get_total_duration()
        if total_duration <= 0:
            total_duration = 1.0  # Default 1 second

        num_frames = max(1, int(total_duration * fps))
        frames = []

        for i in range(num_frames):
            t = i / fps

            # Run frame hooks (may call ik-solve, updating skeleton state)
            for hook in self.frame_hooks:
                hook()

            # Evaluate all animations at time t
            modifications = self.evaluate_at(t)

            # Build a mapping: target_id → dict of property → value
            obj_mods: dict[int, dict[str, Any]] = {}
            for target_id, prop, value in modifications:
                if target_id not in obj_mods:
                    obj_mods[target_id] = {}
                obj_mods[target_id][prop] = value

            # Apply skin bindings: update bound graphics with joint transforms
            _merge_skin_bindings(obj_mods)

            # Create modified objects list
            modified_objects = []
            for obj in canvas.objects:
                if not isinstance(obj, GraphicObject):
                    modified_objects.append(obj)
                    continue

                mods = obj_mods.get(id(obj))
                if mods is None:
                    modified_objects.append(obj)
                else:
                    modified_objects.append(_apply_modifications(obj, mods))

            # Render this frame
            surface = backend.create_surface(width, height, bg_color)

            if is_sprite:
                from pml.transform import AffineTransform

                center_t = AffineTransform.translate(
                    width / 2, height * 0.45
                )
                for obj in modified_objects:
                    if isinstance(obj, GraphicObject):
                        centered = obj.with_transform(
                            center_t.compose(obj.transform)
                        )
                        backend.draw(surface, centered)
            else:
                for obj in modified_objects:
                    if isinstance(obj, GraphicObject):
                        backend.draw(surface, obj)

            frames.append(surface)

        return frames


def _apply_modifications(
    obj: GraphicObject, mods: dict[str, Any]
) -> GraphicObject:
    """Apply property modifications to a GraphicObject, returning a new copy.

    Supported property paths:
    - param keys (x, y, r, w, h, cx, cy, size, etc.) → obj.params[key]
    - 'fill' → obj.fill
    - 'stroke' → obj.stroke
    - 'stroke-width' → obj.stroke_width
    - 'transform' → obj.transform (full replacement)
    - 'transform.tx' → obj.transform.e
    - 'transform.ty' → obj.transform.f
    """
    from pml.transform import AffineTransform

    new_params = dict(obj.params)
    new_fill = obj.fill
    new_stroke = obj.stroke
    new_stroke_width = obj.stroke_width
    new_transform = obj.transform

    for prop, value in mods.items():
        if prop == "fill":
            new_fill = value
        elif prop == "stroke":
            new_stroke = value
        elif prop == "stroke-width":
            new_stroke_width = float(value)
        elif prop == "transform":
            if isinstance(value, AffineTransform):
                new_transform = value
        elif prop == "transform.tx":
            new_transform = AffineTransform(
                a=new_transform.a,
                b=new_transform.b,
                c=new_transform.c,
                d=new_transform.d,
                e=float(value),
                f=new_transform.f,
            )
        elif prop == "transform.ty":
            new_transform = AffineTransform(
                a=new_transform.a,
                b=new_transform.b,
                c=new_transform.c,
                d=new_transform.d,
                e=new_transform.e,
                f=float(value),
            )
        else:
            # Assume it's a param key
            new_params[prop] = value

    return GraphicObject(
        shape_type=obj.shape_type,
        params=new_params,
        fill=new_fill,
        stroke=new_stroke,
        stroke_width=new_stroke_width,
        transform=new_transform,
        children=obj.children,
        metadata=dict(obj.metadata),
    )


def _merge_skin_bindings(obj_mods: dict[int, dict[str, Any]]) -> None:
    """Process skeleton skin bindings and merge joint-driven transforms.

    For each graphic bound to a skeleton joint via bind-skin, compute
    the joint's world-space transform and add it to obj_mods.
    """
    try:
        from pml.skeleton import get_skin_bindings
    except ImportError:
        return

    import math

    from pml.transform import AffineTransform

    bindings = get_skin_bindings()
    if not bindings:
        return

    for gid, binding_list in bindings.items():
        for skeleton, joint_names in binding_list:
            if not joint_names:
                continue

            # Use the first joint for the transform
            joint_name = joint_names[0]
            try:
                idx = skeleton.template.joint_index(joint_name)
            except ValueError:
                continue

            # Compute joint world position and bone angle
            positions = skeleton.forward_kinematics()
            jx, jy = positions[idx]

            # Cumulative angle up to and including this joint
            cumulative = sum(skeleton.angles[: idx + 1])

            # Build transform: translate to joint position, rotate by bone angle
            joint_transform = AffineTransform.translate(jx, jy).compose(
                AffineTransform.rotate(cumulative)
            )

            if gid not in obj_mods:
                obj_mods[gid] = {}
            obj_mods[gid]["transform"] = joint_transform
