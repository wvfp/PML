"""PML component registry — central registry for semantic components."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any, Callable

from pml.environment import Environment
from pml.graphics.objects import GraphicObject
from pml.types import BuiltinProcedure

if TYPE_CHECKING:
    from pml.sprites.validator import ParamSchema


class ComponentRegistry:
    """Registry for semantic sprite components (body, head, eyes, etc.)."""

    _instance: ComponentRegistry | None = None

    def __init__(self) -> None:
        self._components: dict[str, Callable[..., GraphicObject]] = {}
        self._schemas: dict[str, ParamSchema] = {}
        self._categories: dict[str, str] = {}

    @classmethod
    def get_instance(cls) -> ComponentRegistry:
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def register(
        self,
        name: str,
        factory: Callable[..., GraphicObject],
        schema: ParamSchema | None = None,
        category: str | None = None,
    ) -> None:
        self._components[name] = factory
        if schema is not None:
            self._schemas[name] = schema
        if category is not None:
            self._categories[name] = category

    def create(self, name: str, **kwargs: Any) -> GraphicObject:
        if name not in self._components:
            raise ValueError(f"Unknown component: {name}")
        return self._components[name](**kwargs)

    def has(self, name: str) -> bool:
        return name in self._components

    def list_components(self) -> list[str]:
        return list(self._components.keys())

    def get_schema(self, name: str) -> ParamSchema | None:
        return self._schemas.get(name)

    def get_category(self, name: str) -> str | None:
        return self._categories.get(name)

    def list_by_category(self, category: str) -> list[str]:
        return [n for n, c in self._categories.items() if c == category]

    def categories(self) -> list[str]:
        return sorted(set(self._categories.values()))


def register_components(env: Environment) -> None:
    """Register all semantic components as builtins."""
    # Phase 2+3: core character components
    from pml.sprites.components.body import create_body, _BODY_SCHEMA
    from pml.sprites.components.head import create_head, _HEAD_SCHEMA
    from pml.sprites.components.eyes import create_eyes, _EYES_SCHEMA
    from pml.sprites.components.mouth import create_mouth, _MOUTH_SCHEMA
    from pml.sprites.components.hair import create_hair, _HAIR_SCHEMA
    from pml.sprites.components.character import create_character, _CHARACTER_SCHEMA

    # Phase 4: extended components
    from pml.sprites.components.outfit import create_outfit, _OUTFIT_SCHEMA
    from pml.sprites.components.items import (
        create_weapon,
        create_potion,
        create_chest,
        create_generic_item,
        _WEAPON_SCHEMA,
        _POTION_SCHEMA,
        _CHEST_SCHEMA,
        _GENERIC_ITEM_SCHEMA,
    )
    from pml.sprites.components.ui_widgets import (
        create_button,
        create_panel,
        create_health_bar,
        create_icon,
        _BUTTON_SCHEMA,
        _PANEL_SCHEMA,
        _HEALTH_BAR_SCHEMA,
        _ICON_SCHEMA,
    )
    from pml.sprites.components.scene_elements import (
        create_tile,
        create_decoration,
        create_background,
        _TILE_SCHEMA,
        _DECORATION_SCHEMA,
        _BACKGROUND_SCHEMA,
    )

    registry = ComponentRegistry.get_instance()

    # (name, factory, schema, category)
    components = [
        # Character
        ("body", create_body, _BODY_SCHEMA, "character"),
        ("head", create_head, _HEAD_SCHEMA, "character"),
        ("anime-eyes", create_eyes, _EYES_SCHEMA, "character"),
        ("mouth", create_mouth, _MOUTH_SCHEMA, "character"),
        ("hair", create_hair, _HAIR_SCHEMA, "character"),
        ("character", create_character, _CHARACTER_SCHEMA, "character"),
        # Outfit
        ("outfit", create_outfit, _OUTFIT_SCHEMA, "character"),
        # Items
        ("weapon", create_weapon, _WEAPON_SCHEMA, "items"),
        ("potion", create_potion, _POTION_SCHEMA, "items"),
        ("chest", create_chest, _CHEST_SCHEMA, "items"),
        ("generic-item", create_generic_item, _GENERIC_ITEM_SCHEMA, "items"),
        # UI widgets
        ("button", create_button, _BUTTON_SCHEMA, "ui"),
        ("panel", create_panel, _PANEL_SCHEMA, "ui"),
        ("health-bar", create_health_bar, _HEALTH_BAR_SCHEMA, "ui"),
        ("icon", create_icon, _ICON_SCHEMA, "ui"),
        # Scene elements
        ("tile", create_tile, _TILE_SCHEMA, "scene"),
        ("decoration", create_decoration, _DECORATION_SCHEMA, "scene"),
        ("background", create_background, _BACKGROUND_SCHEMA, "scene"),
    ]

    for name, factory, schema, category in components:
        registry.register(name, factory, schema=schema, category=category)
        env.define(name, BuiltinProcedure(name, factory, accepts_kwargs=True))
