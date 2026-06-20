#include "layer_builtins.h"

#include "aseprite_writer.h"
#include "blend_mode_skia.h"
#include "compositor.h"
#include "composition.h"
#include "layer.h"
#include "spritesheet.h"
#include "pml/api/context.h"
#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/evaluator/environment.h"
#include "pml/filter/image_filter.h"
#include "pml/graphics/color.h"
#include "pml/graphics/transform.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>

namespace pml {

namespace {

using namespace pml::kwargs;

static double to_double_value(const Value& v) {
    if (v.is_double()) return v.double_val();
    if (v.is_int()) return static_cast<double>(v.int_val());
    return 0.0;
}

static std::optional<Color> parse_color_kw(
    const std::unordered_map<std::string, Value>& kwargs, std::string_view key)
{
    auto it = kwargs.find(std::string(key));
    if (it == kwargs.end()) return std::nullopt;
    auto s = value_to_opt_string(it->second);
    if (!s) return std::nullopt;
    auto rgba = parse_color_rgba(*s);
    if (!rgba) return std::nullopt;
    uint8_t r = static_cast<uint8_t>(std::clamp((*rgba)[0] * 255.0, 0.0, 255.0));
    uint8_t g = static_cast<uint8_t>(std::clamp((*rgba)[1] * 255.0, 0.0, 255.0));
    uint8_t b = static_cast<uint8_t>(std::clamp((*rgba)[2] * 255.0, 0.0, 255.0));
    uint8_t a = static_cast<uint8_t>(std::clamp((*rgba)[3] * 255.0, 0.0, 255.0));
    return Color{static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | b)};
}

static std::optional<Point2D> parse_point_kw(
    const std::unordered_map<std::string, Value>& kwargs, const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    const auto* lst = it->second.as_list();
    if (!lst || !*lst) return std::nullopt;
    const auto& elems = (*lst)->elements;
    if (elems.size() != 2) return std::nullopt;
    return Point2D{to_double_value(elems[0]), to_double_value(elems[1])};
}

static std::optional<AffineTransform> parse_transform_kw(
    const std::unordered_map<std::string, Value>& kwargs, const std::string& key)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return std::nullopt;
    const auto* t = it->second.as_transform();
    if (!t || !*t) return std::nullopt;
    return **t;
}

static std::vector<std::shared_ptr<ImageFilter>> parse_filters_kw(
    const std::unordered_map<std::string, Value>& kwargs, const std::string& key)
{
    std::vector<std::shared_ptr<ImageFilter>> result;
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return result;

    if (const auto* f = it->second.as_image_filter()) {
        if (*f) result.push_back(*f);
        return result;
    }

    const auto* lst = it->second.as_list();
    if (!lst || !*lst) return result;
    for (const auto& elem : (*lst)->elements) {
        if (const auto* f = elem.as_image_filter()) {
            if (*f) result.push_back(*f);
        }
    }
    return result;
}

static Result<Value> builtin_make_layer(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc)
{
    if (args.empty()) {
        return std::unexpected(arity_error(loc, 1, 0));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(loc, "make-layer: name must be string"));
    }
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    const auto* go = args[1].as_graphic_object();
    if (!go || !*go) {
        return std::unexpected(type_error(loc, "make-layer: second arg must be a graphic object"));
    }

    auto kwargs = parse_kwargs(args, 2);
    LayerProperties props{*name_opt};
    if (auto t = parse_transform_kw(kwargs, "transform")) props.transform = *t;
    if (auto p = parse_point_kw(kwargs, "offset")) props.offset = *p;
    if (auto a = anchor_from_keyword(kw_string(kwargs, "anchor", "top-left")))
        props.anchor = *a;
    if (auto it = kwargs.find("opacity"); it != kwargs.end() && it->second.is_number())
        props.opacity = static_cast<float>(to_double_value(it->second));
    if (auto b = blend_mode_from_keyword(kw_string(kwargs, "blend", "normal")))
        props.blend = *b;
    if (auto it = kwargs.find("visible"); it != kwargs.end() && it->second.is_bool())
        props.visible = it->second.bool_val();
    if (auto it = kwargs.find("locked"); it != kwargs.end() && it->second.is_bool())
        props.locked = it->second.bool_val();
    if (auto filters = parse_filters_kw(kwargs, "filter"); !filters.empty())
        props.filters = std::move(filters);

    return Value(std::make_shared<Layer>(std::move(props), *go));
}

static Result<Value> builtin_make_group(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc)
{
    if (args.empty()) {
        return std::unexpected(arity_error(loc, 1, 0));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(loc, "make-group: name must be string"));
    }
    std::vector<std::shared_ptr<Layer>> children;
    for (size_t i = 1; i < args.size(); ++i) {
        auto layer_ptr = args[i].as_layer();
        if (!layer_ptr || !*layer_ptr) {
            return std::unexpected(type_error(loc, "make-group: expected layer arguments"));
        }
        children.push_back(*layer_ptr);
    }
    LayerProperties props{*name_opt};
    return Value(std::make_shared<Layer>(std::move(props), std::move(children)));
}

