"""PML IK solver — CCD (Cyclic Coordinate Descent).

CCD iterates from the end joint toward the root, rotating each joint
to minimize the distance between the end effector and the target.
"""

from __future__ import annotations

import math

from pml.skeleton.skeleton import SkeletonInstance


def solve_ccd(
    skeleton: SkeletonInstance,
    end_effector: str,
    target_x: float,
    target_y: float,
    max_iterations: int = 10,
    tolerance: float = 0.01,
) -> bool:
    """Solve IK using Cyclic Coordinate Descent.

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
    end_idx = skeleton.template.joint_index(end_effector)
    target = (float(target_x), float(target_y))

    for _ in range(max_iterations):
        # Check convergence
        end_pos = skeleton.end_effector_position()
        dist = math.hypot(end_pos[0] - target[0], end_pos[1] - target[1])
        if dist < tolerance:
            return True

        # Iterate from end effector joint back to root
        for joint_idx in range(end_idx, -1, -1):
            # Compute current positions
            positions = skeleton.forward_kinematics()
            joint_pos = positions[joint_idx]

            # End effector tip position
            cumulative = sum(skeleton.angles[: end_idx + 1])
            end_joint_pos = positions[end_idx]
            tip_x = end_joint_pos[0] + skeleton.template.joints[end_idx].length * math.cos(cumulative)
            tip_y = end_joint_pos[1] + skeleton.template.joints[end_idx].length * math.sin(cumulative)
            tip = (tip_x, tip_y)

            # Vector from joint to current end effector
            v_end_x = tip[0] - joint_pos[0]
            v_end_y = tip[1] - joint_pos[1]

            # Vector from joint to target
            v_target_x = target[0] - joint_pos[0]
            v_target_y = target[1] - joint_pos[1]

            # Angle between the two vectors
            angle_end = math.atan2(v_end_y, v_end_x)
            angle_target = math.atan2(v_target_y, v_target_x)
            delta = angle_target - angle_end

            # Normalize delta to [-pi, pi]
            while delta > math.pi:
                delta -= 2 * math.pi
            while delta < -math.pi:
                delta += 2 * math.pi

            # Apply rotation
            new_angle = skeleton.angles[joint_idx] + delta
            skeleton.set_angle(joint_idx, new_angle)

    # Final convergence check
    end_pos = skeleton.end_effector_position()
    dist = math.hypot(end_pos[0] - target[0], end_pos[1] - target[1])

    # Single-joint chain: the bone is already pointing in the best possible
    # direction.  Converged.
    if end_idx == 0:
        return True

    return dist < tolerance
