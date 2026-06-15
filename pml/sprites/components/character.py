"""Character assembler — combines body, head, eyes, mouth, hair into a complete sprite."""

from __future__ import annotations

from typing import Any

from pml.graphics.objects import GraphicObject
from pml.sprites.palette import Palette, get_palette, set_active_palette
from pml.sprites.style import StyleDescriptor, resolve_style
from pml.sprites.validator import ParamSchema, validate_params
from pml.transform import AffineTransform
from pml.types import Symbol

_CHARACTER_SCHEMA = (
    ParamSchema()
    .any_type("body", None)
    .any_type("head", None)
    .any_type("hair", None)
    .any_type("outfit", None)
    .any_type("accessory", None)
    .any_type("weapon", None)
    .enum("pose", ["standing", "walking", "running", "sitting", "action"], "standing")
    .enum("direction", ["front", "back", "left", "right", "front-left", "front-right"], "front")
    .enum("expression", ["neutral", "happy", "angry", "sad", "surprised"], "neutral")
    .any_type("style", None)
    .any_type("palette", None)
    .number("scale", 1.0, min_val=0.1, max_val=10.0)
)


def _sym_str(v: Any) -> str:
    if isinstance(v, Symbol):
        return v.name
    return str(v) if v is not None else ""


def _create_default_body(skin: str) -> GraphicObject:
    from pml.sprites.components.body import create_body
    return create_body(skin=skin, build="average", proportions="anime")


def _create_default_head(skin: str) -> GraphicObject:
    from pml.sprites.components.head import create_head
    return create_head(shape="oval", skin=skin, ears="normal")


def _create_default_eyes(expression: str) -> GraphicObject:
    from pml.sprites.components.eyes import create_eyes
    style_map = {
        "neutral": "shoujo",
        "happy": "round",
        "angry": "sharp",
        "sad": "sleepy",
        "surprised": "round",
    }
    eye_style = style_map.get(expression, "shoujo")
    return create_eyes(style=eye_style, color="#4a90d9", size=1.0)


def _create_default_mouth(expression: str) -> GraphicObject:
    from pml.sprites.components.mouth import create_mouth
    style_map = {
        "neutral": "neutral",
        "happy": "smile",
        "angry": "frown",
        "sad": "frown",
        "surprised": "open",
    }
    mouth_style = style_map.get(expression, "neutral")
    return create_mouth(style=mouth_style, size=1.0)


def _create_default_hair() -> GraphicObject:
    from pml.sprites.components.hair import create_hair
    return create_hair(style="medium", color="#2c2c2c")


def _apply_style_to_object(obj: GraphicObject, style: StyleDescriptor) -> GraphicObject:
    """Apply style parameters (outline width/color) to a GraphicObject tree."""
    if style.outline_style == "none":
        return obj

    if obj.shape_type == "group":
        styled_children = tuple(
            _apply_style_to_object(child, style) for child in obj.children
        )
        return GraphicObject(
            shape_type="group",
            params=dict(obj.params),
            fill=obj.fill,
            stroke=obj.stroke,
            stroke_width=obj.stroke_width,
            transform=obj.transform,
            children=styled_children,
            metadata=dict(obj.metadata),
        )

    # Apply style outline to shapes with existing strokes
    new_stroke = obj.stroke
    new_stroke_width = obj.stroke_width

    if obj.stroke and obj.stroke != "#FFFFFF":
        new_stroke = style.outline_color
        new_stroke_width = style.outline_width

    return GraphicObject(
        shape_type=obj.shape_type,
        params=dict(obj.params),
        fill=obj.fill,
        stroke=new_stroke,
        stroke_width=new_stroke_width,
        transform=obj.transform,
        children=obj.children,
        metadata=dict(obj.metadata),
    )


