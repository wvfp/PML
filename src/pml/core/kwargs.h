#pragma once

// ==========================================================================================================================================================================================================================================═
// PML keyword-argument parsing helpers
// ==========================================================================================================================================================================================================================================═
//
// Reusable utilities for builtins that accept :keyword (or symbol) key/value
// pairs after their positional arguments.
//
// Usage:
//   auto kwargs = parse_kwargs(args, positional_count);
//   std::string fill = kw_string(kwargs, "fill", "black");
//   int width        = kw_int(kwargs, "width", 100);
//
// ==========================================================================================================================================================================================================================================═

#include "pml/core/error.h"
#include "pml/core/types.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace pml::kwargs {

/// Convert a Value to an optional string.  Accepts string, symbol, keyword,
/// int64_t, and double alternatives (numbers are converted with std::to_string).
[[nodiscard]] inline std::optional<std::string> value_to_opt_string(
    const Value& v)
{
    if (const auto* s = v.as_string()) return *s;
    if (const auto* sym = v.as_symbol()) return sym->name;
    if (const auto* kw = v.as_keyword()) return kw->name;
    if (v.is_int()) return std::to_string(v.int_val());
    if (v.is_double()) return std::to_string(v.double_val());
    return std::nullopt;
}

/// Convert a Value to a string, returning an error if it cannot be represented
/// as a string.
[[nodiscard]] inline Result<std::string> value_to_string_req(const Value& v)
{
    auto s = value_to_opt_string(v);
    if (s.has_value()) return std::move(*s);
    return std::unexpected(
        type_error("expected a string, got " + value_to_string(v)));
}

/// Parse keyword arguments from `args` starting at index `start`.
/// Key/value pairs may use either Keyword (`:fill`) or Symbol (`fill`) keys.
[[nodiscard]] inline std::unordered_map<std::string, Value> parse_kwargs(
    const std::vector<Value>& args, size_t start)
{
    std::unordered_map<std::string, Value> result;
    for (size_t i = start; i + 1 < args.size(); i += 2) {
        if (const auto* kw = args[i].as_keyword()) {
            result[kw->name] = args[i + 1];
        } else if (const auto* sym = args[i].as_symbol()) {
            result[sym->name] = args[i + 1];
        } else {
            break;
        }
    }
    return result;
}

/// Extract a string keyword argument, falling back to `default_val`.
[[nodiscard]] inline std::string kw_string(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    const std::string& default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    return value_to_opt_string(it->second).value_or(default_val);
}

/// Extract an integer keyword argument, falling back to `default_val`.
[[nodiscard]] inline int kw_int(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    int default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    if (it->second.is_int()) {
        return static_cast<int>(it->second.int_val());
    }
    return default_val;
}

/// Extract a floating-point keyword argument, falling back to `default_val`.
[[nodiscard]] inline double kw_double(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    double default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    if (it->second.is_double()) {
        return it->second.double_val();
    }
    if (it->second.is_int()) {
        return static_cast<double>(it->second.int_val());
    }
    return default_val;
}

}  // namespace pml::kwargs
