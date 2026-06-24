// ═══════════════════════════════════════════════════════════════════════════════
// PML Render Dispatch — Implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "render.h"

#include "registry.h"      // BackendRegistry (via -I../backend)
#include "backend.h"       // RenderBackend ABC
#include "color_helpers.h" // parse_color() string → uint32_t
#include "canvas.h"        // Canvas (+ g_current_canvas)
#include "objects.h"       // GraphicObject
#include "transform.h"     // AffineTransform
#include "tilemap.h"       // TilemapManager, Tilemap, tile_at
#include "tileset.h"       // TilesetManager, Tileset, TileType
#include "pml/graphics3d/camera3d.h"
#include "timeline.h"      // Timeline, Animation, _apply_modifications
#include "graphics_types.h"
#include "error.h"
#include "kwargs.h"
#include "environment.h" // full definition for env->define()
#include "pml/api/context.h" // PMLContext::current().output_files

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

// nlohmann-json (available via pml_core)
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace pml {

// Forward declaration from pml/skeleton/skin_binding.h — implemented in
// pml_skeleton so that pml_graphics can apply skin bindings during render
// without creating a target dependency cycle.
void merge_skin_bindings(
    std::unordered_map<uint64_t, std::unordered_map<std::string, Value>>& obj_mods);

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers — argument parsing from PML Values
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// Keyword-argument helpers are provided by pml/core/kwargs.h.
using pml::kwargs::kw_int;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;

// Ensure the parent directory of a file path exists.
static void ensure_parent_dir(const std::string& filepath) {
    namespace fs = std::filesystem;
    fs::path p(filepath);
    auto parent = p.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        fs::create_directories(parent, ec);
    }
}

// Resolve a render filename against the current PML source directory and
// the CLI output directory (-o). Relative paths are first resolved relative
// to the source file; if an output directory is set, the final basename is
// placed inside that directory (matching Python reference behaviour).
static std::string resolve_output_path(const std::string& filename) {
    namespace fs = std::filesystem;
    const PMLContext& ctx = PMLContext::current();
    fs::path p(filename);

    if (p.is_relative() && !ctx.source_dir.empty()) {
        p = fs::path(ctx.source_dir) / p;
    }

    if (!ctx.output_dir.empty()) {
        fs::path out_dir(ctx.output_dir);
        p = out_dir / p.filename();
    }

    return p.string();
}
using pml::kwargs::value_to_opt_string;
using pml::kwargs::value_to_string_req;

[[nodiscard]] std::string detect_format(const std::string& filename, const std::string& fmt_hint) {
    if (!fmt_hint.empty()) {
        std::string upper = fmt_hint;
        for (auto& c : upper)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return upper;
    }
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos)
        return "PNG";
    std::string ext = filename.substr(dot + 1);
    for (auto& c : ext)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (ext == "JPG" || ext == "JPEG")
        return "JPEG";
    if (ext == "PNG")
        return "PNG";
    if (ext == "GIF")
        return "GIF";
    if (ext == "SVG")
        return "SVG";
    if (ext == "BMP")
        return "BMP";
    if (ext == "TIFF" || ext == "TIF")
        return "TIFF";
    if (ext == "WEBP")
        return "WEBP";
    return ext;
}

[[nodiscard]] Result<void> write_json_file(const std::string& path, const json& data) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return std::unexpected(resource_error("failed to write metadata file: " + path));
    }
    out << data.dump(2) << std::endl;
    return {};
}

[[nodiscard]] std::string meta_path(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        return filename.substr(0, dot) + ".meta.json";
    }
    return filename + ".meta.json";
}

[[nodiscard]] std::string spritesheet_meta_path(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        return filename.substr(0, dot) + ".spritesheet.json";
    }
    return filename + ".spritesheet.json";
}

// Parse a CSS colour string to uint32_t ARGB via the backend's color helper.
[[nodiscard]] uint32_t bg_color_to_uint32(const std::string& color_str) {
    return parse_color(color_str).value_or(0x00000000);
}

