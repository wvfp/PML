// ==========================================================================================================================================================================================================================================═
// PML Render Channels Builtins — Implementation
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Implements (render-channels ...) for multi-channel sprite export.
//
// Channel strategies:
//   albedo   — Render the GraphicObject as-is (no modification).
//   specular — Traverse the GraphicObject tree and replace every fill/stroke
//              colour with its luminance value L = 0.299R + 0.587G + 0.114B.
//              Alpha from the original colour is preserved.
//   normal   — Traverse the GraphicObject tree and replace every fill/stroke
//              colour with the default Z-normal RGB(128, 128, 255).
//              Alpha from the original colour is preserved.
// ==========================================================================================================================================================================================================================================═

#include "render_channels_builtins.h"

#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/backend/color_helpers.h"
#include "pml/graphics/objects.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/core/types.h"
#include "pml/api/context.h" // PMLContext::current().output_files
#include "environment.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <vector>

namespace pml {

using pml::kwargs::kw_int;
using pml::kwargs::kw_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::value_to_opt_string;

namespace {

// ==========================================================================================================================================================================================================================================═
// Internal helpers
// ==========================================================================================================================================================================================================================================═

/// Ensure the parent directory of a file path exists.
static void ensure_parent_dir(const std::string& filepath) {
    namespace fs = std::filesystem;
    fs::path p(filepath);
    auto parent = p.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        fs::create_directories(parent, ec);
    }
}

// ==========================================================================================================================================================================================================================================═
// Channel: specular — replace colours with luminance values
// ==========================================================================================================================================================================================================================================═

/// Replace fill/stroke colours in a GraphicObject tree with their luminance
/// values.  Luminance = 0.299R + 0.587G + 0.114B.  Alpha is preserved from
/// the original colour.  Objects without a fill or stroke are left untouched.
///
/// This is applied *before* rendering so that any backend can produce the
/// specular output (no post-render pixel manipulation needed).
static void replace_colors_luminance(GraphicObject& obj) {
    if (obj.fill.has_value()) {
        auto parsed = parse_color(*obj.fill);
        if (parsed) {
            uint32_t argb = *parsed;
            int a = (argb >> 24) & 0xFF;
            int r = (argb >> 16) & 0xFF;
            int g = (argb >> 8) & 0xFF;
            int b = argb & 0xFF;
            uint8_t l = static_cast<uint8_t>(std::clamp(
                static_cast<int>(0.299f * r + 0.587f * g + 0.114f * b), 0, 255));
            obj.fill = std::format("#{:02X}{:02X}{:02X}{:02X}", l, l, l, a);
        }
    }
    if (obj.stroke.has_value()) {
        auto parsed = parse_color(*obj.stroke);
        if (parsed) {
            uint32_t argb = *parsed;
            int a = (argb >> 24) & 0xFF;
            int r = (argb >> 16) & 0xFF;
            int g = (argb >> 8) & 0xFF;
            int b = argb & 0xFF;
            uint8_t l = static_cast<uint8_t>(std::clamp(
                static_cast<int>(0.299f * r + 0.587f * g + 0.114f * b), 0, 255));
            obj.stroke = std::format("#{:02X}{:02X}{:02X}{:02X}", l, l, l, a);
        }
    }
    for (auto& child : obj.children) {
        replace_colors_luminance(child);
    }
}

// ==========================================================================================================================================================================================================================================═
// Channel: normal — replace colours with default Z-normal
// ==========================================================================================================================================================================================================================================═

/// Replace fill/stroke colours in a GraphicObject tree with the default normal
/// map colour RGB(128, 128, 255) — representing a Z-normal (facing-forward
/// surface normal for 2D shapes).  Alpha from the original colour is preserved
/// so that transparent regions remain transparent in the normal map.
static void replace_colors_normal(GraphicObject& obj) {
    if (obj.fill.has_value()) {
        auto parsed = parse_color(*obj.fill);
        if (parsed) {
            uint32_t argb = *parsed;
            int a = (argb >> 24) & 0xFF;
            obj.fill = std::format("#8080FF{:02X}", a);
        } else {
            obj.fill = "#8080FF";
        }
    }
    if (obj.stroke.has_value()) {
        auto parsed = parse_color(*obj.stroke);
        if (parsed) {
            uint32_t argb = *parsed;
            int a = (argb >> 24) & 0xFF;
            obj.stroke = std::format("#8080FF{:02X}", a);
        } else {
            obj.stroke = "#8080FF";
        }
    }
    for (auto& child : obj.children) {
        replace_colors_normal(child);
    }
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_render_channels(std::shared_ptr<Environment> env) {
    if (!env) return;

    // ---- (render-channels graphic :output "path" [:channels ...] [:width W] [:height H] [:bg "transparent"]) ----
    //
    // Signature:   (render-channels graphic [kwargs...])
    //   graphic:   GraphicObject (required, first positional)
    //   :output:   string (required) — file path prefix; each channel appends
    //              `_channel.png` to produce e.g. "sprite_albedo.png".
    //   :channels: list of channel symbols (optional, default '(albedo specular normal))
    //   :width:    int (optional) — output width in pixels (default 64).
    //   :height:   int (optional) — output height in pixels (default 64).
    //   :bg:       string (optional) — background colour (default "transparent").
    //
    // Returns a list of output filenames created.
    //
    // Error cases:
    //   - First argument is not a GraphicObject          → PMLTypeError
    //   - :output is missing or empty                    → PMLTypeError
    //   - Unknown channel name in :channels list         → PMLTypeError
    //   - Surface creation or image save fails           → PMLResourceError
    auto render_channels_fn = [](const std::vector<Value>& args,
                                  Environment& /*env*/) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }

        // ---- First positional arg: GraphicObject ----------------------------------------------------
        const auto* graphic_ptr = args[0].as_graphic_object();
        if (!graphic_ptr || !*graphic_ptr) {
            return std::unexpected(
                type_error("render-channels: first argument must be a GraphicObject, got " +
                           value_to_string(args[0])));
        }
        const GraphicObject& graphic = **graphic_ptr;

        // ---- Parse kwargs starting at position 1 ----------------------------------------------------
        auto kwargs = parse_kwargs(args, 1);

        // :output (required)
        std::string output_prefix = kw_string(kwargs, "output", "");
        if (output_prefix.empty()) {
            return std::unexpected(
                type_error("render-channels: :output keyword argument is required"));
        }

        // :channels (optional, default '(albedo specular normal))
        std::vector<std::string> channels;
        if (auto it = kwargs.find("channels"); it != kwargs.end()) {
            const auto* lst = it->second.as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("render-channels: :channels must be a list of symbols, got " +
                               value_to_string(it->second)));
            }
            for (const auto& elem : (*lst)->elements) {
                auto ch_name = value_to_opt_string(elem);
                if (!ch_name) {
                    return std::unexpected(
                        type_error("render-channels: each channel must be a symbol, got " +
                                   value_to_string(elem)));
                }
                if (*ch_name != "albedo" && *ch_name != "specular" && *ch_name != "normal") {
                    return std::unexpected(
                        type_error("render-channels: unknown channel '" + *ch_name +
                                   "'. Valid channels: albedo, specular, normal"));
                }
                channels.push_back(std::move(*ch_name));
            }
        } else {
            channels = {"albedo", "specular", "normal"};
        }

        // :width / :height (optional, default 64)
        int width = kw_int(kwargs, "width", 64);
        int height = kw_int(kwargs, "height", 64);
        if (width < 1) width = 1;
        if (height < 1) height = 1;

        // :bg (optional, default "transparent")
        std::string bg = kw_string(kwargs, "bg", "transparent");
        uint32_t bg_color = parse_color(bg).value_or(0x00000000);

        // ---- Render each channel ------------------------------------------------------------------------------------
        RenderBackend& backend = BackendRegistry::instance().active();

        std::vector<std::string> results;
        results.reserve(channels.size());

        for (const auto& channel : channels) {
            // Make a deep copy of the GraphicObject tree for this channel.
            GraphicObject channel_graphic = graphic;

            // Apply channel-specific colour transformation.
            if (channel == "specular") {
                replace_colors_luminance(channel_graphic);
            } else if (channel == "normal") {
                replace_colors_normal(channel_graphic);
            }
            // "albedo" renders as-is — no modification.

            // Create the render surface.
            auto surface = backend.create_surface(width, height, bg_color);
            if (!surface) {
                return std::unexpected(
                    resource_error("render-channels: failed to create surface for channel '" +
                                   channel + "'"));
            }

            // Draw the (possibly modified) graphic onto the surface.
            auto dr = backend.draw(*surface, channel_graphic);
            if (!dr) {
                return std::unexpected(std::move(dr.error()));
            }

            // Build output path: {prefix}_{channel}.png
            std::string output_path = output_prefix + "_" + channel + ".png";

            // Ensure the parent directory exists.
            ensure_parent_dir(output_path);

            // Save as PNG via the active backend.
            auto sr = backend.save_image(*surface, output_path, "PNG");
            if (!sr) {
                return std::unexpected(std::move(sr.error()));
            }

            PMLContext::current().output_files.push_back(output_path);
            results.push_back(std::move(output_path));
        }

        // ---- Return list of created filenames --------------------------------------------------------─
        auto val_list = std::make_shared<ValueList>();
        for (auto& f : results) {
            val_list->elements.push_back(Value(std::move(f)));
        }
        return Value(std::move(val_list));
    };

    auto proc = std::make_shared<BuiltinProcedure>(
        "render-channels", std::move(render_channels_fn), true);
    env->define("render-channels", Value(std::move(proc)));
}

} // namespace pml
