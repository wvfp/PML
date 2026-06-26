// ==========================================================================================================================================================================================================================================═
// Params implementation
// ==========================================================================================================================================================================================================================================═

#include "params.h"

#include <algorithm>
#include <bit>
#include <cstring>

namespace pml {

namespace {

struct KeyEntry {
    std::string_view name;
    ParamKey key;
};

constexpr KeyEntry k_key_table[] = {
    {"x",         ParamKey::x},
    {"y",         ParamKey::y},
    {"w",         ParamKey::w},
    {"h",         ParamKey::h},
    {"cx",        ParamKey::cx},
    {"cy",        ParamKey::cy},
    {"r",         ParamKey::r},
    {"rx",        ParamKey::rx},
    {"ry",        ParamKey::ry},
    {"x1",        ParamKey::x1},
    {"y1",        ParamKey::y1},
    {"x2",        ParamKey::x2},
    {"y2",        ParamKey::y2},
    {"text",      ParamKey::text},
    {"font-size", ParamKey::font_size},
    {"font_size", ParamKey::font_size},
    {"d",         ParamKey::d},
    {"src",       ParamKey::src},
    {"points",    ParamKey::points},
    {"shader",    ParamKey::shader},
    {"crop",      ParamKey::crop},
    {"mesh",           ParamKey::mesh},
    {"transform",      ParamKey::transform},
    {"path_commands",  ParamKey::path_commands},
    // Texture-mapping keys
    {"uv",             ParamKey::uv},
    {"uv-mode",        ParamKey::uv_mode},
    {"wrap-x",         ParamKey::wrap_x},
    {"wrap-y",         ParamKey::wrap_y},
    {"filter",         ParamKey::filter},
    {"edge-blend",     ParamKey::edge_blend},
    {"uv-vertices",    ParamKey::uv_vertices},
};

}  // anonymous namespace

ParamKey param_key_from_string(std::string_view sv) noexcept {
    for (const auto& e : k_key_table) {
        if (e.name == sv) return e.key;
    }
    return ParamKey::unknown;
}

const char* param_key_to_string(ParamKey key) noexcept {
    switch (key) {
        case ParamKey::x:         return "x";
        case ParamKey::y:         return "y";
        case ParamKey::w:         return "w";
        case ParamKey::h:         return "h";
        case ParamKey::cx:        return "cx";
        case ParamKey::cy:        return "cy";
        case ParamKey::r:         return "r";
        case ParamKey::rx:        return "rx";
        case ParamKey::ry:        return "ry";
        case ParamKey::x1:        return "x1";
        case ParamKey::y1:        return "y1";
        case ParamKey::x2:        return "x2";
        case ParamKey::y2:        return "y2";
        case ParamKey::text:      return "text";
        case ParamKey::font_size: return "font-size";
        case ParamKey::d:         return "d";
        case ParamKey::src:       return "src";
        case ParamKey::points:    return "points";
        case ParamKey::shader:    return "shader";
        case ParamKey::crop:      return "crop";
        case ParamKey::mesh:           return "mesh";
        case ParamKey::transform:      return "transform";
        case ParamKey::path_commands:  return "path_commands";
        case ParamKey::uv:             return "uv";
        case ParamKey::uv_mode:        return "uv-mode";
        case ParamKey::wrap_x:         return "wrap-x";
        case ParamKey::wrap_y:         return "wrap-y";
        case ParamKey::filter:         return "filter";
        case ParamKey::edge_blend:     return "edge-blend";
        case ParamKey::uv_vertices:    return "uv-vertices";
        default:                       return "";
    }
}

Params::Params(std::initializer_list<std::pair<const char*, Value>> init) {
    for (const auto& [k, v] : init) {
        set(k, Value(v));
    }
}

Params::Params(std::initializer_list<std::pair<ParamKey, Value>> init) {
    for (const auto& [k, v] : init) {
        set(k, Value(v));
    }
}

void Params::set(ParamKey key, Value value) {
    if (key == ParamKey::unknown || key == ParamKey::count) return;
    values_[index(key)] = std::move(value);
    set_bit(key);
}

void Params::set(const std::string& key, Value value) {
    set(param_key_from_string(key), std::move(value));
}

bool Params::contains(ParamKey key) const noexcept {
    return bit(key);
}

bool Params::contains(const std::string& key) const noexcept {
    return contains(param_key_from_string(key));
}

const Value* Params::find(ParamKey key) const noexcept {
    if (!bit(key)) return nullptr;
    return &values_[index(key)];
}

const Value* Params::find(const std::string& key) const noexcept {
    return find(param_key_from_string(key));
}

Value& Params::operator[](ParamKey key) {
    if (key == ParamKey::unknown || key == ParamKey::count) {
        return values_[0];  // dummy slot
    }
    set_bit(key);
    return values_[index(key)];
}

Value& Params::operator[](const std::string& key) {
    return (*this)[param_key_from_string(key)];
}

size_t Params::size() const noexcept {
    return std::popcount(present_);
}

bool Params::empty() const noexcept {
    return present_ == 0;
}

bool Params::operator==(const Params& other) const noexcept {
    if (present_ != other.present_) return false;
    for (size_t i = 0; i < N; ++i) {
        if (((present_ >> i) & 1) && values_[i] != other.values_[i]) {
            return false;
        }
    }
    return true;
}

}  // namespace pml
