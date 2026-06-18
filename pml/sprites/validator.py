"""PML component parameter validator."""

from __future__ import annotations

from typing import Any

from pml.sprites.palette import resolve_color


class ParamSchema:
    """Schema for validating component parameters.

    Supports enum values (with fallback) and continuous values (with clamp).
    """

    def __init__(self) -> None:
        self._fields: dict[str, dict[str, Any]] = {}

    def enum(self, name: str, values: list[str], default: str) -> ParamSchema:
        """Register an enum parameter with a default fallback."""
        self._fields[name] = {
            "type": "enum",
            "values": values,
            "default": default,
        }
        return self

    def number(
        self, name: str, default: float, min_val: float = -1e9, max_val: float = 1e9
    ) -> ParamSchema:
        """Register a numeric parameter with clamp range."""
        self._fields[name] = {
            "type": "number",
            "default": default,
            "min": min_val,
            "max": max_val,
        }
        return self

    def boolean(self, name: str, default: bool = False) -> ParamSchema:
        self._fields[name] = {"type": "boolean", "default": default}
        return self

    def color(self, name: str, default: str = "#808080") -> ParamSchema:
        self._fields[name] = {"type": "color", "default": default}
        return self

    def any_type(self, name: str, default: Any = None) -> ParamSchema:
        self._fields[name] = {"type": "any", "default": default}
        return self

    def to_dict(self) -> dict[str, Any]:
        """Serialize schema to LLM-friendly dict format.

        Returns a dict with a "params" key containing a list of parameter specs:
        [{"name": ..., "type": ..., "default": ..., ...}, ...]
        """
        params: list[dict[str, Any]] = []
        for name, spec in self._fields.items():
            entry: dict[str, Any] = {
                "name": name,
                "type": spec["type"],
                "default": spec.get("default"),
            }
            if spec["type"] == "enum":
                entry["values"] = list(spec["values"])
            elif spec["type"] == "number":
                entry["min"] = spec["min"]
                entry["max"] = spec["max"]
            params.append(entry)
        return {"params": params}


def validate_params(schema: ParamSchema, kwargs: dict[str, Any]) -> dict[str, Any]:
    """Validate and clean keyword arguments against a schema.

    - Enum values: fallback to default if not in allowed set (with warning)
    - Numeric values: clamp to [min, max]
    - Missing values: use defaults
    - Unknown values: passed through (ignored by components that don't use them)
    """
    result: dict[str, Any] = {}

    for field_name, field_spec in schema._fields.items():
        value = kwargs.get(field_name)
        ftype = field_spec["type"]

        if value is None:
            result[field_name] = field_spec["default"]
            continue

        if ftype == "enum":
            if str(value) not in [str(v) for v in field_spec["values"]]:
                print(
                    f"Warning: invalid value '{value}' for '{field_name}', "
                    f"falling back to '{field_spec['default']}'"
                )
                result[field_name] = field_spec["default"]
            else:
                result[field_name] = str(value)

        elif ftype == "number":
            try:
                v = float(value)
                v = max(field_spec["min"], min(field_spec["max"], v))
                result[field_name] = v
            except (TypeError, ValueError):
                result[field_name] = field_spec["default"]

        elif ftype == "boolean":
            result[field_name] = bool(value)

        elif ftype == "color":
            raw = str(value) if value else field_spec["default"]
            result[field_name] = resolve_color(raw)

        else:
            result[field_name] = value

    # Pass through any extra kwargs not in schema
    for k, v in kwargs.items():
        if k not in result:
            result[k] = v

    return result
