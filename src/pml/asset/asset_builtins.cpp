// ═══════════════════════════════════════════════════════════════════════════════
// Asset / Bitmap I/O builtins — image loading, bitmap layers, spritesheets
// ═══════════════════════════════════════════════════════════════════════════════

#include "asset_builtins.h"
#include "asset_cache.h"
#include "asset_loader.h"

#include "pml/api/context.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/core/types.h"
#include "pml/evaluator/builtins.h"
#include "pml/graphics/objects.h"
#include "pml/layer/blend_mode.h"
#include "pml/layer/layer.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace pml {

namespace fs = std::filesystem;
namespace kw = pml::kwargs;
using kw::value_to_opt_string;

// ═══════════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════════

static std::string current_source_file(Environment& env)
{
    auto v = env.try_lookup("__source_file__");
    if (v && v->is_string()) {
        return *v->as_string();
    }
    return "";
}

static Value make_image_object(const std::string& path,
                               double x, double y,
                               double w, double h,
                               const std::optional<Value>& crop)
{
    Params params{
        {ParamKey::src, Value(path)},
        {ParamKey::x,   Value(x)},
        {ParamKey::y,   Value(y)},
    };
    if (w > 0.0) params.set(ParamKey::w, Value(w));
    if (h > 0.0) params.set(ParamKey::h, Value(h));
    if (crop)    params.set(ParamKey::crop, *crop);

    return Value(std::make_shared<GraphicObject>(
        GraphicObject("image", std::move(params))));
}