static Result<Value> builtin_make_composition(const std::vector<Value>& args,
                                              Environment& /*env*/,
                                              SourceLocation loc)
{
    if (args.size() < 3) {
        return std::unexpected(arity_error(loc, 3, static_cast<int>(args.size())));
    }
    auto name_opt = value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(loc, "make-composition: name must be string"));
    }
    if (!args[1].is_number() || !args[2].is_number()) {
        return std::unexpected(type_error(loc, "make-composition: width/height must be numbers"));
    }
    int w = static_cast<int>(to_double_value(args[1]));
    int h = static_cast<int>(to_double_value(args[2]));

    auto kwargs = parse_kwargs(args, 3);
    Composition comp(*name_opt, Size2D{w, h}, kw_int(kwargs, "fps", 12));
    if (auto bg = parse_color_kw(kwargs, "bg")) {
        comp = comp.with_background(*bg);
    }
    auto comp_ptr = std::make_shared<Composition>(comp);
    PMLContext::current().compositions.push_back(comp_ptr);
    return Value(comp_ptr);
}

static Result<Value> builtin_composition_add(const std::vector<Value>& args,
                                             Environment& /*env*/,
                                             SourceLocation loc)
{
    if (args.empty()) return std::unexpected(arity_error(loc, 1, 0));
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) {
        return std::unexpected(type_error(loc, "composition-add: first arg must be a composition"));
    }
    Composition comp = **comp_ptr;
    for (size_t i = 1; i < args.size(); ++i) {
        auto layer_ptr = args[i].as_layer();
        if (!layer_ptr || !*layer_ptr) {
            return std::unexpected(type_error(loc, "composition-add: expected layer arguments"));
        }
        comp = comp.with_layer_appended(*layer_ptr);
    }
    auto new_comp = std::make_shared<Composition>(std::move(comp));
    PMLContext::current().compositions.push_back(new_comp);
    return Value(new_comp);
}

static Result<Value> builtin_layer_with(const std::vector<Value>& args,
                                        Environment& /*env*/,
                                        SourceLocation loc)
{
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto layer_ptr = args[0].as_layer();
    if (!layer_ptr || !*layer_ptr) {
        return std::unexpected(type_error(loc, "layer-with: first arg must be a layer"));
    }
    Layer layer = **layer_ptr;
    auto kwargs = parse_kwargs(args, 1);

    if (auto t = parse_transform_kw(kwargs, "transform")) layer = layer.with_transform(*t);
    if (auto p = parse_point_kw(kwargs, "offset")) layer = layer.with_offset(*p);
    if (auto a = anchor_from_keyword(kw_string(kwargs, "anchor", ""))) layer = layer.with_anchor(*a);
    if (auto it = kwargs.find("opacity"); it != kwargs.end() && it->second.is_number())
        layer = layer.with_opacity(static_cast<float>(to_double_value(it->second)));
    if (auto b = blend_mode_from_keyword(kw_string(kwargs, "blend", "")))
        layer = layer.with_blend(*b);
    if (auto it = kwargs.find("visible"); it != kwargs.end() && it->second.is_bool())
        layer = layer.with_visibility(it->second.bool_val());
    if (auto it = kwargs.find("locked"); it != kwargs.end() && it->second.is_bool())
        layer = layer.with_locked(it->second.bool_val());
    if (auto filters = parse_filters_kw(kwargs, "filter"); !filters.empty())
        layer = layer.with_filters(std::move(filters));

    return Value(std::make_shared<Layer>(std::move(layer)));
}

static Result<Value> builtin_composition_render(const std::vector<Value>& args,
                                                Environment& /*env*/,
                                                SourceLocation loc)
{
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) {
        return std::unexpected(type_error(loc, "composition-render: first arg must be a composition"));
    }
    auto filename_opt = value_to_opt_string(args[1]);
    if (!filename_opt) {
        return std::unexpected(type_error(loc, "composition-render: filename must be string"));
    }

    auto kwargs = parse_kwargs(args, 2);
    std::string output_dir = kw_string(kwargs, "output-dir", ".");
    std::string path = (std::filesystem::path(output_dir) / *filename_opt).string();

    auto result = render_composition_to_disk(**comp_ptr, output_dir, *filename_opt);
    if (!result) return std::unexpected(result.error());

    PMLContext::current().output_files.push_back(path);
    return Value(path);
}

