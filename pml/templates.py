"""PML template system — reusable sprite/asset templates for LLM-friendly generation."""

from __future__ import annotations

import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from pml.types import BuiltinProcedure


# ======================================================================
# Template model
# ======================================================================


@dataclass
class Template:
    """A named, parameterized PML template.

    Templates are reusable sprite/scene blueprints that LLMs can invoke
    with ``(use-template ...)`` instead of writing full layout code.
    """

    name: str
    """Short name, e.g. ``\"portrait\"``."""

    params: list[dict[str, Any]] = field(default_factory=list)
    """Parameter descriptors: ``[{\"name\": ..., \"type\": ..., \"default\": ...}, ...]``."""

    category: str = "general"
    """Category for grouping: ``\"character\"``, ``\"ui\"``, ``\"scene\"``, ``\"items\"``."""

    description: str = ""
    """Human-readable summary of what this template produces."""

    file: str = ""
    """Path to the template's ``.pml`` definition file."""

    preview: str = ""
    """Path to a preview image (thumbnail of template output)."""


# ======================================================================
# Template registry
# ======================================================================

_template_registry: dict[str, Template] = {}


def register_template(template: Template) -> None:
    _template_registry[template.name] = template


def get_template(name: str) -> Template | None:
    return _template_registry.get(name)


def list_templates(category: str | None = None) -> list[dict[str, Any]]:
    """List all registered templates, optionally filtered by category.

    Returns a list of dicts with name, params, category, and description.
    """
    result: list[dict[str, Any]] = []
    for name, t in _template_registry.items():
        if category is not None and t.category != category:
            continue
        result.append({
            "name": t.name,
            "params": t.params,
            "category": t.category,
            "description": t.description,
            "preview": t.preview if t.preview and os.path.exists(t.preview) else None,
        })
    return sorted(result, key=lambda x: x["name"])


# ======================================================================
# Stdlib template discovery
# ======================================================================

_STDLIB_TEMPLATES_DIR: str = ""


def _discover_stdlib_templates() -> None:
    """Scan ``stdlib/templates/`` for template definitions and register them.

    Each subdirectory under ``stdlib/templates/`` should contain:
      - ``template.pml`` — PML code defining the template procedures
      - ``README.md`` — parameter docs (optional)
      - ``preview.png`` — thumbnail (optional)
    """
    global _STDLIB_TEMPLATES_DIR
    pml_root = Path(__file__).parent
    templates_dir = pml_root.parent / "stdlib" / "templates"

    if not templates_dir.is_dir():
        return

    _STDLIB_TEMPLATES_DIR = str(templates_dir)

    for entry in sorted(templates_dir.iterdir()):
        if not entry.is_dir():
            continue
        tmpl_file = entry / "template.pml"
        readme_file = entry / "README.md"
        preview_file = entry / "preview.png"

        if not tmpl_file.is_file():
            continue

        name = entry.name

        # Parse basic metadata from README header if present
        description = ""
        params: list[dict[str, Any]] = []
        category = "general"

        if readme_file.is_file():
            text = readme_file.read_text(encoding="utf-8")
            for line in text.splitlines():
                line = line.strip()
                if line.lower().startswith("description:"):
                    description = line.split(":", 1)[1].strip()
                elif line.lower().startswith("category:"):
                    category = line.split(":", 1)[1].strip().lower()
                elif line.lower().startswith("- `"):
                    # Parameter line: - `name` (type) description
                    pass

        if not description:
            description = f"{name} template"

        register_template(
            Template(
                name=name,
                params=params,
                category=category,
                description=description,
                file=str(tmpl_file),
                preview=str(preview_file) if preview_file.is_file() else "",
            )
        )


# ======================================================================
# PML builtins
# ======================================================================


def _builtin_list_templates(category: str | None = None) -> list[dict[str, Any]]:
    """(list-templates) or (list-templates \"character\")"""
    return list_templates(category)


def _builtin_use_template(name: str, **kwargs: Any) -> Any:
    """(use-template \"portrait\" :expression \"happy\") — invoke a template.

    Loads the template's ``.pml`` file and calls its main procedure.
    """
    template = get_template(name)
    if template is None:
        from pml.errors import PMLError
        raise PMLError(f"template '{name}' not found")

    if not template.file or not os.path.isfile(template.file):
        from pml.errors import PMLError
        raise PMLError(f"template file not found: {template.file}")

    # Load and execute the template file
    from pml.repl import run_file, create_global_env

    env = create_global_env()
    # The template .pml should define a procedure named after the template
    # that accepts keyword args (or just evaluate as-is for procedural templates)
    result = run_file(template.file, env=env)

    # If the template defined a main function, call it
    try:
        fn = env.lookup(name.replace("-", "_"))
        from pml.evaluator import apply_function
        return apply_function(fn, [], kwargs)
    except Exception:
        return result


# ======================================================================
# Registration
# ======================================================================


def register_templates(env: Any) -> None:
    """Register template builtins and discover stdlib templates."""
    _discover_stdlib_templates()

    env.define(
        "list-templates",
        BuiltinProcedure("list-templates", _builtin_list_templates),
    )
    env.define(
        "use-template",
        BuiltinProcedure("use-template", _builtin_use_template, accepts_kwargs=True),
    )