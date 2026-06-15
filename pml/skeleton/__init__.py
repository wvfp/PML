"""PML skeleton subsystem — registration and builtins.

Registers instantiate-skeleton, ik-solve, bind-skin, and joint-position
as PML builtins. defskelton is registered as a special form in evaluator.py.
"""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import IKNoSolutionError, PMLTypeError
from pml.skeleton.skeleton import SkeletonInstance, SkeletonTemplate
from pml.types import BuiltinProcedure, Symbol


# Global skin bindings registry:
# Maps id(GraphicObject) → list of (SkeletonInstance, [joint_names])
_skin_bindings: dict[int, list[tuple[SkeletonInstance, list[str]]]] = {}


def get_skin_bindings() -> dict[int, list[tuple[SkeletonInstance, list[str]]]]:
    """Return the global skin bindings registry."""
    return _skin_bindings


def clear_skin_bindings() -> None:
    """Clear all skin bindings (for testing / runtime reset)."""
    _skin_bindings.clear()


# ======================================================================
# Builtin implementations
# ======================================================================


def _instantiate_skeleton(
    template: Any,
    root_x: Any,
    root_y: Any,
    **kwargs: Any,
) -> SkeletonInstance:
    """(instantiate-skeleton <template> <x> <y> [:name <string>])

    Create a skeleton instance from a template.
    Root position is given as two separate positional args: x y.
    """
    if not isinstance(template, SkeletonTemplate):
        raise PMLTypeError(
            f"instantiate-skeleton: expected SkeletonTemplate, got {type(template).__name__}"
        )

    rx = float(root_x)
    ry = float(root_y)

    name = kwargs.get("name", "")
    if isinstance(name, str):
        pass
    elif isinstance(name, Symbol):
        name = name.name
    else:
        name = str(name)

    return SkeletonInstance(template, rx, ry, name=name)


def _ik_solve(
    instance: Any,
    end_effector: Any,
    target_x: Any,
    target_y: Any,
    **kwargs: Any,
) -> Any:
    """(ik-solve <instance> <end-effector> <target-x> <target-y>
                [:method <symbol>] [:iterations <number>] [:tolerance <number>])

    Solve IK for the given end effector toward the target point.
    Returns #t if converged, #f otherwise.
    In strict mode, raises IKNoSolutionError on failure.
    """
    if not isinstance(instance, SkeletonInstance):
        raise PMLTypeError(
            f"ik-solve: expected SkeletonInstance, got {type(instance).__name__}"
        )

    # Parse end effector name
    if isinstance(end_effector, Symbol):
        ee_name = end_effector.name
    else:
        ee_name = str(end_effector)

    tx = float(target_x)
    ty = float(target_y)

    # Parse options
    method = kwargs.get("method", "fabrik")
    if isinstance(method, Symbol):
        method = method.name

    iterations = int(kwargs.get("iterations", 10))
    tolerance = float(kwargs.get("tolerance", 0.01))
    strict = kwargs.get("strict", False)

    # Dispatch to solver
    if method == "fabrik":
        from pml.skeleton.ik_fabrik import solve_fabrik

        converged = solve_fabrik(instance, ee_name, tx, ty, iterations, tolerance)
    elif method == "ccd":
        from pml.skeleton.ik_ccd import solve_ccd

        converged = solve_ccd(instance, ee_name, tx, ty, iterations, tolerance)
    else:
        raise PMLTypeError(
            f"ik-solve: unknown method '{method}', use 'fabrik' or 'ccd'"
        )

    if not converged and strict:
        raise IKNoSolutionError(
            f"IK solver '{method}' failed to converge for end effector "
            f"'{ee_name}' toward ({tx:.1f}, {ty:.1f}) within {iterations} iterations"
        )

    return converged


def _bind_skin(
    graphic: Any,
    instance: Any,
    *joint_names: Any,
) -> None:
    """(bind-skin <graphic> <instance> <joint-name> ...)

    Bind a graphic object to one or more skeleton joints.
    """
    if not isinstance(instance, SkeletonInstance):
        raise PMLTypeError(
            f"bind-skin: expected SkeletonInstance, got {type(instance).__name__}"
        )

    names: list[str] = []
    for jn in joint_names:
        if isinstance(jn, Symbol):
            names.append(jn.name)
        else:
            names.append(str(jn))

    gid = id(graphic)
    if gid not in _skin_bindings:
        _skin_bindings[gid] = []
    _skin_bindings[gid].append((instance, names))


def _joint_position(
    instance: Any,
    joint_name: Any,
) -> list[float]:
    """(joint-position <instance> <joint-name>)

    Return the world-space position of a joint as (x y).
    """
    if not isinstance(instance, SkeletonInstance):
        raise PMLTypeError(
            f"joint-position: expected SkeletonInstance, got {type(instance).__name__}"
        )

    if isinstance(joint_name, Symbol):
        name = joint_name.name
    else:
        name = str(joint_name)

    pos = instance.joint_world_position(name)
    return [pos[0], pos[1]]


# ======================================================================
# Registration
# ======================================================================


def register_skeleton(env: Environment) -> None:
    """Register all skeleton builtins and the defskelton special form."""
    # Register builtins
    env.define(
        "instantiate-skeleton",
        BuiltinProcedure(
            "instantiate-skeleton", _instantiate_skeleton, accepts_kwargs=True
        ),
    )
    env.define(
        "ik-solve",
        BuiltinProcedure("ik-solve", _ik_solve, accepts_kwargs=True),
    )
    env.define("bind-skin", BuiltinProcedure("bind-skin", _bind_skin))
    env.define(
        "joint-position",
        BuiltinProcedure("joint-position", _joint_position),
    )

    # Register defskelton as a special form
    from pml.evaluator import SPECIAL_FORMS
    from pml.skeleton.evaluator import _eval_defskeleton

    SPECIAL_FORMS["defskeleton"] = _eval_defskeleton
