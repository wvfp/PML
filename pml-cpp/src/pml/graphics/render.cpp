// ═══════════════════════════════════════════════════════════════════════════════
// PML Render Dispatch — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "render.h"

#include "registry.h"       // BackendRegistry (via -I../backend)
#include "backend.h"        // RenderBackend ABC
#include "color_helpers.h"  // parse_color() string → uint32_t
#include "canvas.h"         // Canvas (+ g_current_canvas)
#include "objects.h"        // GraphicObject
#include "transform.h"      // ::AffineTransform (global namespace)
#include "timeline.h"       // Timeline, Animation, _apply_modifications
#include "types.h"
#include "error.h"
#include "environment.h"    // full definition for env->define()

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>

// nlohmann-json (available via pml_core)
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers — argument parsing from PML Values
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

[[nodiscard]] std::optional<std::string> value_to_opt_string(const Value& v)
{
    return std::visit([](const auto& arg) -> std::optional<std::string> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, Symbol>) {
            return arg.name;
        } else if constexpr (std::is_same_v<T, Keyword>) {
            return arg.name;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        }
        return std::nullopt;
    }, v);
}

[[nodiscard]] Result<std::string> value_to_string_req(const Value& v)
{
    auto s = value_to_opt_string(v);
    if (s.has_value()) return std::move(*s);
    return std::unexpected(type_error("expected a string, got " + value_to_string(v)));
}

[[nodiscard]] std::unordered_map<std::string, Value> parse_kwargs(
    const std::vector<Value>& args, size_t start)
{
    std::unordered_map<std::string, Value> result;
    for (size_t i = start; i + 1 < args.size(); i += 2) {
        if (const auto* kw = std::get_if<Keyword>(&args[i])) {
            result[kw->name] = args[i + 1];
        } else if (const auto* sym = std::get_if<Symbol>(&args[i])) {
            result[sym->name] = args[i + 1];
        } else {
            break;
        }
    }
    return result;
}

[[nodiscard]] std::string kw_string(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    const std::string& default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    return value_to_opt_string(it->second).value_or(default_val);
}

[[nodiscard]] int kw_int(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    int default_val)
{
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_val;
    if (auto* i = std::get_if<int64_t>(&it->second)) {
        return static_cast<int>(*i);
    }
    return default_val;
}

[[nodiscard]] std::string detect_format(
    const std::string& filename, const std::string& fmt_hint)
{
    if (!fmt_hint.empty()) {
        std::string upper = fmt_hint;
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return upper;
    }
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "PNG";
    std::string ext = filename.substr(dot + 1);
    for (auto& c : ext) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (ext == "JPG" || ext == "JPEG") return "JPEG";
    if (ext == "PNG")  return "PNG";
    if (ext == "GIF")  return "GIF";
    if (ext == "SVG")  return "SVG";
    if (ext == "BMP")  return "BMP";
    if (ext == "TIFF" || ext == "TIF") return "TIFF";
    if (ext == "WEBP") return "WEBP";
    return ext;
}

[[nodiscard]] Result<void> write_json_file(
    const std::string& path, const json& data)
{
    std::ofstream out(path);
    if (!out.is_open()) {
        return std::unexpected(resource_error(
            "failed to write metadata file: " + path));
    }
    out << data.dump(2) << std::endl;
    return {};
}

[[nodiscard]] std::string meta_path(const std::string& filename)
{
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        return filename.substr(0, dot) + ".meta.json";
    }
    return filename + ".meta.json";
}

[[nodiscard]] std::string spritesheet_meta_path(const std::string& filename)
{
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        return filename.substr(0, dot) + ".spritesheet.json";
    }
    return filename + ".spritesheet.json";
}

// Parse a CSS colour string to uint32_t ARGB via the backend's color helper.
[[nodiscard]] uint32_t bg_color_to_uint32(const std::string& color_str)
{
    return parse_color(color_str).value_or(0x00000000);
}

