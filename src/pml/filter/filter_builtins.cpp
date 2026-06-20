// ═══════════════════════════════════════════════════════════════════════════════
// Filter builtins — PML-facing constructors for image filters and filter chains
// ═══════════════════════════════════════════════════════════════════════════════

#include "filter_builtins.h"

#include "adjustments.h"
#include "convolution.h"
#include "image_filter.h"
#include "layer_style.h"

#include "pml/backend/color_helpers.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/evaluator/builtins.h"
#include "pml/evaluator/environment.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pml {

namespace {

using namespace pml::kwargs;

static double to_double_value(const Value& v) {
    if (v.is_double()) return v.double_val();
    if (v.is_int()) return static_cast<double>(v.int_val());
    return 0.0;
}

static std::optional<uint32_t> parse_color_kw(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    std::optional<uint32_t> default_val = std::nullopt)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    auto s = value_to_opt_string(it->second);
    if (!s) return default_val;
    return parse_color(*s);
}

static bool kw_bool(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    bool default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    if (it->second.is_bool()) return it->second.bool_val();
    return default_val;
}

static std::vector<float> value_list_to_floats(const Value& v) {
    std::vector<float> result;
    const auto* lst = v.as_list();
    if (!lst || !*lst) return result;
    for (const auto& elem : (*lst)->elements) {
        if (elem.is_number()) {
            result.push_back(static_cast<float>(to_double_value(elem)));
        }
    }
    return result;
}

static std::vector<std::pair<uint8_t,uint8_t>> value_list_to_curve_points(const Value& v) {
    std::vector<std::pair<uint8_t,uint8_t>> result;
    const auto* lst = v.as_list();
    if (!lst || !*lst) return result;
    for (const auto& elem : (*lst)->elements) {
        const auto* pair = elem.as_list();
        if (!pair || !*pair || (*pair)->elements.size() < 2) continue;
        int x = static_cast<int>(to_double_value((*pair)->elements[0]));
        int y = static_cast<int>(to_double_value((*pair)->elements[1]));
        result.emplace_back(
            static_cast<uint8_t>(std::clamp(x, 0, 255)),
            static_cast<uint8_t>(std::clamp(y, 0, 255)));
    }
    return result;
}

static Result<Value> builtin_color_adjust(const std::vector<Value>& args,
                                          Environment& /*env*/,
                                          SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    ColorAdjustParams p;
    p.brightness  = kw_double(kwargs, "brightness",  0.0);
    p.contrast    = kw_double(kwargs, "contrast",    1.0);
    p.saturation  = kw_double(kwargs, "saturation",  1.0);
    p.hue         = kw_double(kwargs, "hue",         0.0);
    p.exposure    = kw_double(kwargs, "exposure",    0.0);
    p.gamma       = kw_double(kwargs, "gamma",       1.0);
    p.temperature = kw_double(kwargs, "temperature", 0.0);
    p.tint        = kw_double(kwargs, "tint",        0.0);
    p.grayscale   = kw_bool(kwargs, "grayscale", false);
    p.sepia       = kw_bool(kwargs, "sepia",     false);
    p.invert      = kw_bool(kwargs, "invert",    false);
    return Value(std::make_shared<ColorMatrixFilter>(p));
}

static Result<Value> builtin_levels(const std::vector<Value>& args,
                                    Environment& /*env*/,
                                    SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    double in_low  = kw_double(kwargs, "in-low",  0.0);
    double gamma   = kw_double(kwargs, "gamma",   1.0);
    double in_high = kw_double(kwargs, "in-high", 255.0);
    double out_low = kw_double(kwargs, "out-low", 0.0);
    double out_high= kw_double(kwargs, "out-high",255.0);
    return Value(std::make_shared<LevelsFilter>(in_low, gamma, in_high, out_low, out_high));
}

static int channel_keyword_to_index(const std::string& s) {
    if (s == "r" || s == "red")   return 0;
    if (s == "g" || s == "green") return 1;
    if (s == "b" || s == "blue")  return 2;
    return 3; // rgb / all
}

static Result<Value> builtin_curves(const std::vector<Value>& args,
                                    Environment& /*env*/,
                                    SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    int channel = channel_keyword_to_index(kw_string(kwargs, "channel", "rgb"));
    auto points = value_list_to_curve_points(kwargs["points"]);
    if (points.empty()) {
        points.emplace_back(0, 0);
        points.emplace_back(255, 255);
    }
    return Value(std::make_shared<CurvesFilter>(channel, std::move(points)));
}

