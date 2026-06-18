#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML UI Widget Components
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/components/ui_widgets.py.
// Provides button, panel, health-bar, and icon factory functions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/graphics/objects.h"
#include "pml/sprites/validator.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pml {

/// Schema for the button component.
ParamSchema button_schema();

/// Create a button graphic object.
std::shared_ptr<GraphicObject> create_button(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the panel component.
ParamSchema panel_schema();

/// Create a panel graphic object.
std::shared_ptr<GraphicObject> create_panel(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the health-bar component.
ParamSchema health_bar_schema();

/// Create a health-bar graphic object.
std::shared_ptr<GraphicObject> create_health_bar(
    const std::unordered_map<std::string, Value>& kwargs);

/// Schema for the icon component.
ParamSchema icon_schema();

/// Create an icon graphic object.
std::shared_ptr<GraphicObject> create_icon(
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
