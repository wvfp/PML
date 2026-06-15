"""Tests for Phase 4 components — outfit, items, UI widgets, scene elements."""

import os
import tempfile
from pathlib import Path

import pytest

from pml.graphics.objects import GraphicObject
from pml.sprites.components.outfit import create_outfit
from pml.sprites.components.items import (
    create_weapon,
    create_potion,
    create_chest,
    create_generic_item,
)
from pml.sprites.components.ui_widgets import (
    create_button,
    create_panel,
    create_health_bar,
    create_icon,
)
from pml.sprites.components.scene_elements import (
    create_tile,
    create_decoration,
    create_background,
)
from pml.sprites.components.character import create_character


# ======================================================================
# Outfit
# ======================================================================


class TestOutfit:
    def test_default_outfit(self):
        obj = create_outfit()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "outfit"
        assert obj.shape_type == "group"

    def test_outfit_with_all_params(self):
        obj = create_outfit(top="hoodie", bottom="pants", shoes="boots", detail="striped")
        assert obj.metadata["top"] == "hoodie"
        assert obj.metadata["bottom"] == "pants"
        assert obj.metadata["shoes"] == "boots"

    def test_outfit_invalid_type_fallback(self):
        obj = create_outfit(top="nonexistent")
        assert obj.metadata["top"] == "t-shirt"  # default fallback

    @pytest.mark.parametrize("top", [
        "t-shirt", "jacket", "hoodie", "dress", "armor", "robe", "suit", "tank", "sailor",
    ])
    def test_outfit_top_styles(self, top):
        obj = create_outfit(top=top)
        assert obj.metadata["top"] == top

    @pytest.mark.parametrize("bottom", ["pants", "skirt", "shorts", "long-skirt", "armor"])
    def test_outfit_bottom_styles(self, bottom):
        obj = create_outfit(bottom=bottom)
        assert obj.metadata["bottom"] == bottom


# ======================================================================
# Items
# ======================================================================


class TestWeapon:
    def test_default_weapon(self):
        obj = create_weapon()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "weapon"

    @pytest.mark.parametrize("wtype", [
        "sword", "axe", "bow", "staff", "dagger", "spear", "gun",
    ])
    def test_weapon_types(self, wtype):
        obj = create_weapon(type=wtype)
        assert obj.metadata["type"] == wtype

    def test_weapon_with_element(self):
        obj = create_weapon(type="sword", element="fire")
        assert obj.metadata["type"] == "sword"
        # Element glow adds extra children
        plain = create_weapon(type="sword", element="none")
        assert len(obj.children) > len(plain.children)

    def test_weapon_sizes(self):
        small = create_weapon(size="small")
        large = create_weapon(size="large")
        assert small.metadata["size"] == "small"
        assert large.metadata["size"] == "large"


class TestPotion:
    def test_default_potion(self):
        obj = create_potion()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "potion"

    @pytest.mark.parametrize("ptype", ["health", "mana", "buff", "poison", "bomb"])
    def test_potion_types(self, ptype):
        obj = create_potion(type=ptype)
        assert obj.metadata["type"] == ptype


class TestChest:
    def test_default_chest(self):
        obj = create_chest()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "chest"

    def test_chest_open_state(self):
        obj = create_chest(state="open")
        assert obj.metadata["state"] == "open"


class TestGenericItem:
    def test_default_generic_item(self):
        obj = create_generic_item()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "generic-item"

    def test_generic_item_diamond(self):
        obj = create_generic_item(**{"base-shape": "diamond"})
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "generic-item"


# ======================================================================
# UI Widgets
# ======================================================================


class TestButton:
    def test_default_button(self):
        obj = create_button()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "button"

    @pytest.mark.parametrize("style", ["rounded", "sharp", "pixel", "ornate"])
    def test_button_styles(self, style):
        obj = create_button(style=style)
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "button"

    @pytest.mark.parametrize("state", ["normal", "hover", "pressed", "disabled"])
    def test_button_states(self, state):
        obj = create_button(state=state)
        assert obj.metadata["state"] == state


class TestPanel:
    def test_default_panel(self):
        obj = create_panel()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "panel"

    def test_panel_with_title(self):
        obj = create_panel(title="Settings")
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "panel"


class TestHealthBar:
    def test_default_health_bar(self):
        obj = create_health_bar()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "health-bar"

    def test_health_bar_value(self):
        obj = create_health_bar(value=0.5)
        assert obj.metadata["value"] == 0.5

    def test_health_bar_clamp(self):
        obj = create_health_bar(value=1.5)
        assert obj.metadata["value"] == 1.0

    def test_health_bar_zero(self):
        obj = create_health_bar(value=0)
        assert obj.metadata["value"] == 0.0


class TestIcon:
    @pytest.mark.parametrize("itype", ["heart", "star", "coin", "gem", "shield", "skull"])
    def test_icon_types(self, itype):
        obj = create_icon(type=itype)
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["type"] == itype


# ======================================================================
# Scene Elements
# ======================================================================


class TestTile:
    def test_default_tile(self):
        obj = create_tile()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "tile"

    @pytest.mark.parametrize("ttype", [
        "grass", "stone", "wood", "sand", "water", "snow", "brick",
    ])
    def test_tile_types(self, ttype):
        obj = create_tile(type=ttype)
        assert obj.metadata["tile_type"] == ttype

    def test_tile_edge(self):
        obj = create_tile(edge="top")
        assert obj.metadata["edge"] == "top"

    def test_tile_size(self):
        obj = create_tile(size=64)
        assert obj.metadata["size"] == 64


