"""PML skeleton system — joint templates, instances, and forward kinematics."""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import Any


@dataclass(frozen=True)
class Joint:
    """A single joint definition within a skeleton template.

    Attributes:
        name: Joint identifier (e.g. 'shoulder', 'elbow').
        pos: Offset from parent joint (dx, dy).
        length: Bone segment length from this joint.
        angle: Initial angle in radians (relative to parent direction).
        min_angle: Optional lower bound constraint (radians).
        max_angle: Optional upper bound constraint (radians).
    """

    name: str
    pos: tuple[float, float] = (0.0, 0.0)
    length: float = 0.0
    angle: float = 0.0
    min_angle: float | None = None
    max_angle: float | None = None


@dataclass(frozen=True)
class SkeletonTemplate:
    """A skeleton blueprint — defines joint topology and initial pose.

    Attributes:
        name: Template name.
        root_params: Parameter names for root position, e.g. ("x", "y").
        joints: Ordered joint chain (parent → child).
    """

    name: str
    root_params: tuple[str, str] = ("x", "y")
    joints: tuple[Joint, ...] = ()

    def joint_index(self, name: str) -> int:
        """Return the index of a joint by name, or raise ValueError."""
        for i, j in enumerate(self.joints):
            if j.name == name:
                return i
        raise ValueError(f"Joint '{name}' not found in skeleton '{self.name}'")

    def __repr__(self) -> str:
        joint_names = ", ".join(j.name for j in self.joints)
        return f"<SkeletonTemplate {self.name} [{joint_names}]>"


class SkeletonInstance:
    """A live skeleton with mutable joint angles.

    Created from a SkeletonTemplate via instantiate-skeleton.
    IK solvers modify angles in-place.
    """

    __slots__ = ("template", "root_x", "root_y", "angles", "name", "_bindings")

    def __init__(
        self,
        template: SkeletonTemplate,
        root_x: float,
        root_y: float,
        name: str = "",
    ) -> None:
        self.template = template
        self.root_x = float(root_x)
        self.root_y = float(root_y)
        # Initialize angles from template defaults
        self.angles: list[float] = [j.angle for j in template.joints]
        self.name = name
        # Skin bindings: list of (graphic_object_id, [joint_names])
        self._bindings: list[tuple[int, list[str]]] = []

    def clamp_angle(self, index: int, angle: float) -> float:
        """Apply min/max constraints to a joint angle."""
        joint = self.template.joints[index]
        if joint.min_angle is not None:
            angle = max(joint.min_angle, angle)
        if joint.max_angle is not None:
            angle = min(joint.max_angle, angle)
        return angle

    def set_angle(self, index: int, angle: float) -> None:
        """Set a joint angle with constraint clamping."""
        self.angles[index] = self.clamp_angle(index, angle)

    def forward_kinematics(self) -> list[tuple[float, float]]:
        """Compute world-space positions for all joints.

        Returns a list of (x, y) tuples — one per joint.
        The position is the START of each bone segment.
        """
        positions: list[tuple[float, float]] = []
        # Start from root
        cx, cy = self.root_x, self.root_y
        cumulative_angle = 0.0

        for i, joint in enumerate(self.template.joints):
            # Apply offset from parent (in parent's local frame)
            dx, dy = joint.pos
            if dx != 0 or dy != 0:
                # Rotate offset by cumulative angle of parent
                cos_a = math.cos(cumulative_angle)
                sin_a = math.sin(cumulative_angle)
                cx += dx * cos_a - dy * sin_a
                cy += dx * sin_a + dy * cos_a

            # Record this joint's world position
            positions.append((cx, cy))

            # Cumulative angle includes this joint's angle
            cumulative_angle += self.angles[i]

            # Advance to end of bone (start of next joint)
            cx += joint.length * math.cos(cumulative_angle)
            cy += joint.length * math.sin(cumulative_angle)

        return positions

    def joint_world_position(self, joint_name: str) -> tuple[float, float]:
        """Get a specific joint's world-space position."""
        index = self.template.joint_index(joint_name)
        positions = self.forward_kinematics()
        return positions[index]

    def end_effector_position(self) -> tuple[float, float]:
        """Get the position at the end of the last bone segment."""
        positions = self.forward_kinematics()
        if not positions:
            return (self.root_x, self.root_y)

        # The end effector is the tip of the last bone
        last_joint = self.template.joints[-1]
        last_pos = positions[-1]

        # Cumulative angle through ALL joints determines the bone direction
        cumulative = sum(self.angles)
        ex = last_pos[0] + last_joint.length * math.cos(cumulative)
        ey = last_pos[1] + last_joint.length * math.sin(cumulative)
        return (ex, ey)

    def chain_positions(
        self, end_effector_name: str
    ) -> list[tuple[float, float]]:
        """Return positions from root to end effector, including bone tip.

        The returned list has len(joints_up_to_effector) + 1 elements:
        [joint_0_pos, joint_1_pos, ..., end_effector_tip_pos]
        """
        end_idx = self.template.joint_index(end_effector_name)
        positions = self.forward_kinematics()

        # Get chain up to and including end effector joint
        chain = list(positions[: end_idx + 1])

        # Add the end effector tip (end of last bone in chain)
        cumulative = sum(self.angles[: end_idx + 1])
        tip_x = chain[-1][0] + self.template.joints[end_idx].length * math.cos(cumulative)
        tip_y = chain[-1][1] + self.template.joints[end_idx].length * math.sin(cumulative)
        chain.append((tip_x, tip_y))

        return chain

    def bone_lengths(self, end_effector_name: str) -> list[float]:
        """Return bone lengths from root to end effector."""
        end_idx = self.template.joint_index(end_effector_name)
        return [self.template.joints[i].length for i in range(end_idx + 1)]

    def __repr__(self) -> str:
        return (
            f"<SkeletonInstance {self.name or self.template.name} "
            f"root=({self.root_x:.1f}, {self.root_y:.1f}) "
            f"joints={len(self.template.joints)}>"
        )