// Check a Result<void> and propagate any error.
[[nodiscard]] Result<void> check_result(Result<void>&& r) {
    if (!r)
        return std::unexpected(std::move(r.error()));
    return {};
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// SVG Path Parsing
// ═══════════════════════════════════════════════════════════════════════════════

auto parse_svg_path(const std::string& svg_path) -> Result<std::vector<PathCommand>> {
    std::vector<PathCommand> commands;

    // State machine for parsing
    enum class State { Command, Number, Done };
    State state = State::Command;

    size_t pos = 0;
    char current_cmd = 0;
    bool relative = false;

    // Buffer for reading numbers
    auto skip_whitespace = [&]() {
        while (pos < svg_path.size() &&
               (svg_path[pos] == ' ' || svg_path[pos] == '\t' || svg_path[pos] == '\n' ||
                svg_path[pos] == '\r' || svg_path[pos] == ',')) {
            ++pos;
        }
    };

    // Parse a single float from the current position
    auto parse_number = [&]() -> std::optional<double> {
        skip_whitespace();
        if (pos >= svg_path.size())
            return std::nullopt;

        const char* start = svg_path.data() + pos;
        char* end = nullptr;
        double val = std::strtod(start, &end);
        if (end == start)
            return std::nullopt; // no number parsed

        pos += static_cast<size_t>(end - start);
        return val;
    };

    // Parse N numbers into the args vector
    auto parse_n_numbers = [&](int n, std::vector<double>& args) -> bool {
        for (int i = 0; i < n; ++i) {
            auto num = parse_number();
            if (!num.has_value())
                return false;
            args.push_back(*num);
        }
        return true;
    };

    // Helper: get the expected arg count for a command type
    auto args_for_type = [](PathCommand::Type type) -> int {
        switch (type) {
        case PathCommand::Type::MoveTo:
            return 2;
        case PathCommand::Type::LineTo:
            return 2;
        case PathCommand::Type::HorizontalLineTo:
            return 1;
        case PathCommand::Type::VerticalLineTo:
            return 1;
        case PathCommand::Type::CurveTo:
            return 6;
        case PathCommand::Type::SmoothCurveTo:
            return 4;
        case PathCommand::Type::QuadTo:
            return 4;
        case PathCommand::Type::SmoothQuadTo:
            return 2;
        case PathCommand::Type::ArcTo:
            return 7;
        case PathCommand::Type::ClosePath:
            return 0;
        }
        return 0;
    };

    // Emit a command with its arguments
    auto emit_command = [&](PathCommand::Type type, bool rel, std::vector<double> args) {
        commands.push_back({type, rel, std::move(args)});
    };

    // Parse a command letter and set up the current command
    auto parse_command = [&]() -> bool {
        skip_whitespace();
        if (pos >= svg_path.size())
            return false;

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
        case 'M':
            return PathCommand::Type::MoveTo;
        case 'L':
            return PathCommand::Type::LineTo;
        case 'H':
            return PathCommand::Type::HorizontalLineTo;
        case 'V':
            return PathCommand::Type::VerticalLineTo;
        case 'C':
            return PathCommand::Type::CurveTo;
        case 'S':
            return PathCommand::Type::SmoothCurveTo;
        case 'Q':
            return PathCommand::Type::QuadTo;
        case 'T':
            return PathCommand::Type::SmoothQuadTo;
        case 'A':
            return PathCommand::Type::ArcTo;
        case 'Z':
            return PathCommand::Type::ClosePath;
        default:
            return std::nullopt;
        }
    };

    while (pos < svg_path.size()) {
        skip_whitespace();
        if (pos >= svg_path.size())
            break;

        char c = svg_path[pos];

        if (std::isalpha(static_cast<unsigned char>(c))) {
            // New command
            if (!parse_command()) {
                return std::unexpected(
                    syntax_error("invalid SVG path command at position " + std::to_string(pos)));
            }

            auto type = char_to_type(current_cmd);
            if (!type.has_value()) {
                return std::unexpected(
                    syntax_error("unknown SVG path command: " + std::string(1, current_cmd)));
            }

            if (*type == PathCommand::Type::ClosePath && current_cmd == 'Z') {
                emit_command(PathCommand::Type::ClosePath, false, {});
                continue;
            }

            int nargs = args_for_type(*type);
            std::vector<double> args;
            if (!parse_n_numbers(nargs, args)) {
                return std::unexpected(syntax_error("unexpected end of SVG path data"));
            }
            emit_command(*type, relative, std::move(args));

            // Handle implicit repeated commands
            // After M, subsequent coordinate pairs are implicit L
            // After all other commands, subsequent coordinate pairs repeat the same command
        } else if (c == '-' || c == '+' || c == '.' ||
                   std::isdigit(static_cast<unsigned char>(c))) {
            // Implicit repeated command — no letter needed
            if (current_cmd == 0) {
                return std::unexpected(
                    syntax_error("unexpected number in SVG path without preceding command"));
            }

            auto type = char_to_type(current_cmd);
            if (!type.has_value()) {
                return std::unexpected(syntax_error("invalid repeated SVG path command"));
            }

            int nargs = args_for_type(*type);
            std::vector<double> args;
            if (!parse_n_numbers(nargs, args)) {
                return std::unexpected(
                    syntax_error("unexpected end of SVG path data for implicit command"));
            }
            emit_command(*type, relative, std::move(args));
        } else {
            return std::unexpected(syntax_error("unexpected character '" + std::string(1, c) +
                                                "' in SVG path at position " +
                                                std::to_string(pos)));
        }
    }

    return commands;
}