class TestDecoration:
    def test_default_decoration(self):
        obj = create_decoration()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "decoration"

    @pytest.mark.parametrize("dtype", [
        "tree", "bush", "rock", "flower", "mushroom",
        "crate", "barrel", "torch", "sign", "fence", "lamp",
    ])
    def test_decoration_types(self, dtype):
        obj = create_decoration(type=dtype)
        assert obj.metadata["deco_type"] == dtype

    def test_decoration_seasons(self):
        obj = create_decoration(type="tree", season="autumn")
        assert obj.metadata["season"] == "autumn"


class TestBackground:
    def test_default_background(self):
        obj = create_background()
        assert isinstance(obj, GraphicObject)
        assert obj.metadata["component"] == "background"

    @pytest.mark.parametrize("bgtype", [
        "sky", "forest", "dungeon", "town", "ocean", "mountain",
    ])
    def test_background_types(self, bgtype):
        obj = create_background(type=bgtype)
        assert obj.metadata["bg_type"] == bgtype

    def test_background_time(self):
        obj = create_background(time="night")
        assert obj.metadata["time"] == "night"

    def test_background_weather(self):
        obj = create_background(weather="rain")
        assert obj.metadata["weather"] == "rain"

    def test_background_dimensions(self):
        obj = create_background(width=512, height=384)
        assert obj.metadata["width"] == 512
        assert obj.metadata["height"] == 384


# ======================================================================
# Integration: Character with outfit + weapon
# ======================================================================


class TestCharacterWithEquipment:
    def test_character_with_outfit(self):
        outfit_obj = create_outfit(top="armor", bottom="armor")
        char = create_character(outfit=outfit_obj)
        assert isinstance(char, GraphicObject)
        assert char.metadata["component"] == "character"
        # Should have more children than base character (body+head+eyes+mouth+hair+outfit)
        assert len(char.children) >= 6

    def test_character_with_weapon(self):
        weapon_obj = create_weapon(type="sword", size="large")
        char = create_character(weapon=weapon_obj)
        assert isinstance(char, GraphicObject)
        assert len(char.children) >= 6  # base 5 + weapon

    def test_character_with_outfit_and_weapon(self):
        outfit_obj = create_outfit(top="hoodie", bottom="pants", shoes="sneakers")
        weapon_obj = create_weapon(type="staff", element="lightning")
        char = create_character(
            outfit=outfit_obj,
            weapon=weapon_obj,
            expression="happy",
        )
        assert isinstance(char, GraphicObject)
        # body + head + eyes + mouth + hair + outfit + weapon = 7
        assert len(char.children) >= 7


# ======================================================================
# Integration: Full PML evaluation
# ======================================================================


class TestPhase4PMLIntegration:
    """Test that Phase 4 components work through the PML interpreter."""

    def _eval(self, code: str):
        from pml.lexer import Lexer
        from pml.parser import Parser
        from pml.evaluator import evaluate
        from pml.repl import create_global_env
        env = create_global_env()
        tokens = Lexer(code).tokenize()
        ast = Parser(tokens).parse()
        result = None
        for expr in ast:
            result = evaluate(expr, env)
        return result

    def test_tile_pml(self):
        result = self._eval('(tile :type \'stone :size 48)')
        assert isinstance(result, GraphicObject)
        assert result.metadata["tile_type"] == "stone"

    def test_weapon_pml(self):
        result = self._eval("(weapon :type 'bow :size 'large)")
        assert isinstance(result, GraphicObject)
        assert result.metadata["type"] == "bow"

    def test_button_pml(self):
        result = self._eval("(button :label \"Start\" :width 120 :height 40)")
        assert isinstance(result, GraphicObject)
        assert result.metadata["component"] == "button"

    def test_decoration_pml(self):
        result = self._eval("(decoration :type 'tree :season 'winter :size 'large)")
        assert isinstance(result, GraphicObject)
        assert result.metadata["deco_type"] == "tree"

    def test_background_pml(self):
        result = self._eval("(background :type 'forest :time 'dusk :weather 'fog)")
        assert isinstance(result, GraphicObject)
        assert result.metadata["bg_type"] == "forest"

    def test_potion_pml(self):
        result = self._eval("(potion :type 'mana :container 'flask)")
        assert isinstance(result, GraphicObject)

    def test_icon_pml(self):
        result = self._eval("(icon :type 'heart :size 32)")
        assert isinstance(result, GraphicObject)

    def test_health_bar_pml(self):
        result = self._eval("(health-bar :value 0.7 :width 100 :height 12)")
        assert isinstance(result, GraphicObject)

    def test_panel_pml(self):
        result = self._eval('(panel :width 200 :height 150 :title "Info")')
        assert isinstance(result, GraphicObject)

    def test_outfit_pml(self):
        result = self._eval("(outfit :top 'armor :bottom 'armor :shoes 'boots)")
        assert isinstance(result, GraphicObject)

    def test_render_character_with_gear(self):
        """Render a full character with outfit and weapon to a file."""
        with tempfile.TemporaryDirectory() as tmpdir:
            outpath = Path(tmpdir, "hero_equipped.png").as_posix()
            code = f"""
(sprite-canvas 128 128)
(define my-outfit (outfit :top 'hoodie :bottom 'pants :shoes 'sneakers))
(define my-weapon (weapon :type 'sword :material 'steel))
(add (character :outfit my-outfit :weapon my-weapon :expression 'happy))
(render "{outpath}")
"""
            self._eval(code)
            assert os.path.exists(outpath)
            assert os.path.getsize(outpath) > 0