// Check a Result<void> and propagate any error.
[[nodiscard]] Result<void> check_result(Result<void>&& r)
{
    if (!r) return std::unexpected(std::move(r.error()));
    return {};
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// SVG Path Parsing
// ═══════════════════════════════════════════════════════════════════════════════

auto parse_svg_path(const std::string& svg_path)
    -> Result<std::vector<PathCommand>>
{
    std::vector<PathCommand> commands;

    // State machine for parsing
    enum class State { Command, Number, Done };
    State state = State::Command;

    size_t pos = 0;
    char current_cmd = 0;
    bool relative = false;

    // Buffer for reading numbers
    auto skip_whitespace = [&]() {
        while (pos < svg_path.size() && (svg_path[pos] == ' ' ||
               svg_path[pos] == '\t' || svg_path[pos] == '\n' ||
               svg_path[pos] == '\r' || svg_path[pos] == ',')) {
            ++pos;
        }
    };

    // Parse a single float from the current position
    auto parse_number = [&]() -> std::optional<double> {
        skip_whitespace();
        if (pos >= svg_path.size()) return std::nullopt;

        const char* start = svg_path.data() + pos;
        char* end = nullptr;
        double val = std::strtod(start, &end);
        if (end == start) return std::nullopt;  // no number parsed

        pos += static_cast<size_t>(end - start);
        return val;
    };

    // Parse N numbers into the args vector
    auto parse_n_numbers = [&](int n, std::vector<double>& args) -> bool {
        for (int i = 0; i < n; ++i) {
            auto num = parse_number();
            if (!num.has_value()) return false;
            args.push_back(*num);
        }
        return true;
    };

    // Helper: get the expected arg count for a command type
    auto args_for_type = [](PathCommand::Type type) -> int {
        switch (type) {
            case PathCommand::Type::MoveTo:           return 2;
            case PathCommand::Type::LineTo:           return 2;
            case PathCommand::Type::HorizontalLineTo: return 1;
            case PathCommand::Type::VerticalLineTo:   return 1;
            case PathCommand::Type::CurveTo:          return 6;
            case PathCommand::Type::SmoothCurveTo:    return 4;
            case PathCommand::Type::QuadTo:           return 4;
            case PathCommand::Type::SmoothQuadTo:     return 2;
            case PathCommand::Type::ArcTo:            return 7;
            case PathCommand::Type::ClosePath:        return 0;
        }
        return 0;
    };

    // Emit a command with its arguments
    auto emit_command = [&](PathCommand::Type type, bool rel,
                            std::vector<double> args) {
        commands.push_back({type, rel, std::move(args)});
    };

    // Parse a command letter and set up the current command
    auto parse_command = [&]() -> bool {
        skip_whitespace();
        if (pos >= svg_path.size()) return false;

        char c = svg_path[pos];
        if (!std::isalpha(static_cast<unsigned char>(c)) && c != 'Z' && c != 'z') {
            return false;
        }
        ++pos;

        relative = std::islower(static_cast<unsigned char>(c));
        current_cmd = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return true;
    };

    // Map command char to enum
    auto char_to_type = [](char c) -> std::optional<PathCommand::Type> {
        switch (c) {
            case 'M': return PathCommand::Type::MoveTo;
            case 'L': return PathCommand::Type::LineTo;
            case 'H': return PathCommand::Type::HorizontalLineTo;
            case 'V': return PathCommand::Type::VerticalLineTo;
            case 'C': return PathCommand::Type::CurveTo;
            case 'S': return PathCommand::Type::SmoothCurveTo;
            case 'Q': return PathCommand::Type::QuadTo;
            case 'T': return PathCommand::Type::SmoothQuadTo;
            case 'A': return PathCommand::Type::ArcTo;
            case 'Z': return PathCommand::Type::ClosePath;
            default:  return std::nullopt;
        }
    };

    while (pos < svg_path.size()) {
        skip_whitespace();
        if (pos >= svg_path.size()) break;

        char c = svg_path[pos];

        if (std::isalpha(static_cast<unsigned char>(c))) {
            // New command
            if (!parse_command()) {
                return std::unexpected(syntax_error(
                    "invalid SVG path command at position " + std::to_string(pos)));
            }

            auto type = char_to_type(current_cmd);
            if (!type.has_value()) {
                return std::unexpected(syntax_error(
                    "unknown SVG path command: " + std::string(1, current_cmd)));
            }

            if (*type == PathCommand::Type::ClosePath && current_cmd == 'Z') {
                emit_command(PathCommand::Type::ClosePath, false, {});
                continue;
            }

            int nargs = args_for_type(*type);
            std::vector<double> args;
            if (!parse_n_numbers(nargs, args)) {
                return std::unexpected(syntax_error(
                    "unexpected end of SVG path data"));
            }
            emit_command(*type, relative, std::move(args));

            // Handle implicit repeated commands
            // After M, subsequent coordinate pairs are implicit L
            // After all other commands, subsequent coordinate pairs repeat the same command
        } else if (c == '-' || c == '+' || c == '.' || std::isdigit(static_cast<unsigned char>(c))) {
            // Implicit repeated command — no letter needed
            if (current_cmd == 0) {
                return std::unexpected(syntax_error(
                    "unexpected number in SVG path without preceding command"));
            }

            auto type = char_to_type(current_cmd);
            if (!type.has_value()) {
                return std::unexpected(syntax_error(
                    "invalid repeated SVG path command"));
            }

            int nargs = args_for_type(*type);
            std::vector<double> args;
            if (!parse_n_numbers(nargs, args)) {
                return std::unexpected(syntax_error(
                    "unexpected end of SVG path data for implicit command"));
            }
            emit_command(*type, relative, std::move(args));
        } else {
            return std::unexpected(syntax_error(
                "unexpected character '" + std::string(1, c) +
                "' in SVG path at position " + std::to_string(pos)));
        }
    }

    return commands;
}

// Render a single frame of a canvas to a freshly-created surface.
// Shared by static render() and animation render.
[[nodiscard]] static Result<std::unique_ptr<Surface>> render_frame(
    RenderBackend& backend,
    const Canvas& canvas)
{
    uint32_t bg = bg_color_to_uint32(canvas.bg_color);
    auto surface = backend.create_surface(canvas.width, canvas.height, bg);
    if (!surface) {
        return std::unexpected(resource_error("failed to create render surface"));
    }

    if (canvas.is_sprite) {
        AffineTransform center_t = AffineTransform::translate(
            static_cast<double>(canvas.width) / 2.0,
            static_cast<double>(canvas.height) * 0.45);
        for (const auto& obj : canvas.objects) {
            AffineTransform composed = center_t.compose(obj.transform);
            auto centered = obj.with_transform(composed);
            auto r = backend.draw(*surface, centered);
            if (!r) return std::unexpected(std::move(r.error()));
        }
    } else {
        for (const auto& obj : canvas.objects) {
            auto r = backend.draw(*surface, obj);
            if (!r) return std::unexpected(std::move(r.error()));
        }
    }

    return surface;
}

// Render an animated GIF/sequence by stepping the global timeline.
[[nodiscard]] static Result<std::string> render_animation(
    const std::string& filename,
    const std::string& fmt,
    std::shared_ptr<Canvas> canvas,
    int fps)
{
    if (fps <= 0) fps = 30;

    auto timeline = get_or_create_timeline();
    timeline->play();

    double total_duration = timeline->get_total_duration();
    if (total_duration <= 0.0) total_duration = 1.0;

    int num_frames = std::max(1, static_cast<int>(std::ceil(total_duration * fps)));

    RenderBackend& backend = BackendRegistry::instance().active();

    // Preserve original canvas objects so repeated renders are deterministic.
    auto original_objects = canvas->objects;

    std::vector<std::unique_ptr<Surface>> frame_surfaces;
    std::vector<Surface*> frame_ptrs;
    frame_surfaces.reserve(static_cast<size_t>(num_frames));
    frame_ptrs.reserve(static_cast<size_t>(num_frames));

    for (int i = 0; i < num_frames; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(fps);

        // Invoke per-frame hooks.
        for (const auto& hook : timeline->frame_hooks) {
            hook(t);
        }

        // Evaluate animations and build target_id -> {property -> value} map.
        auto modifications = timeline->evaluate_at(t);
        std::unordered_map<uint64_t, std::unordered_map<std::string, Value>> obj_mods;
        for (const auto& [target_id, prop, value] : modifications) {
            obj_mods[target_id][prop] = value;
        }

        // Apply modifications to canvas objects in-place.
        for (auto& obj : canvas->objects) {
            auto it = obj_mods.find(obj.id);
            if (it != obj_mods.end()) {
                obj = _apply_modifications(obj, it->second);
            }
        }

        // Render this frame.
        auto frame = render_frame(backend, *canvas);
        if (!frame) return std::unexpected(std::move(frame.error()));

        frame_ptrs.push_back(frame->get());
        frame_surfaces.push_back(std::move(*frame));
    }

    // Restore original object state for deterministic re-renders.
    canvas->objects = std::move(original_objects);

    timeline->current_time = total_duration;
    timeline->state = TimelineState::Finished;

    auto sr = backend.save_animation(frame_ptrs, filename, fmt, fps);
    if (!sr) return std::unexpected(std::move(sr.error()));

    return filename;
}

auto render(
    const std::string& filename,
    const std::string& fmt,
    std::shared_ptr<Canvas> canvas,
    int fps)
    -> Result<std::string>
{
    if (!canvas) canvas = get_current_canvas();
    if (!canvas) canvas = std::make_shared<Canvas>(256, 256, "transparent");

    const std::string format = detect_format(filename, fmt);

    // Determine whether this should be an animation render.
    bool has_animation = false;
    if (g_timeline && !g_timeline->animations.empty()) {
        has_animation = true;
    }

    if (fps > 0 || (has_animation && format == "GIF")) {
        int effective_fps = (fps > 0) ? fps : 30;
        return render_animation(filename, format, canvas, effective_fps);
    }

    RenderBackend& backend = BackendRegistry::instance().active();

    auto surface_result = render_frame(backend, *canvas);
    if (!surface_result) return std::unexpected(std::move(surface_result.error()));
    auto& surface = *surface_result;

    auto sr = backend.save_image(*surface, filename, format);
    if (!sr) return std::unexpected(std::move(sr.error()));

    if (canvas->is_sprite) {
        json meta;
        meta["file"]    = filename;
        meta["width"]   = canvas->width;
        meta["height"]  = canvas->height;
        meta["anchor"]  = canvas->anchor;
        meta["padding"] = canvas->padding;
        meta["format"]  = format;
        meta["pml_version"] = "0.1.0";
        auto wj = write_json_file(meta_path(filename), meta);
        if (!wj) { /* non-fatal — match Python behaviour */ }
    }

    return filename;
}

auto render_set(
    const std::string& name,
    const GraphicObject& content,
    const std::vector<int>& scales,
    int base_width,
    int base_height,
    const std::string& fmt)
    -> Result<std::vector<std::string>>
{
    if (scales.empty()) return std::vector<std::string>{};

    RenderBackend& backend = BackendRegistry::instance().active();
    std::vector<std::string> results;
    results.reserve(scales.size());

    for (int s : scales) {
        const int w = base_width * s;
        const int h = base_height * s;

        uint32_t bg = bg_color_to_uint32("transparent");
        auto surface = backend.create_surface(w, h, bg);
        if (!surface) {
            return std::unexpected(resource_error(
                "failed to create render-set surface at " + std::to_string(s) + "x"));
        }

        AffineTransform scale_t = AffineTransform::scale(
            static_cast<double>(s), static_cast<double>(s));
        AffineTransform new_t = scale_t.compose(content.transform);
        GraphicObject scaled = content.with_transform(new_t);

        auto dr = backend.draw(*surface, scaled);
        if (!dr) return std::unexpected(std::move(dr.error()));

        std::string suffix = "@" + std::to_string(s) + "x";
        std::string ext;
        if (fmt == "PNG")        ext = ".png";
        else if (fmt == "JPEG")  ext = ".jpg";
        else if (fmt == "GIF")   ext = ".gif";
        else if (fmt == "SVG")   ext = ".svg";
        else                     ext = "." + fmt;

        std::string out_name = name + suffix + ext;
        auto sr = backend.save_image(*surface, out_name, fmt);
        if (!sr) return std::unexpected(std::move(sr.error()));

        results.push_back(std::move(out_name));
    }

    return results;
}

auto render_spritesheet(
    const std::string& filename,
    const std::vector<GraphicObject>& sprites,
    int cols,
    int cell_width,
    int cell_height,
    int padding,
    const std::string& bg)
    -> Result<std::string>
{
    if (sprites.empty()) return filename;

    const int rows = std::max(1, (static_cast<int>(sprites.size()) + cols - 1) / cols);
    const int total_w = cols * (cell_width + padding) + padding;
    const int total_h = rows * (cell_height + padding) + padding;

    RenderBackend& backend = BackendRegistry::instance().active();

    uint32_t bg_col = bg_color_to_uint32(bg);
    auto surface = backend.create_surface(total_w, total_h, bg_col);
    if (!surface) {
        return std::unexpected(resource_error("failed to create spritesheet surface"));
    }

    json frames = json::array();

    for (size_t i = 0; i < sprites.size(); ++i) {
        const int col = static_cast<int>(i) % cols;
        const int row = static_cast<int>(i) / cols;
        const int x = padding + col * (cell_width + padding);
        const int y = padding + row * (cell_height + padding);

        uint32_t cell_bg = bg_color_to_uint32("transparent");
        auto cell_surface = backend.create_surface(cell_width, cell_height, cell_bg);
        if (cell_surface) {
            auto dr = backend.draw(*cell_surface, sprites[i]);
            if (!dr) return std::unexpected(std::move(dr.error()));

            auto cr = backend.composite(*surface, *cell_surface, x, y);
            if (!cr) return std::unexpected(std::move(cr.error()));
        }

        frames.push_back({
            {"index",  i},
            {"x",      x},
            {"y",      y},
            {"width",  cell_width},
            {"height", cell_height},
            {"col",    col},
            {"row",    row},
        });
    }

    const std::string format = detect_format(filename, "PNG");

    auto sr = backend.save_image(*surface, filename, format);
    if (!sr) return std::unexpected(std::move(sr.error()));

    json meta;
    meta["file"]          = filename;
    meta["format"]        = format;
    meta["total_width"]   = total_w;
    meta["total_height"]  = total_h;
    meta["cols"]          = cols;
    meta["rows"]          = rows;
    meta["cell_width"]    = cell_width;
    meta["cell_height"]   = cell_height;
    meta["padding"]       = padding;
    meta["frames"]        = std::move(frames);
    meta["pml_version"]   = "0.1.0";

    auto wj = write_json_file(spritesheet_meta_path(filename), meta);
    if (!wj) { /* non-fatal */ }

    return filename;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Environment Registration — bind render functions to PML
// ═══════════════════════════════════════════════════════════════════════════════
//
// Each function is wrapped in a BuiltinProcedure with accepts_kwargs=true.
// The callback parses arguments from PML Value vectors and dispatches to
// the C++ render functions above.
// ═══════════════════════════════════════════════════════════════════════════════

void register_render(std::shared_ptr<Environment> env)
{
    if (!env) return;

    // ── (render filename :format "PNG" ...) ──────────────────────────────
    //
    // Signature: (render filename [kwargs...])
    //   - filename: string (required, first positional arg)
    //   - :format: string (optional, output format)
    // Renders the current canvas to a file.
    {
            auto render_fn = [](const std::vector<Value>& args,
                                Environment& /*env*/) -> Result<Value>
            {
                if (args.empty()) {
                    return std::unexpected(arity_error(
                        SourceLocation{}, 1, static_cast<int>(args.size())));
                }

                // First arg: filename
                auto filename = value_to_string_req(args[0]);
                if (!filename) return std::unexpected(std::move(filename.error()));

                // Remaining args: kwargs
                std::string fmt;
                int fps = 0;
                if (args.size() > 1) {
                    auto kwargs = parse_kwargs(args, 1);
                    fmt = kw_string(kwargs, "format", "");
                    if (auto it = kwargs.find("fps"); it != kwargs.end()) {
                        if (std::holds_alternative<int64_t>(it->second)) {
                            fps = static_cast<int>(std::get<int64_t>(it->second));
                        } else if (std::holds_alternative<double>(it->second)) {
                            fps = static_cast<int>(std::get<double>(it->second));
                        }
                    }
                }

                auto result = render(*filename, fmt, nullptr, fps);
                if (!result) return std::unexpected(std::move(result.error()));

                return Value(std::move(*result));
            };

            env->define("render",
                Value(std::make_shared<BuiltinProcedure>(
                    "render", std::move(render_fn), true)));
        }

    // ── (render-set name :content obj :scales (1 2 4) :base-size (64 64)) ─
    //
    // Signature: (render-set name [kwargs...])
    //   - name: string (required, first positional arg)
    //   - :content: GraphicObject (required kwarg)
    //   - :scales: list of ints (optional, default (1 2 4))
    //   - :base-size: list of two ints (optional, default (64 64))
    //   - :format: string (optional, output format)
    {
        auto render_set_fn = [](const std::vector<Value>& args,
                                Environment& /*env*/) -> Result<Value>
        {
            if (args.empty()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            auto name = value_to_string_req(args[0]);
            if (!name) return std::unexpected(std::move(name.error()));

            auto kwargs = parse_kwargs(args, 1);

            // Extract :content (required)
            std::shared_ptr<GraphicObject> content_obj;
            if (auto it = kwargs.find("content"); it != kwargs.end()) {
                if (auto* go = std::get_if<std::shared_ptr<GraphicObject>>(&it->second)) {
                    content_obj = *go;
                } else {
                    return std::unexpected(type_error(
                        "render-set :content expects a GraphicObject, got " +
                        value_to_string(it->second)));
                }
            }

            if (!content_obj) {
                return std::unexpected(type_error(
                    "render-set requires :content kwarg with a GraphicObject"));
            }

            // Extract :scales (default {1, 2, 4})
            std::vector<int> scales = {1, 2, 4};
            if (auto it = kwargs.find("scales"); it != kwargs.end()) {
                scales.clear();
                if (auto* lst = std::get_if<std::shared_ptr<ValueList>>(&it->second)) {
                    for (const auto& elem : lst->get()->elements) {
                        if (auto* i = std::get_if<int64_t>(&elem)) {
                            scales.push_back(static_cast<int>(*i));
                        }
                    }
                }
                if (scales.empty()) scales = {1, 2, 4};
            }

            // Extract :base-size (default {64, 64})
            int base_w = 64, base_h = 64;
            if (auto it = kwargs.find("base-size"); it != kwargs.end()) {
                if (auto* lst = std::get_if<std::shared_ptr<ValueList>>(&it->second)) {
                    const auto& elems = lst->get()->elements;
                    if (elems.size() >= 1 && std::holds_alternative<int64_t>(elems[0]))
                        base_w = static_cast<int>(std::get<int64_t>(elems[0]));
                    if (elems.size() >= 2 && std::holds_alternative<int64_t>(elems[1]))
                        base_h = static_cast<int>(std::get<int64_t>(elems[1]));
                }
            }

            std::string fmt = kw_string(kwargs, "format", "PNG");

            auto results = render_set(*name, *content_obj, scales,
                                       base_w, base_h, fmt);
            if (!results) return std::unexpected(std::move(results.error()));

            // Convert result list to PML Value list
            auto val_list = std::make_shared<ValueList>();
            for (auto& f : *results) {
                val_list->elements.push_back(Value(std::move(f)));
            }
            return Value(std::move(val_list));
        };

        env->define("render-set",
            Value(std::make_shared<BuiltinProcedure>(
                "render-set", std::move(render_set_fn), true)));
    }

    // ── (render-spritesheet "file.png" sprite... :cols 4 ...) ────────────
    //
    // Signature: (render-spritesheet filename [sprites...] [kwargs...])
    //   - filename: string (required, first positional arg)
    //   - sprites: GraphicObject values (positional, after filename)
    //   - :cols, :cell-width, :cell-height, :padding, :bg: kwargs
    {
        auto spritesheet_fn = [](const std::vector<Value>& args,
                                 Environment& /*env*/) -> Result<Value>
        {
            if (args.empty()) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            auto filename = value_to_string_req(args[0]);
            if (!filename) return std::unexpected(std::move(filename.error()));

            // Collect sprites from positional args (after filename, before first Keyword)
            std::vector<GraphicObject> sprites;
            size_t kw_start = args.size();
            for (size_t i = 1; i < args.size(); ++i) {
                if (std::holds_alternative<Keyword>(args[i]) ||
                    std::holds_alternative<Symbol>(args[i])) {
                    kw_start = i;
                    break;
                }
                if (auto* go = std::get_if<std::shared_ptr<GraphicObject>>(&args[i])) {
                    sprites.push_back(**go);  // dereference shared_ptr to copy
                }
            }

            auto kwargs = parse_kwargs(args, kw_start);

            int cols        = kw_int(kwargs, "cols", 4);
            int cell_w      = kw_int(kwargs, "cell-width", 64);
            int cell_h      = kw_int(kwargs, "cell-height", 64);
            int padding     = kw_int(kwargs, "padding", 2);
            std::string bg  = kw_string(kwargs, "bg", "transparent");

            auto result = render_spritesheet(*filename, sprites,
                                             cols, cell_w, cell_h,
                                             padding, bg);
            if (!result) return std::unexpected(std::move(result.error()));

            return Value(std::move(*result));
        };

        env->define("render-spritesheet",
            Value(std::make_shared<BuiltinProcedure>(
                "render-spritesheet", std::move(spritesheet_fn), true)));
    }
}

}  // namespace pml