// Render a single frame of a canvas to a freshly-created surface.
// Shared by static render() and animation render.
[[nodiscard]] static Result<std::unique_ptr<Surface>> render_frame(RenderBackend& backend,
                                                                   const Canvas& canvas) {
    // Update global 3D camera to match the canvas. By default, orthographic
    // size equals canvas height so that world units map 1:1 to screen pixels.
    if (canvas.height > 0) {
        current_camera().aspect =
            static_cast<double>(canvas.width) / static_cast<double>(canvas.height);
        if (current_camera().projection == Camera3D::Projection::Orthographic &&
            !current_camera().user_ortho_size) {
            current_camera().ortho_size = static_cast<double>(canvas.height);
        }
    }

    uint32_t bg = bg_color_to_uint32(canvas.bg_color);
    auto surface = backend.create_surface(canvas.width, canvas.height, bg);
    if (!surface) {
        return std::unexpected(resource_error("failed to create render surface"));
    }

    if (canvas.is_sprite) {
        AffineTransform center_t = AffineTransform::translate(
            static_cast<double>(canvas.width) / 2.0, static_cast<double>(canvas.height) * 0.45);
        for (const auto& obj : canvas.objects) {
            AffineTransform composed = center_t.compose(obj.transform);
            auto centered = obj.with_transform(composed);
            auto r = backend.draw(*surface, centered);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
    } else {
        for (const auto& obj : canvas.objects) {
            auto r = backend.draw(*surface, obj);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
    }

    return surface;
}

// Apply skeleton skin bindings to the objects of a canvas in-place.
// Used by both static renders and animation renders so that bind-skin
// takes effect without requiring an explicit animation timeline.
static void apply_skin_bindings_to_canvas(Canvas& canvas) {
    std::unordered_map<uint64_t, std::unordered_map<std::string, Value>> obj_mods;
    merge_skin_bindings(obj_mods);
    if (obj_mods.empty()) return;

    for (auto& obj : canvas.objects) {
        auto it = obj_mods.find(obj.id);
        if (it != obj_mods.end()) {
            obj = _apply_modifications(obj, it->second);
        }
    }
}

// Render an animated GIF/sequence by stepping the global timeline.
[[nodiscard]] static Result<std::string> render_animation(const std::string& filename,
                                                          const std::string& fmt,
                                                          std::shared_ptr<Canvas> canvas,
                                                          int fps,
                                                          double duration_override = 0.0) {
    if (fps <= 0)
        fps = 30;

    auto timeline = get_or_create_timeline();
    timeline->play();

    double total_duration = timeline->get_total_duration();
    fprintf(stderr, "[render] total_duration from timeline=%.2f, override=%.2f\n", total_duration, duration_override);
    if (duration_override > 0.0)
        total_duration = duration_override;
    if (total_duration <= 0.0)
        total_duration = 1.0;

    int num_frames = std::max(1, static_cast<int>(std::ceil(total_duration * fps)));
    fprintf(stderr, "[render] final total_duration=%.2f, num_frames=%d\n", total_duration, num_frames);

    RenderBackend& backend = BackendRegistry::instance().active();

    // Preserve original canvas objects so repeated renders are deterministic.
    auto original_objects = canvas->objects;

    std::vector<std::unique_ptr<Surface>> frame_surfaces;
    std::vector<Surface*> frame_ptrs;
    frame_surfaces.reserve(static_cast<size_t>(num_frames));
    frame_ptrs.reserve(static_cast<size_t>(num_frames));

    for (int i = 0; i < num_frames; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(fps);

        // Invoke per-frame hooks.  Hooks may call (canvas ...) to recreate
        // the canvas (e.g. every-frame demos), so re-fetch afterwards.
        for (const auto& hook : timeline->frame_hooks) {
            hook(t);
        }

        // Re-fetch canvas in case a hook replaced it (e.g. every-frame).
        canvas = current_canvas_ref();

        // Evaluate animations and build target_id -> {property -> value} map.
        auto modifications = timeline->evaluate_at(t);
        std::unordered_map<uint64_t, std::unordered_map<std::string, Value>> obj_mods;
        for (const auto& [target_id, prop, value] : modifications) {
            obj_mods[target_id][prop] = value;
        }

        // Merge skeleton skin bindings into the modification map.
        merge_skin_bindings(obj_mods);

        // Apply modifications to canvas objects in-place.
        for (auto& obj : canvas->objects) {
            auto it = obj_mods.find(obj.id);
            if (it != obj_mods.end()) {
                obj = _apply_modifications(obj, it->second);
            }
        }

        // Render this frame.
        auto frame = render_frame(backend, *canvas);
        if (!frame)
            return std::unexpected(std::move(frame.error()));

        frame_ptrs.push_back(frame->get());
        frame_surfaces.push_back(std::move(*frame));
    }

    // Restore original object state for deterministic re-renders.
    canvas->objects = std::move(original_objects);

    timeline->current_time = total_duration;
    timeline->state = TimelineState::Finished;

    auto sr = backend.save_animation(frame_ptrs, filename, fmt, fps);
    if (!sr)
        return std::unexpected(std::move(sr.error()));

    PMLContext::current().output_files.push_back(filename);
    return filename;
}

auto render(const std::string& filename,
            const std::string& fmt,
            std::shared_ptr<Canvas> canvas,
            int fps,
            double duration_override) -> Result<std::string> {
    if (!canvas)
        canvas = get_current_canvas();
    if (!canvas)
        canvas = std::make_shared<Canvas>(256, 256, "transparent");

    const std::string resolved = resolve_output_path(filename);
    const std::string format = detect_format(resolved, fmt);

    // Determine whether this should be an animation render.
    bool has_animation = false;
    auto timeline = get_or_create_timeline();
    if (timeline && !timeline->animations.empty()) {
        has_animation = true;
    }

    if (fps > 0 || (has_animation && format == "GIF")) {
        int effective_fps = (fps > 0) ? fps : 30;
        return render_animation(resolved, format, canvas, effective_fps, duration_override);
    }

    RenderBackend& backend = BackendRegistry::instance().active();

    // Preserve original canvas objects so skin binding application is non-destructive.
    auto original_objects = canvas->objects;
    apply_skin_bindings_to_canvas(*canvas);

    auto surface_result = render_frame(backend, *canvas);
    canvas->objects = std::move(original_objects);
    if (!surface_result)
        return std::unexpected(std::move(surface_result.error()));
    auto& surface = *surface_result;

    ensure_parent_dir(resolved);
    auto sr = backend.save_image(*surface, resolved, format);
    if (!sr)
        return std::unexpected(std::move(sr.error()));

    PMLContext::current().output_files.push_back(resolved);

    if (canvas->is_sprite) {
        json meta;
        meta["file"] = resolved;
        meta["width"] = canvas->width;
        meta["height"] = canvas->height;
        meta["anchor"] = canvas->anchor;
        meta["padding"] = canvas->padding;
        meta["format"] = format;
        meta["pml_version"] = "0.1.0";
        auto wj = write_json_file(meta_path(resolved), meta);
        if (!wj) { /* non-fatal — match Python behaviour */
        }
        PMLContext::current().output_files.push_back(meta_path(resolved));
    }

    return resolved;
}

auto render_set(const std::string& name,
                const GraphicObject& content,
                const std::vector<int>& scales,
                int base_width,
                int base_height,
                const std::string& fmt) -> Result<std::vector<std::string>> {
    if (scales.empty())
        return std::vector<std::string>{};

    const std::string resolved_name = resolve_output_path(name);

    RenderBackend& backend = BackendRegistry::instance().active();
    std::vector<std::string> results;
    results.reserve(scales.size());

    for (int s : scales) {
        const int w = base_width * s;
        const int h = base_height * s;

        uint32_t bg = bg_color_to_uint32("transparent");
        auto surface = backend.create_surface(w, h, bg);
        if (!surface) {
            return std::unexpected(resource_error("failed to create render-set surface at " +
                                                  std::to_string(s) + "x"));
        }

        AffineTransform scale_t =
            AffineTransform::scale(static_cast<double>(s), static_cast<double>(s));
        AffineTransform new_t = scale_t.compose(content.transform);
        GraphicObject scaled = content.with_transform(new_t);

        auto dr = backend.draw(*surface, scaled);
        if (!dr)
            return std::unexpected(std::move(dr.error()));

        std::string suffix = "@" + std::to_string(s) + "x";
        std::string ext;
        if (fmt == "PNG")
            ext = ".png";
        else if (fmt == "JPEG")
            ext = ".jpg";
        else if (fmt == "GIF")
            ext = ".gif";
        else if (fmt == "SVG")
            ext = ".svg";
        else
            ext = "." + fmt;

        std::string out_name = resolved_name + suffix + ext;
        auto sr = backend.save_image(*surface, out_name, fmt);
        if (!sr)
            return std::unexpected(std::move(sr.error()));

        PMLContext::current().output_files.push_back(out_name);
        results.push_back(std::move(out_name));
    }

    return results;
}

auto render_spritesheet(const std::string& filename,
                        const std::vector<GraphicObject>& sprites,
                        int cols,
                        int cell_width,
                        int cell_height,
                        int padding,
                        const std::string& bg) -> Result<std::string> {
    if (sprites.empty())
        return filename;

    const std::string resolved = resolve_output_path(filename);

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
            if (!dr)
                return std::unexpected(std::move(dr.error()));

            auto cr = backend.composite(*surface, *cell_surface, x, y);
            if (!cr)
                return std::unexpected(std::move(cr.error()));
        }

        frames.push_back({
            {"index", i},
            {"x", x},
            {"y", y},
            {"width", cell_width},
            {"height", cell_height},
            {"col", col},
            {"row", row},
        });
    }

    const std::string format = detect_format(resolved, "PNG");

    auto sr = backend.save_image(*surface, resolved, format);
    if (!sr)
        return std::unexpected(std::move(sr.error()));

    PMLContext::current().output_files.push_back(resolved);

    json meta;
    meta["file"] = resolved;
    meta["format"] = format;
    meta["total_width"] = total_w;
    meta["total_height"] = total_h;
    meta["cols"] = cols;
    meta["rows"] = rows;
    meta["cell_width"] = cell_width;
    meta["cell_height"] = cell_height;
    meta["padding"] = padding;
    meta["frames"] = std::move(frames);
    meta["pml_version"] = "0.1.0";

    auto wj = write_json_file(spritesheet_meta_path(resolved), meta);
    if (!wj) { /* non-fatal */
    }
    PMLContext::current().output_files.push_back(spritesheet_meta_path(resolved));

    return resolved;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Environment Registration — bind render functions to PML
// ═══════════════════════════════════════════════════════════════════════════════
//
// Each function is wrapped in a BuiltinProcedure with accepts_kwargs=true.
// The callback parses arguments from PML Value vectors and dispatches to
// the C++ render functions above.
// ═══════════════════════════════════════════════════════════════════════════════

void register_render(std::shared_ptr<Environment> env) {
    if (!env)
        return;

    // ── (render filename :format "PNG" ...) ──────────────────────────────
    //
    // Signature: (render filename [kwargs...])
    //   - filename: string (required, first positional arg)
    //   - :format: string (optional, output format)
    // Renders the current canvas to a file.
    {
        auto render_fn = [](const std::vector<Value>& args, Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            // First arg: filename
            auto filename = value_to_string_req(args[0]);
            if (!filename)
                return std::unexpected(std::move(filename.error()));

            // Remaining args: kwargs
            std::string fmt;
            int fps = 0;
            double duration_override = 0.0;
            if (args.size() > 1) {
                auto kwargs = parse_kwargs(args, 1);
                fmt = kw_string(kwargs, "format", "");
                if (auto it = kwargs.find("fps"); it != kwargs.end()) {
                    if (it->second.is_int()) {
                        fps = static_cast<int>(it->second.int_val());
                    } else if (it->second.is_double()) {
                        fps = static_cast<int>(it->second.double_val());
                    }
                }
                if (auto it = kwargs.find("duration"); it != kwargs.end()) {
                    if (it->second.is_int()) {
                        duration_override = static_cast<double>(it->second.int_val());
                    } else if (it->second.is_double()) {
                        duration_override = it->second.double_val();
                    }
                }
            }

            auto result = render(*filename, fmt, nullptr, fps, duration_override);
            if (!result)
                return std::unexpected(std::move(result.error()));

            return Value(std::move(*result));
        };

        env->define(
            "render",
            Value(std::make_shared<BuiltinProcedure>("render", std::move(render_fn), true)));
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
                                Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            auto name = value_to_string_req(args[0]);
            if (!name)
                return std::unexpected(std::move(name.error()));

            auto kwargs = parse_kwargs(args, 1);

            // Extract :content (required)
            std::shared_ptr<GraphicObject> content_obj;
            if (auto it = kwargs.find("content"); it != kwargs.end()) {
                if (auto* go = it->second.as_graphic_object()) {
                    content_obj = *go;
                } else {
                    return std::unexpected(
                        type_error("render-set :content expects a GraphicObject, got " +
                                   value_to_string(it->second)));
                }
            }

            if (!content_obj) {
                return std::unexpected(
                    type_error("render-set requires :content kwarg with a GraphicObject"));
            }

            // Extract :scales (default {1, 2, 4})
            std::vector<int> scales = {1, 2, 4};
            if (auto it = kwargs.find("scales"); it != kwargs.end()) {
                scales.clear();
                if (auto* lst = it->second.as_list()) {
                    for (const auto& elem : lst->get()->elements) {
                        if (elem.is_int()) {
                            scales.push_back(static_cast<int>(elem.int_val()));
                        }
                    }
                }
                if (scales.empty())
                    scales = {1, 2, 4};
            }

            // Extract :base-size (default {64, 64})
            int base_w = 64, base_h = 64;
            if (auto it = kwargs.find("base-size"); it != kwargs.end()) {
                if (auto* lst = it->second.as_list()) {
                    const auto& elems = lst->get()->elements;
                    if (elems.size() >= 1 && elems[0].is_int())
                        base_w = static_cast<int>(elems[0].int_val());
                    if (elems.size() >= 2 && elems[1].is_int())
                        base_h = static_cast<int>(elems[1].int_val());
                }
            }

            std::string fmt = kw_string(kwargs, "format", "PNG");

            auto results = render_set(*name, *content_obj, scales, base_w, base_h, fmt);
            if (!results)
                return std::unexpected(std::move(results.error()));

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
                                 Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            auto filename = value_to_string_req(args[0]);
            if (!filename)
                return std::unexpected(std::move(filename.error()));

            // Collect sprites from positional args (after filename, before first Keyword)
            std::vector<GraphicObject> sprites;
            size_t kw_start = args.size();
            for (size_t i = 1; i < args.size(); ++i) {
                if (args[i].is_keyword() || args[i].is_symbol()) {
                    kw_start = i;
                    break;
                }
                if (auto* go = args[i].as_graphic_object()) {
                    sprites.push_back(**go); // dereference shared_ptr to copy
                }
            }

            auto kwargs = parse_kwargs(args, kw_start);

            int cols = kw_int(kwargs, "cols", 4);
            int cell_w = kw_int(kwargs, "cell-width", 64);
            int cell_h = kw_int(kwargs, "cell-height", 64);
            int padding = kw_int(kwargs, "padding", 2);
            std::string bg = kw_string(kwargs, "bg", "transparent");

            auto result = render_spritesheet(*filename, sprites, cols, cell_w, cell_h, padding, bg);
            if (!result)
                return std::unexpected(std::move(result.error()));

            return Value(std::move(*result));
        };

        env->define("render-spritesheet",
                    Value(std::make_shared<BuiltinProcedure>(
                        "render-spritesheet", std::move(spritesheet_fn), true)));
    }

    // ── (render-tilemap name :output "out.png" [:bg "transparent"]) ──────
    //
    // Signature: (render-tilemap name [kwargs...])
    //   - name: string or symbol (tilemap name, required, first positional)
    //   - :output: string (required, output file path)
    //   - :bg: string (optional, background colour, default "transparent")
    //
    // Looks up the named tilemap and its associated tileset, builds a
    // rendering surface, places each visible tile as a GraphicObject, and
    // saves the result as PNG via the active backend.
    {
        auto tilemap_fn = [](const std::vector<Value>& args,
                             Environment& /*env*/) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
            }

            // First positional: tilemap name
            auto tm_name_opt = pml::kwargs::value_to_opt_string(args[0]);
            if (!tm_name_opt) {
                return std::unexpected(
                    type_error("render-tilemap: first argument must be a string or symbol (tilemap name)"));
            }

            // Parse kwargs starting at position 1
            auto kwargs = pml::kwargs::parse_kwargs(args, 1);

            // :output kwarg (required)
            std::string output = pml::kwargs::kw_string(kwargs, "output", "");
            if (output.empty()) {
                return std::unexpected(
                    type_error("render-tilemap: :output keyword argument is required"));
            }

            // :bg kwarg (optional, default "transparent")
            std::string bg = pml::kwargs::kw_string(kwargs, "bg", "transparent");

            // Look up the tilemap
            Tilemap* tm = TilemapManager::instance().lookup_tilemap(*tm_name_opt);
            if (!tm) {
                return std::unexpected(
                    type_error(std::format("render-tilemap: unknown tilemap '{}'", *tm_name_opt)));
            }

            // Look up the tileset associated with this tilemap
            const Tileset* ts = TilesetManager::instance().lookup_tileset(tm->tileset_name);
            if (!ts) {
                return std::unexpected(
                    type_error(std::format("render-tilemap: tileset '{}' not found for tilemap '{}'",
                                           tm->tileset_name, *tm_name_opt)));
            }

            // Build a fast tile_id -> TileType* lookup
            std::unordered_map<int, const TileType*> tile_lookup;
            tile_lookup.reserve(ts->tile_types.size());
            for (const auto& tt : ts->tile_types) {
                tile_lookup[tt.id] = &tt;
            }

            // :projection kwarg (optional, default 'orthogonal)
            std::string projection = pml::kwargs::kw_string(kwargs, "projection", "orthogonal");

            // Calculate output pixel dimensions
            int pixel_w, pixel_h;
            if (projection == "isometric") {
                // Isometric bounds: the diamond spans (cols+rows)*tile_size/2 in each axis
                pixel_w = (tm->cols + tm->rows) * tm->tile_size / 2;
                pixel_h = (tm->cols + tm->rows) * tm->tile_size / 2;
                if (pixel_w < 8) pixel_w = 8;
                if (pixel_h < 8) pixel_h = 8;
            } else {
                // Orthogonal: simple grid dimensions
                pixel_w = tm->cols * tm->tile_size;
                pixel_h = tm->rows * tm->tile_size;
            }
            if (pixel_w <= 0 || pixel_h <= 0) {
                return std::unexpected(
                    type_error("render-tilemap: tilemap has zero pixel dimensions"));
            }

            // Create a canvas with the tilemap dimensions
            auto canvas = std::make_shared<Canvas>(pixel_w, pixel_h, bg);

            // Helper: collect non-empty tile entries for a layer
            struct TileEntry {
                const TileType* tile;
                int col;
                int row;
            };

            if (projection == "isometric") {
                // ── Isometric projection ─────────────────────────────────
                // Uses Painter's Algorithm: sort non-empty tiles by
                // (row + col) ascending and render back-to-front.
                //
                // Screen position for isometric tile (col, row):
                //   sx = (col - row) * tile_size / 2  +  pixel_w / 2
                //   sy = (col + row) * tile_size / 4
                double half_w = static_cast<double>(pixel_w) / 2.0;
                double ts = static_cast<double>(tm->tile_size);

                // Render each visible layer bottom-to-top
                for (const auto& layer : tm->layers) {
                    if (!layer.visible) continue;

                    // Collect non-empty tiles
                    std::vector<TileEntry> entries;
                    entries.reserve(static_cast<size_t>(layer.rows * layer.cols));
                    for (int row = 0; row < layer.rows; ++row) {
                        for (int col = 0; col < layer.cols; ++col) {
                            int tile_id = tile_at(layer, col, row);
                            if (tile_id == 0) continue;
                            auto it = tile_lookup.find(tile_id);
                            if (it == tile_lookup.end()) continue;
                            entries.push_back({it->second, col, row});
                        }
                    }

                    // Depth sort by (row + col) ascending (Painter's Algorithm)
                    std::ranges::sort(entries, [](const TileEntry& a, const TileEntry& b) {
                        return (a.row + a.col) < (b.row + b.col);
                    });

                    // Render sorted tiles
                    for (const auto& entry : entries) {
                        double sx = (static_cast<double>(entry.col - entry.row)) * ts / 2.0 + half_w;
                        double sy = (static_cast<double>(entry.col + entry.row)) * ts / 4.0;

                        GraphicObject positioned = entry.tile->graphic.with_transform(
                            AffineTransform::translate(sx, sy));
                        canvas->add(positioned);

                        if (entry.tile->detail.has_value()) {
                            GraphicObject detail_pos = entry.tile->detail->with_transform(
                                AffineTransform::translate(sx, sy));
                            canvas->add(detail_pos);
                        }
                    }
                }
            } else {
                // ── Orthogonal projection (default) ──────────────────────
                // Simple grid layout: tile at (col, row) → pixel (col*S, row*S)
                for (const auto& layer : tm->layers) {
                    if (!layer.visible) continue;

                    for (int row = 0; row < layer.rows; ++row) {
                        for (int col = 0; col < layer.cols; ++col) {
                            int tile_id = tile_at(layer, col, row);
                            if (tile_id == 0) continue;  // empty tile

                            auto it = tile_lookup.find(tile_id);
                            if (it == tile_lookup.end()) continue;  // unknown tile id

                            const TileType* tile = it->second;
                            double x = static_cast<double>(col * tm->tile_size);
                            double y = static_cast<double>(row * tm->tile_size);

                            // Add the base graphic at the tile position
                            GraphicObject positioned = tile->graphic.with_transform(
                                AffineTransform::translate(x, y));
                            canvas->add(positioned);

                            // Add detail overlay (if present) at the same position
                            if (tile->detail.has_value()) {
                                GraphicObject detail_pos = tile->detail->with_transform(
                                    AffineTransform::translate(x, y));
                                canvas->add(detail_pos);
                            }
                        }
                    }
                }
            }

            // Use the existing render() function to save to file
            auto r = render(output, "PNG", canvas);
            if (!r) return std::unexpected(std::move(r.error()));
            return Value(std::move(*r));
        };

        env->define("render-tilemap",
                    Value(std::make_shared<BuiltinProcedure>(
                        "render-tilemap", std::move(tilemap_fn), true)));
    }
}

} // namespace pml