static std::shared_ptr<ValueList> make_value_list(
    std::vector<Value> elements)
{
    return std::make_shared<ValueList>(std::move(elements));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Builtins
// ═══════════════════════════════════════════════════════════════════════════════

static Result<Value> builtin_image(const std::vector<Value>& args,
                                   Environment& env,
                                   SourceLocation loc)
{
    (void)loc;
    if (args.empty()) {
        return std::unexpected(type_error("image: path required"));
    }
    auto path = value_to_opt_string(args[0]);
    if (!path) {
        return std::unexpected(type_error("image: path must be a string"));
    }

    auto resolved = resolve_asset_path(*path, current_source_file(env));
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    auto kwargs = kw::parse_kwargs(args, 1);
    double x = kw::kw_double(kwargs, "x", 0.0);
    double y = kw::kw_double(kwargs, "y", 0.0);
    double w = kw::kw_double(kwargs, "width", 0.0);
    double h = kw::kw_double(kwargs, "height", 0.0);

    std::optional<Value> crop;
    if (auto it = kwargs.find("crop"); it != kwargs.end()) {
        crop = it->second;
    }

    auto result = make_image_object(*resolved, x, y, w, h, crop);

    // Apply :blend-mode and :stroke-align from kwargs
    if (auto* go = result.as_graphic_object()) {
        if (auto it = kwargs.find("blend-mode"); it != kwargs.end()) {
            if (auto bm_str = kw::value_to_opt_string(it->second)) {
                if (auto bm = blend_mode_from_keyword(*bm_str)) {
                    (*go)->blend_mode = *bm;
                }
            }
        }
        if (auto it = kwargs.find("stroke-align"); it != kwargs.end()) {
            if (auto sa = kw::value_to_opt_string(it->second)) {
                if (*sa == "inside" || *sa == "outside") {
                    (*go)->stroke_align = *sa;
                }
            }
        }
    }

    return result;
}

static Result<Value> builtin_load_image(const std::vector<Value>& args,
                                        Environment& env,
                                        SourceLocation loc)
{
    return builtin_image(args, env, loc);
}

static Result<Value> builtin_bitmap_layer(const std::vector<Value>& args,
                                          Environment& env,
                                          SourceLocation loc)
{
    (void)loc;
    if (args.empty()) {
        return std::unexpected(type_error( "bitmap-layer: path required"));
    }
    auto path = value_to_opt_string(args[0]);
    if (!path) {
        return std::unexpected(type_error( "bitmap-layer: path must be a string"));
    }

    auto resolved = resolve_asset_path(*path, current_source_file(env));
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    auto kwargs = kw::parse_kwargs(args, 1);
    std::string name = kw::kw_string(kwargs, "name", fs::path(*resolved).stem().string());
    double x = kw::kw_double(kwargs, "x", 0.0);
    double y = kw::kw_double(kwargs, "y", 0.0);
    double w = kw::kw_double(kwargs, "width", 0.0);
    double h = kw::kw_double(kwargs, "height", 0.0);

    std::optional<Value> crop;
    if (auto it = kwargs.find("crop"); it != kwargs.end()) {
        crop = it->second;
    }

    LayerProperties props;
    props.name = std::move(name);
    props.offset = {x, y};

    auto leaf = std::make_shared<GraphicObject>(
        GraphicObject("image", Params{
            {ParamKey::src, Value(*resolved)},
            {ParamKey::x,   Value(0.0)},
            {ParamKey::y,   Value(0.0)},
        }));
    if (w > 0.0) leaf->params.set(ParamKey::w, Value(w));
    if (h > 0.0) leaf->params.set(ParamKey::h, Value(h));
    if (crop)    leaf->params.set(ParamKey::crop, *crop);

    return Value(std::make_shared<Layer>(Layer(props, leaf)));
}

static Result<Value> builtin_asset_path(const std::vector<Value>& args,
                                        Environment& env,
                                        SourceLocation loc)
{
    (void)loc;
    if (args.empty()) {
        return std::unexpected(type_error( "asset-path?: path required"));
    }
    auto path = value_to_opt_string(args[0]);
    if (!path) {
        return std::unexpected(type_error( "asset-path?: path must be a string"));
    }

    auto resolved = resolve_asset_path(*path, current_source_file(env));
    return Value(resolved.has_value());
}

static Result<Value> builtin_clear_assets(const std::vector<Value>& /*args*/,
                                          Environment& /*env*/,
                                          SourceLocation /*loc*/)
{
    PMLContext::current().assets->clear();
    return Value(nullptr);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Spritesheet import (Aseprite / TexturePacker JSON)
// ═══════════════════════════════════════════════════════════════════════════════

static Result<nlohmann::json> read_json_file(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return std::unexpected(resource_error(
            std::format("cannot open JSON file: {}", path)));
    }
    try {
        nlohmann::json j;
        ifs >> j;
        return j;
    } catch (const std::exception& e) {
        return std::unexpected(resource_error(
            std::format("JSON parse error in {}: {}", path, e.what())));
    }
}

static Result<Value> builtin_load_spritesheet(const std::vector<Value>& args,
                                              Environment& env,
                                              SourceLocation loc)
{
    (void)loc;
    if (args.size() < 2) {
        return std::unexpected(type_error(
            "load-spritesheet: image path and JSON path required"));
    }

    auto image_path = value_to_opt_string(args[0]);
    auto json_path  = value_to_opt_string(args[1]);
    if (!image_path || !json_path) {
        return std::unexpected(type_error(
            "load-spritesheet: both paths must be strings"));
    }

    std::string from_file = current_source_file(env);
    auto resolved_json = resolve_asset_path(*json_path, from_file);
    if (!resolved_json) {
        return std::unexpected(resolved_json.error());
    }

    auto resolved_image = resolve_asset_path(*image_path, from_file);
    if (!resolved_image) {
        return std::unexpected(resolved_image.error());
    }

    auto json_result = read_json_file(*resolved_json);
    if (!json_result) {
        return std::unexpected(json_result.error());
    }
    const auto& json = *json_result;

    std::vector<Value> frames;
    if (json.contains("frames")) {
        const auto& frames_node = json["frames"];
        if (frames_node.is_array()) {
            for (const auto& f : frames_node) {
                const auto& frame = f.value("frame", nlohmann::json::object());
                int x = frame.value("x", 0);
                int y = frame.value("y", 0);
                int w = frame.value("w", 0);
                int h = frame.value("h", 0);
                if (w <= 0 || h <= 0) continue;

                std::vector<Value> crop{
                    Value(static_cast<int64_t>(x)),
                    Value(static_cast<int64_t>(y)),
                    Value(static_cast<int64_t>(w)),
                    Value(static_cast<int64_t>(h)),
                };
                frames.push_back(make_image_object(*resolved_image, 0, 0, 0, 0,
                    Value(make_value_list(std::move(crop)))));
            }
        } else if (frames_node.is_object()) {
            for (auto it = frames_node.begin(); it != frames_node.end(); ++it) {
                const auto& frame = it.value().value("frame", nlohmann::json::object());
                int x = frame.value("x", 0);
                int y = frame.value("y", 0);
                int w = frame.value("w", 0);
                int h = frame.value("h", 0);
                if (w <= 0 || h <= 0) continue;

                std::vector<Value> crop{
                    Value(static_cast<int64_t>(x)),
                    Value(static_cast<int64_t>(y)),
                    Value(static_cast<int64_t>(w)),
                    Value(static_cast<int64_t>(h)),
                };
                frames.push_back(make_image_object(*resolved_image, 0, 0, 0, 0,
                    Value(make_value_list(std::move(crop)))));
            }
        }
    }

    return Value(make_value_list(std::move(frames)));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_asset_builtins(std::shared_ptr<Environment> env)
{
    env->define("image", Value(std::make_shared<BuiltinProcedure>(
        "image", builtin_image, true)));
    env->define("load-image", Value(std::make_shared<BuiltinProcedure>(
        "load-image", builtin_load_image, true)));
    env->define("bitmap-layer", Value(std::make_shared<BuiltinProcedure>(
        "bitmap-layer", builtin_bitmap_layer, true)));
    env->define("load-spritesheet", Value(std::make_shared<BuiltinProcedure>(
        "load-spritesheet", builtin_load_spritesheet, true)));
    env->define("asset-path?", Value(std::make_shared<BuiltinProcedure>(
        "asset-path?", builtin_asset_path, true)));
    env->define("clear-assets!", Value(std::make_shared<BuiltinProcedure>(
        "clear-assets!", builtin_clear_assets, true)));
}

} // namespace pml
