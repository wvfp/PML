#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Component Parameter Validator
// ───────────────────────────────────────────────────────────────────────────────
// Ported from pml/sprites/validator.py.
// Provides a fluent ParamSchema builder and validate_params() that cleans
// component keyword arguments: enums fall back to defaults, numbers are clamped,
// missing keys use defaults, unknown keys pass through.
// ═══════════════════════════════════════════════════════════════════════════════

#include "types.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// FieldSpec — one schema field
// ═══════════════════════════════════════════════════════════════════════════════

/// Runtime field specification used by ParamSchema.
struct FieldSpec {
    enum Type { Enum, Number, Boolean, Color, Any };

    Type type;
    std::optional<std::vector<std::string>> enum_values;
    Value default_value;
    double min_value{-1e9};
    double max_value{1e9};
};

// ═══════════════════════════════════════════════════════════════════════════════
// ParamSchema — fluent schema builder
// ═══════════════════════════════════════════════════════════════════════════════

/// Fluent schema builder for sprite component parameters.
class ParamSchema {
public:
    /// Register an enum parameter with a default fallback.
    ParamSchema& enum_(const std::string& name,
                       const std::vector<std::string>& values,
                       const std::string& default_value);

    /// Register a numeric parameter with a clamp range.
    ParamSchema& number(const std::string& name,
                        double default_value,
                        double min_val = -1e9,
                        double max_val = 1e9);

    /// Register a boolean parameter.
    ParamSchema& boolean(const std::string& name, bool default_value = false);

    /// Register a color parameter.
    ParamSchema& color(const std::string& name,
                       const std::string& default_value = "#808080");

    /// Register a parameter of any type (passed through unchanged).
    ParamSchema& any_type(const std::string& name,
                          const Value& default_value = nullptr);

    /// Access the underlying field map.
    [[nodiscard]] const std::unordered_map<std::string, FieldSpec>& fields() const {
        return m_fields;
    }

private:
    std::unordered_map<std::string, FieldSpec> m_fields;
};

// ═══════════════════════════════════════════════════════════════════════════════
// validate_params — clean kwargs against a schema
// ═══════════════════════════════════════════════════════════════════════════════

/// Validate and clean keyword arguments against a schema.
///
/// - Enum values: fallback to default if not in allowed set.
/// - Numeric values: clamp to [min, max].
/// - Missing values: use defaults.
/// - Unknown values: passed through unchanged.
std::unordered_map<std::string, Value> validate_params(
    const ParamSchema& schema,
    const std::unordered_map<std::string, Value>& kwargs);

}  // namespace pml
