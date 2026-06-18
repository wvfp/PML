// ═══════════════════════════════════════════════════════════════════════════════
// PML Component Parameter Validator — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "validator.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// ParamSchema builder
// ═══════════════════════════════════════════════════════════════════════════════

ParamSchema& ParamSchema::enum_(const std::string& name,
                                const std::vector<std::string>& values,
                                const std::string& default_value) {
    FieldSpec spec;
    spec.type = FieldSpec::Enum;
    spec.enum_values = values;
    spec.default_value = default_value;
    m_fields[name] = std::move(spec);
    return *this;
}

ParamSchema& ParamSchema::number(const std::string& name,
                                 double default_value,
                                 double min_val,
                                 double max_val) {
    FieldSpec spec;
    spec.type = FieldSpec::Number;
    spec.default_value = default_value;
    spec.min_value = min_val;
    spec.max_value = max_val;
    m_fields[name] = std::move(spec);
    return *this;
}

ParamSchema& ParamSchema::boolean(const std::string& name, bool default_value) {
    FieldSpec spec;
    spec.type = FieldSpec::Boolean;
    spec.default_value = default_value;
    m_fields[name] = std::move(spec);
    return *this;
}

ParamSchema& ParamSchema::color(const std::string& name,
                                const std::string& default_value) {
    FieldSpec spec;
    spec.type = FieldSpec::Color;
    spec.default_value = default_value;
    m_fields[name] = std::move(spec);
    return *this;
}

ParamSchema& ParamSchema::any_type(const std::string& name,
                                   const Value& default_value) {
    FieldSpec spec;
    spec.type = FieldSpec::Any;
    spec.default_value = default_value;
    m_fields[name] = std::move(spec);
    return *this;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

[[nodiscard]] std::string value_to_str(const Value& v) {
    if (const auto* s = std::get_if<std::string>(&v)) return *s;
    if (const auto* sym = std::get_if<Symbol>(&v)) return sym->name;
    if (const auto* kw = std::get_if<Keyword>(&v)) return kw->name;
    return value_to_string(v);
}

[[nodiscard]] std::optional<double> value_to_number_opt(const Value& v) {
    if (const auto* i = std::get_if<int64_t>(&v)) {
        return static_cast<double>(*i);
    }
    if (const auto* d = std::get_if<double>(&v)) {
        return *d;
    }
    return std::nullopt;
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// validate_params
// ═══════════════════════════════════════════════════════════════════════════════

std::unordered_map<std::string, Value> validate_params(
    const ParamSchema& schema,
    const std::unordered_map<std::string, Value>& kwargs) {
    std::unordered_map<std::string, Value> result;

    for (const auto& [field_name, spec] : schema.fields()) {
        auto it = kwargs.find(field_name);
        if (it == kwargs.end() || std::holds_alternative<std::nullptr_t>(it->second)) {
            result[field_name] = spec.default_value;
            continue;
        }

        const Value& value = it->second;

        switch (spec.type) {
            case FieldSpec::Enum: {
                std::string s = value_to_str(value);
                const auto& allowed = spec.enum_values.value();
                bool found = std::find(allowed.begin(), allowed.end(), s) != allowed.end();
                if (found) {
                    result[field_name] = s;
                } else {
                    std::cerr << "Warning: invalid value '" << s << "' for '"
                              << field_name << "', falling back to '"
                              << value_to_str(spec.default_value) << "'\n";
                    result[field_name] = spec.default_value;
                }
                break;
            }
            case FieldSpec::Number: {
                if (auto n = value_to_number_opt(value)) {
                    double clamped = std::clamp(*n, spec.min_value, spec.max_value);
                    result[field_name] = clamped;
                } else {
                    result[field_name] = spec.default_value;
                }
                break;
            }
            case FieldSpec::Boolean: {
                if (const auto* b = std::get_if<bool>(&value)) {
                    result[field_name] = *b;
                } else {
                    result[field_name] = spec.default_value;
                }
                break;
            }
            case FieldSpec::Color: {
                if (std::holds_alternative<std::nullptr_t>(value)) {
                    result[field_name] = spec.default_value;
                } else {
                    result[field_name] = value_to_str(value);
                }
                break;
            }
            case FieldSpec::Any:
                result[field_name] = value;
                break;
        }
    }

    // Pass through unknown kwargs.
    for (const auto& [k, v] : kwargs) {
        if (result.find(k) == result.end()) {
            result[k] = v;
        }
    }

    return result;
}

}  // namespace pml
