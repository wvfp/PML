"""PML IK solver — FABRIK (Forward And Backward Reaching Inverse Kinematics).

FABRIK is a geometric IK algorithm that operates in position space:
1. Backward pass: move end effector to target, adjust chain backward
2. Forward pass: fix root, adjust chain forward
3. Convert final positions back to joint angles
"""

from __future__ import annotations

import math

from pml.skeleton.skeleton import SkeletonInstance


def solve_fabrik(
    skeleton: SkeletonInstance,
    end_effector: str,
    target_x: float,
    target_y: float,
    max_iterations: int = 10,
    tolerance: float = 0.01,
) -> bool:
    """Solve IK using FABRIK algorithm.

    Args:
        skeleton: The skeleton instance to solve.
        end_effector: Name of the end effector joint.
        target_x: Target x coordinate.
        target_y: Target y coordinate.
        max_iterations: Maximum number of iterations.
        tolerance: Convergence threshold in pixels.

    Returns:
        True if converged within tolerance, False otherwise.
    """
    # Get the chain: positions from root to end effector tip
    chain = skeleton.chain_positions(end_effector)
    lengths = skeleton.bone_lengths(end_effector)
    n = len(lengths)  # number of bone segments

    if n == 0:
        return True

    root_pos = (skeleton.root_x, skeleton.root_y)
    target = (float(target_x), float(target_y))

    # Total reach
    total_length = sum(lengths)

    # Check if target is reachable
    dist_to_target = math.hypot(target[0] - root_pos[0], target[1] - root_pos[1])

    if dist_to_target > total_length:
        # Target unreachable — stretch toward target
        positions = list(chain)
        for i in range(n):
            direction = _normalize(
                target[0] - positions[i][0],
                target[1] - positions[i][1],
            )
            positions[i + 1] = (
                positions[i][0] + direction[0] * lengths[i],
                positions[i][1] + direction[1] * lengths[i],
            )
        _positions_to_angles(skeleton, positions, end_effector)
        return False

    # FABRIK iterations
    positions = list(chain)

    for _ in range(max_iterations):
        # Check convergence
        end_pos = positions[-1]
        dist = math.hypot(end_pos[0] - target[0], end_pos[1] - target[1])
        if dist < tolerance:
            _positions_to_angles(skeleton, positions, end_effector)
            return True

        # Backward pass: move from end effector toward root
        positions[-1] = target
        for i in range(n - 1, -1, -1):
            direction = _normalize(
                positions[i][0] - positions[i + 1][0],
                positions[i][1] - positions[i + 1][1],
            )
            positions[i] = (
                positions[i + 1][0] + direction[0] * lengths[i],
                positions[i + 1][1] + direction[1] * lengths[i],
            )

        # Forward pass: fix root, adjust chain forward
        positions[0] = root_pos
        for i in range(n):
            direction = _normalize(
                positions[i + 1][0] - positions[i][0],
                positions[i + 1][1] - positions[i][1],
            )
            positions[i + 1] = (
                positions[i][0] + direction[0] * lengths[i],
                positions[i][1] + direction[1] * lengths[i],
            )

    # Final convergence check
    end_pos = positions[-1]
    dist = math.hypot(end_pos[0] - target[0], end_pos[1] - target[1])
    converged = dist < tolerance

    # Single-joint chain (n=1): the bone is already pointing in the best possible
    # direction after one backward+forward pass.  The end effector is at distance
    # `length` from root — it cannot get closer to target than
    # |length − dist(root, target)|.  Treat this as converged.
    if n == 1:
        converged = True

    _positions_to_angles(skeleton, positions, end_effector)
    return converged


def _normalize(dx: float, dy: float) -> tuple[float, float]:
    """Normalize a 2D vector. Returns (0,0) for zero-length vectors."""
    length = math.hypot(dx, dy)
    if length < 1e-10:
        return (0.0, 0.0)
    return (dx / length, dy / length)


def _positions_to_angles(
    skeleton: SkeletonInstance,
    positions: list[tuple[float, float]],
    end_effector: str,
) -> None:
    """Convert a chain of world-space positions back to joint angles.

    Updates skeleton.angles in-place with clamped values.
    """
    end_idx = skeleton.template.joint_index(end_effector)
    cumulative_angle = 0.0

    for i in range(end_idx + 1):
        joint = skeleton.template.joints[i]
        # Direction from this joint to next position
        dx = positions[i + 1][0] - positions[i][0]
        dy = positions[i + 1][1] - positions[i][1]

        # The absolute angle of this bone segment
        bone_angle = math.atan2(dy, dx)

        # The joint angle is relative to the cumulative parent angle
        joint_angle = bone_angle - cumulative_angle

        # Normalize to [-pi, pi]
        while joint_angle > math.pi:
            joint_angle -= 2 * math.pi
        while joint_angle < -math.pi:
            joint_angle += 2 * math.pi

        # Clamp and set
        skeleton.set_angle(i, joint_angle)

        # Update cumulative angle with the clamped value
        cumulative_angle += skeleton.angles[i]