static Result<Value> builtin_threshold(const std::vector<Value>& args,
                                       Environment& /*env*/,
                                       SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    int value = kw_int(kwargs, "value", 128);
    return Value(std::make_shared<ThresholdFilter>(
        static_cast<uint8_t>(std::clamp(value, 0, 255))));
}

static Result<Value> builtin_posterize(const std::vector<Value>& args,
                                       Environment& /*env*/,
                                       SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    int levels = kw_int(kwargs, "levels", 4);
    return Value(std::make_shared<PosterizeFilter>(std::max(levels, 2)));
}

static BlurFilterType blur_type_from_string(const std::string& s) {
    if (s == "box")    return BlurFilterType::Box;
    if (s == "motion") return BlurFilterType::Motion;
    return BlurFilterType::Gaussian;
}

static Result<Value> builtin_blur(const std::vector<Value>& args,
                                  Environment& /*env*/,
                                  SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float radius = static_cast<float>(kw_double(kwargs, "radius", 1.0));
    std::string type = kw_string(kwargs, "type", "gaussian");
    float angle = static_cast<float>(kw_double(kwargs, "angle", 0.0));
    return Value(std::make_shared<BlurFilter>(
        radius, blur_type_from_string(type), angle));
}

static Result<Value> builtin_sharpen(const std::vector<Value>& args,
                                     Environment& /*env*/,
                                     SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float amount = static_cast<float>(kw_double(kwargs, "amount", 1.0));
    float radius = static_cast<float>(kw_double(kwargs, "radius", 1.0));
    return Value(std::make_shared<SharpenFilter>(amount, radius));
}

static EdgeDetectType edge_type_from_string(const std::string& s) {
    if (s == "laplacian") return EdgeDetectType::Laplacian;
    return EdgeDetectType::Sobel;
}

static Result<Value> builtin_edge_detect(const std::vector<Value>& args,
                                         Environment& /*env*/,
                                         SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    std::string type = kw_string(kwargs, "type", "sobel");
    return Value(std::make_shared<EdgeDetectFilter>(edge_type_from_string(type)));
}

static Result<Value> builtin_emboss(const std::vector<Value>& args,
                                    Environment& /*env*/,
                                    SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float direction = static_cast<float>(kw_double(kwargs, "direction", 45.0));
    return Value(std::make_shared<EmbossFilter>(direction));
}

static Result<Value> builtin_convolution(const std::vector<Value>& args,
                                         Environment& /*env*/,
                                         SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    int w = kw_int(kwargs, "width",  3);
    int h = kw_int(kwargs, "height", 3);
    float offset  = static_cast<float>(kw_double(kwargs, "offset",  0.0));
    float divisor = static_cast<float>(kw_double(kwargs, "divisor", 1.0));

    auto values = value_list_to_floats(kwargs["kernel"]);
    int expected = w * h;
    if (expected <= 0) {
        return std::unexpected(type_error(loc, "convolution: width/height must be positive"));
    }
    if (static_cast<int>(values.size()) != expected) {
        return std::unexpected(type_error(loc,
            "convolution: kernel size does not match width*height"));
    }
    return Value(std::make_shared<ConvolutionFilter>(w, h, std::move(values), offset, divisor));
}

static Result<Value> builtin_drop_shadow(const std::vector<Value>& args,
                                         Environment& /*env*/,
                                         SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float dx = static_cast<float>(kw_double(kwargs, "dx",   5.0));
    float dy = static_cast<float>(kw_double(kwargs, "dy",   5.0));
    float blur= static_cast<float>(kw_double(kwargs, "blur", 4.0));
    auto color = parse_color_kw(kwargs, "color", 0xFF000000);
    if (!color) {
        return std::unexpected(type_error(loc, "drop-shadow: invalid color"));
    }
    return Value(std::make_shared<DropShadowFilter>(dx, dy, blur, blur, *color));
}

static Result<Value> builtin_inner_shadow(const std::vector<Value>& args,
                                          Environment& /*env*/,
                                          SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float dx = static_cast<float>(kw_double(kwargs, "dx",   5.0));
    float dy = static_cast<float>(kw_double(kwargs, "dy",   5.0));
    float blur= static_cast<float>(kw_double(kwargs, "blur", 4.0));
    auto color = parse_color_kw(kwargs, "color", 0xFF000000);
    if (!color) {
        return std::unexpected(type_error(loc, "inner-shadow: invalid color"));
    }
    return Value(std::make_shared<InnerShadowFilter>(dx, dy, blur, *color));
}

static Result<Value> builtin_outer_glow(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float blur = static_cast<float>(kw_double(kwargs, "blur", 4.0));
    auto color = parse_color_kw(kwargs, "color", 0xFFFFFFFF);
    if (!color) {
        return std::unexpected(type_error(loc, "outer-glow: invalid color"));
    }
    return Value(std::make_shared<OuterGlowFilter>(blur, *color));
}

