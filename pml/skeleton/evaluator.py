"""PML skeleton evaluator — defskelton special form."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import ArityError, PMLTypeError
from pml.skeleton.skeleton import Joint, SkeletonTemplate
from pml.types import Expr, Keyword, Symbol


def _eval_defskeleton(expr: Expr, env: Environment) -> None:
    """(defskeleton <name> (<root-x> <root-y>)
       (joint <joint-name> :pos (<dx> <dy>) :length <len> :angle <angle>
              [:min <min>] [:max <max>]) ...)

    Defines a skeleton template and binds it in the environment.
    Joint clauses use keywords (:pos, :length, etc.) that must NOT be
    evaluated — hence this is a special form.
    """
    if len(expr) < 4:
        raise ArityError(
            "defskeleton expects name, root params, and at least one joint"
        )

    # 1. Parse template name
    name_expr = expr[1]
    if not isinstance(name_expr, Symbol):
        raise PMLTypeError(f"defskeleton: name must be a symbol, got {name_expr!r}")

    # 2. Parse root parameter names
    root_params_expr = expr[2]
    if not isinstance(root_params_expr, list) or len(root_params_expr) != 2:
        raise PMLTypeError(
            "defskeleton: root params must be (param-x param-y)"
        )
    root_params = []
    for p in root_params_expr:
        if not isinstance(p, Symbol):
            raise PMLTypeError(
                f"defskeleton: root param must be symbol, got {p!r}"
            )
        root_params.append(p.name)

    # 3. Parse joint clauses
    joints: list[Joint] = []
    for joint_expr in expr[3:]:
        if not isinstance(joint_expr, list) or len(joint_expr) < 2:
            raise PMLTypeError(
                "defskeleton: each joint must be (joint name :key val ...)"
            )

        # Check that first element is 'joint' keyword/symbol
        head = joint_expr[0]
        if isinstance(head, Symbol) and head.name != "joint":
            raise PMLTypeError(
                f"defskeleton: expected 'joint', got '{head.name}'"
            )

        # Joint name
        joint_name = joint_expr[1]
        if not isinstance(joint_name, Symbol):
            raise PMLTypeError(
                f"defskeleton: joint name must be symbol, got {joint_name!r}"
            )

        # Parse keyword arguments
        pos = (0.0, 0.0)
        length = 0.0
        angle = 0.0
        min_angle: float | None = None
        max_angle: float | None = None

        i = 2
        while i < len(joint_expr):
            kw = joint_expr[i]
            if isinstance(kw, Keyword):
                if i + 1 >= len(joint_expr):
                    raise PMLTypeError(
                        f"defskeleton: keyword {kw} has no value"
                    )
                val = joint_expr[i + 1]

                if kw.name == "pos":
                    if not isinstance(val, list) or len(val) != 2:
                        raise PMLTypeError(
                            "defskeleton: :pos must be (dx dy)"
                        )
                    pos = (float(val[0]), float(val[1]))
                elif kw.name == "length":
                    length = float(val)
                elif kw.name == "angle":
                    angle = float(val)
                elif kw.name == "min":
                    min_angle = float(val)
                elif kw.name == "max":
                    max_angle = float(val)

                i += 2
            else:
                i += 1

        joints.append(
            Joint(
                name=joint_name.name,
                pos=pos,
                length=length,
                angle=angle,
                min_angle=min_angle,
                max_angle=max_angle,
            )
        )

    # 4. Create template and bind in environment
    template = SkeletonTemplate(
        name=name_expr.name,
        root_params=tuple(root_params),
        joints=tuple(joints),
    )
    env.define(name_expr.name, template)
    return None