static Result<Value> call_step_fn(const Value& fn,
                                  const std::vector<Value>& args,
                                  Environment& env,
                                  SourceLocation loc)
{
    if (auto proc = fn.as_procedure()) {
        if (*proc) return (*proc)->call(args, env);
    }
    if (auto bp = fn.as_builtin()) {
        if (*bp) return (*bp)->call(args, env, loc);
    }
    return std::unexpected(type_error(loc, "composition-animate: :step-fn must be a procedure"));
}

static Result<std::shared_ptr<Composition>> composition_from_step_result(
    const Value& result,
    const Composition& base_comp,
    int fps,
    SourceLocation loc)
{
    if (auto comp = result.as_composition()) {
        if (!*comp) return std::unexpected(composition_error(loc, "step-fn returned null composition"));
        return *comp;
    }

    Composition frame_comp(base_comp.name() + "_frame", base_comp.canvas_size(), fps);

    if (auto layer = result.as_layer()) {
        if (!*layer) return std::unexpected(layer_error(loc, "step-fn returned null layer"));
        frame_comp = frame_comp.with_layer_appended(*layer);
        return std::make_shared<Composition>(std::move(frame_comp));
    }

    if (auto lst = result.as_list()) {
        if (!*lst) return std::unexpected(type_error(loc, "step-fn returned empty list"));
        for (const auto& elem : (*lst)->elements) {
            auto layer = elem.as_layer();
            if (!layer || !*layer) {
                return std::unexpected(type_error(loc, "step-fn list must contain layers"));
            }
            frame_comp = frame_comp.with_layer_appended(*layer);
        }
        return std::make_shared<Composition>(std::move(frame_comp));
    }

    return std::unexpected(type_error(loc, "step-fn must return a composition, layer, or list of layers"));
}

static Result<Value> builtin_composition_animate(const std::vector<Value>& args,
                                                 Environment& env,
                                                 SourceLocation loc)
{
    using namespace pml::kwargs;
    if (args.size() < 2) {
        return std::unexpected(arity_error(loc, 2, static_cast<int>(args.size())));
    }
    auto comp_ptr = args[0].as_composition();
    if (!comp_ptr || !*comp_ptr) {
        return std::unexpected(type_error(loc, "composition-animate: first arg must be a composition"));
    }
    auto basename_opt = value_to_opt_string(args[1]);
    if (!basename_opt) {
        return std::unexpected(type_error(loc, "composition-animate: basename must be string"));
    }

    auto kwargs = parse_kwargs(args, 2);
    int frames = kw_int(kwargs, "frames", 1);
    if (frames <= 0) {
        return std::unexpected(composition_error(loc, "composition-animate: frames must be positive"));
    }
    int fps = kw_int(kwargs, "fps", (**comp_ptr).fps());
    if (fps <= 0) fps = 12;
    int cols = kw_int(kwargs, "cols", 0);
    int padding = kw_int(kwargs, "padding", 0);
    std::string output_dir = kw_string(kwargs, "output-dir", ".");
    std::string meta = kw_string(kwargs, "meta", "aseprite");

    std::optional<Value> step_fn;
    if (auto it = kwargs.find("step-fn"); it != kwargs.end()) {
        if (!it->second.is_procedure() && !it->second.is_builtin()) {
            return std::unexpected(type_error(loc, "composition-animate: :step-fn must be a procedure"));
        }
        step_fn = it->second;
    }

    auto& backend = BackendRegistry::instance().active();
    if (!backend.supports_layer_compositing()) {
        return std::unexpected(resource_error("active backend does not support layer compositing"));
    }

    const Composition& base_comp = **comp_ptr;
    Compositor compositor(backend);
    std::vector<std::unique_ptr<Surface>> frame_surfaces;
    frame_surfaces.reserve(frames);

    for (int i = 0; i < frames; ++i) {
        std::shared_ptr<Composition> frame_comp = *comp_ptr;

        if (step_fn) {
            double t = static_cast<double>(i) / static_cast<double>(fps);
            std::vector<Value> fn_args;
            fn_args.reserve(3);
            fn_args.emplace_back(static_cast<int64_t>(i));
            fn_args.emplace_back(static_cast<int64_t>(frames));
            fn_args.emplace_back(t);

            auto fn_result = call_step_fn(*step_fn, fn_args, env, loc);
            if (!fn_result) return std::unexpected(fn_result.error());

            auto comp_result = composition_from_step_result(*fn_result, base_comp, fps, loc);
            if (!comp_result) return std::unexpected(comp_result.error());
            frame_comp = *comp_result;
        }

        auto surface = compositor.render(*frame_comp);
        if (!surface) return std::unexpected(surface.error());
        frame_surfaces.push_back(std::move(*surface));
    }

    SpriteSheetPacker packer(backend, padding);
    auto sheet_result = packer.pack(frame_surfaces, cols);
    if (!sheet_result) return std::unexpected(sheet_result.error());

    std::filesystem::path out_dir(output_dir);
    std::filesystem::create_directories(out_dir);

    std::string png_path = (out_dir / (*basename_opt + ".png")).string();
    auto save_result = backend.save_image(*sheet_result->surface, png_path, "png");
    if (!save_result) return std::unexpected(save_result.error());

    if (meta == "aseprite") {
        AsepriteMeta ameta;
        ameta.image = *basename_opt + ".png";
        ameta.frame_width = sheet_result->frame_width;
        ameta.frame_height = sheet_result->frame_height;
        ameta.columns = sheet_result->columns;
        ameta.rows = sheet_result->rows;
        ameta.frame_duration_ms = (fps > 0) ? (1000 / fps) : 100;

        AsepriteFrameTag tag;
        tag.name = "default";
        tag.from = 0;
        tag.to = static_cast<int>(sheet_result->frames.size()) - 1;
        tag.direction = "forward";
        ameta.frame_tags.push_back(std::move(tag));

        std::string json_path = (out_dir / (*basename_opt + ".json")).string();
        std::ofstream ofs(json_path);
        if (!ofs) {
            return std::unexpected(resource_error("cannot write metadata: " + json_path));
        }
        ofs << write_aseprite_json(*sheet_result, ameta);
        PMLContext::current().output_files.push_back(json_path);
    }

    PMLContext::current().output_files.push_back(png_path);
    return Value(nullptr);
}