def create_character(**kwargs: Any) -> GraphicObject:
    """Assemble a complete character sprite from components.

    Args:
        :body — body GraphicObject (or None for default)
        :head — head GraphicObject (or None for default)
        :hair — hair GraphicObject (or None for default)
        :outfit — outfit GraphicObject (optional)
        :pose — 'standing | 'walking | ...
        :direction — 'front | 'back | ...
        :expression — 'neutral | 'happy | ...
        :style — style reference (Symbol, str, or StyleDescriptor)
        :palette — palette name (str)
        :scale — global scale factor
    """
    p = validate_params(_CHARACTER_SCHEMA, {_sym_str(k): v for k, v in kwargs.items()})

    expression = p["expression"]
    scale = p["scale"]
    style_ref = p.get("style")
    palette_ref = p.get("palette")

    # Resolve style
    style = resolve_style(style_ref) if style_ref else None

    # Resolve palette
    palette = None
    if palette_ref:
        palette_name = palette_ref.name if isinstance(palette_ref, Symbol) else str(palette_ref)
        palette = get_palette(palette_name)
        set_active_palette(palette)

    skin = palette.get("skin") if palette else "#fce4c8"

    # Get or create components
    body_obj = p.get("body")
    if not isinstance(body_obj, GraphicObject):
        body_obj = _create_default_body(skin)

    head_obj = p.get("head")
    if not isinstance(head_obj, GraphicObject):
        head_obj = _create_default_head(skin)

    hair_obj = p.get("hair")
    if not isinstance(hair_obj, GraphicObject):
        hair_obj = _create_default_hair()

    # Eyes and mouth — use defaults based on expression if not provided
    eyes_obj = _create_default_eyes(expression)
    mouth_obj = _create_default_mouth(expression)

    # Layout in screen coordinates (y increases downward)
    # The character is centered at (0, 0) with head above and body below
    head_w = head_obj.metadata.get("head_width", 56)
    head_h = head_obj.metadata.get("head_height", 64)
    body_h = body_obj.metadata.get("torso_height", 64)

    # Vertical layout (screen coords, y-down):
    #   head center at y = -body_h/2 - head_h/2  (above body)
    #   body starts at y = 0, extends to y = body_h  (below origin)
    head_cy = -(body_h * 0.3 + head_h * 0.5)  # head center y
    body_top = 0  # body top y

    # Position transforms for each component
    positioned: list[GraphicObject] = []

    # Body: rect goes from (body_top) downward
    positioned.append(body_obj.with_transform(
        AffineTransform.translate(0, body_top)
    ))

    # Head centered at head_cy
    positioned.append(head_obj.with_transform(
        AffineTransform.translate(0, head_cy)
    ))

    # Eyes on face (slightly above head center for anime style)
    eye_y = head_cy + 2
    positioned.append(eyes_obj.with_transform(
        AffineTransform.translate(0, eye_y)
    ))

    # Mouth below eyes (about 25% down from head center)
    mouth_y = head_cy + head_h * 0.22
    positioned.append(mouth_obj.with_transform(
        AffineTransform.translate(0, mouth_y)
    ))

    # Hair on top of head (same center as head)
    positioned.append(hair_obj.with_transform(
        AffineTransform.translate(0, head_cy)
    ))

    # Outfit overlay (if provided) — positioned over body area
    outfit_obj = p.get("outfit")
    if isinstance(outfit_obj, GraphicObject):
        positioned.append(outfit_obj.with_transform(
            AffineTransform.translate(0, body_top)
        ))

    # Weapon (if provided) — placed to the character's right side
    weapon_obj = p.get("weapon")
    if isinstance(weapon_obj, GraphicObject):
        weapon_x = head_w * 0.6 + 10
        weapon_y = body_top + body_h * 0.1
        positioned.append(weapon_obj.with_transform(
            AffineTransform.translate(weapon_x, weapon_y)
        ))

    # Apply global scale
    if scale != 1.0:
        scale_t = AffineTransform.scale(scale, scale)
        positioned = [obj.with_transform(scale_t.compose(obj.transform)) for obj in positioned]

    # Apply style if provided
    if style:
        positioned = [_apply_style_to_object(obj, style) for obj in positioned]

    # Reset active palette
    set_active_palette(None)

    return GraphicObject(
        shape_type="group",
        children=tuple(positioned),
        metadata={
            "component": "character",
            "pose": p.get("pose", "standing"),
            "direction": p.get("direction", "front"),
            "expression": expression,
            "scale": scale,
        },
    )
