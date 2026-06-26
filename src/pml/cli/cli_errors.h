#pragma once

// ==========================================================================================================================================================================================================================================═
// PML CLI — Error formatting utilities
//
// Ported from pml/repl.py error formatting logic.
// ==========================================================================================================================================================================================================================================═

#include "pml/core/error.h"

#include <nlohmann/json.hpp>

namespace pml {
void print_error(const PMLException& err);

/// Convert a structured RenderResult error JSON back to a printable message.
void print_render_error(const nlohmann::json& err);

nlohmann::json error_to_json(const PMLException& e);
nlohmann::json error_to_json(int code, const std::string& type,
                             const std::string& message);

void print_json_error(const PMLException& err);
}  // namespace pml