static Result<Value> builtin_inner_glow(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float blur = static_cast<float>(kw_double(kwargs, "blur", 4.0));
    auto color = parse_color_kw(kwargs, "color", 0xFFFFFFFF);
    if (!color) {
        return std::unexpected(type_error(loc, "inner-glow: invalid color"));
    }
    return Value(std::make_shared<InnerGlowFilter>(blur, *color));
}

static Result<Value> builtin_bevel_emboss(const std::vector<Value>& args,
                                          Environment& /*env*/,
                                          SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    float angle    = static_cast<float>(kw_double(kwargs, "angle",    120.0));
    float altitude = static_cast<float>(kw_double(kwargs, "altitude", 30.0));
    float blur     = static_cast<float>(kw_double(kwargs, "blur",     4.0));
    auto highlight = parse_color_kw(kwargs, "highlight", 0xFFFFFFFF);
    auto shadow    = parse_color_kw(kwargs, "shadow",    0xFF000000);
    if (!highlight || !shadow) {
        return std::unexpected(type_error(loc, "bevel-emboss: invalid highlight/shadow color"));
    }
    return Value(std::make_shared<BevelEmbossFilter>(
        angle, altitude, blur, *highlight, *shadow));
}

static Result<Value> builtin_filter_chain(const std::vector<Value>& args,
                                          Environment& /*env*/,
                                          SourceLocation loc)
{
    std::vector<std::shared_ptr<ImageFilter>> filters;
    filters.reserve(args.size());
    for (const auto& arg : args) {
        const auto* f = arg.as_image_filter();
        if (!f || !*f) {
            return std::unexpected(type_error(loc,
                "filter-chain: arguments must be filters"));
        }
        filters.push_back(*f);
    }
    return Value(std::make_shared<FilterChain>(std::move(filters)));
}

static Result<Value> builtin_is_filter(const std::vector<Value>& args,
                                       Environment& /*env*/,
                                       SourceLocation loc)
{
    if (args.size() != 1) {
        return std::unexpected(arity_error(loc, 1, static_cast<int>(args.size())));
    }
    return Value(args[0].is_image_filter());
}

} // anonymous namespace

void register_filter_builtins(std::shared_ptr<Environment> env)
{
    env->define("color-adjust", Value(std::make_shared<BuiltinProcedure>(
        "color-adjust", builtin_color_adjust, true)));
    env->define("levels", Value(std::make_shared<BuiltinProcedure>(
        "levels", builtin_levels, true)));
    env->define("curves", Value(std::make_shared<BuiltinProcedure>(
        "curves", builtin_curves, true)));
    env->define("threshold", Value(std::make_shared<BuiltinProcedure>(
        "threshold", builtin_threshold, true)));
    env->define("posterize", Value(std::make_shared<BuiltinProcedure>(
        "posterize", builtin_posterize, true)));

    env->define("blur", Value(std::make_shared<BuiltinProcedure>(
        "blur", builtin_blur, true)));
    env->define("sharpen", Value(std::make_shared<BuiltinProcedure>(
        "sharpen", builtin_sharpen, true)));
    env->define("edge-detect", Value(std::make_shared<BuiltinProcedure>(
        "edge-detect", builtin_edge_detect, true)));
    env->define("emboss", Value(std::make_shared<BuiltinProcedure>(
        "emboss", builtin_emboss, true)));
    env->define("convolution", Value(std::make_shared<BuiltinProcedure>(
        "convolution", builtin_convolution, true)));

    env->define("drop-shadow", Value(std::make_shared<BuiltinProcedure>(
        "drop-shadow", builtin_drop_shadow, true)));
    env->define("inner-shadow", Value(std::make_shared<BuiltinProcedure>(
        "inner-shadow", builtin_inner_shadow, true)));
    env->define("outer-glow", Value(std::make_shared<BuiltinProcedure>(
        "outer-glow", builtin_outer_glow, true)));
    env->define("inner-glow", Value(std::make_shared<BuiltinProcedure>(
        "inner-glow", builtin_inner_glow, true)));
    env->define("bevel-emboss", Value(std::make_shared<BuiltinProcedure>(
        "bevel-emboss", builtin_bevel_emboss, true)));

    env->define("filter-chain", Value(std::make_shared<BuiltinProcedure>(
        "filter-chain", builtin_filter_chain, false)));
    env->define("filter?", Value(std::make_shared<BuiltinProcedure>(
        "filter?", builtin_is_filter, false)));
}

} // namespace pml