static Result<Value> builtin_project_render_all(const std::vector<Value>& args,
                                                Environment& /*env*/,
                                                SourceLocation loc)
{
    auto kwargs = parse_kwargs(args, 0);
    std::string output_dir = kw_string(kwargs, "output-dir", ".");
    for (const auto& comp : PMLContext::current().compositions) {
        if (!comp) continue;
        auto result = render_composition_to_disk(*comp, output_dir);
        if (!result) return std::unexpected(result.error());
    }
    return Value(nullptr);
}

static Result<Value> builtin_is_layer(const std::vector<Value>& args,
                                      Environment& /*env*/,
                                      SourceLocation loc)
{
    if (args.size() != 1) return std::unexpected(arity_error(loc, 1, static_cast<int>(args.size())));
    return Value(args[0].is_layer());
}

static Result<Value> builtin_is_composition(const std::vector<Value>& args,
                                            Environment& /*env*/,
                                            SourceLocation loc)
{
    if (args.size() != 1) return std::unexpected(arity_error(loc, 1, static_cast<int>(args.size())));
    return Value(args[0].is_composition());
}

} // anonymous namespace

Result<std::string> render_composition_to_disk(const Composition& comp,
                                               const std::string& output_dir,
                                               const std::string& filename)
{
    auto& backend = BackendRegistry::instance().active();
    if (!backend.supports_layer_compositing()) {
        return std::unexpected(resource_error("active backend does not support layer compositing"));
    }

    Compositor compositor(backend);
    auto surface = compositor.render(comp);
    if (!surface) return std::unexpected(surface.error());

    std::string out_filename = filename.empty() ? (comp.name() + ".png") : filename;
    std::filesystem::path out_path(output_dir);
    out_path /= out_filename;
    std::filesystem::create_directories(out_path.parent_path());

    auto save_result = backend.save_image(**surface, out_path.string(), "png");
    if (!save_result) return std::unexpected(save_result.error());

    PMLContext::current().output_files.push_back(out_path.string());
    return out_path.string();
}

void register_layer_builtins(std::shared_ptr<Environment> env)
{
    env->define("make-layer", Value(std::make_shared<BuiltinProcedure>(
        "make-layer", builtin_make_layer, true)));
    env->define("make-group", Value(std::make_shared<BuiltinProcedure>(
        "make-group", builtin_make_group, false)));
    env->define("make-composition", Value(std::make_shared<BuiltinProcedure>(
        "make-composition", builtin_make_composition, true)));
    env->define("composition-add", Value(std::make_shared<BuiltinProcedure>(
        "composition-add", builtin_composition_add, false)));
    env->define("layer-with", Value(std::make_shared<BuiltinProcedure>(
        "layer-with", builtin_layer_with, true)));
    env->define("composition-render", Value(std::make_shared<BuiltinProcedure>(
        "composition-render", builtin_composition_render, true)));
    env->define("composition-animate", Value(std::make_shared<BuiltinProcedure>(
        "composition-animate", builtin_composition_animate, true)));
    env->define("project-render-all", Value(std::make_shared<BuiltinProcedure>(
        "project-render-all", builtin_project_render_all, true)));
    env->define("layer?", Value(std::make_shared<BuiltinProcedure>(
        "layer?", builtin_is_layer, false)));
    env->define("composition?", Value(std::make_shared<BuiltinProcedure>(
        "composition?", builtin_is_composition, false)));
}

} // namespace pml
