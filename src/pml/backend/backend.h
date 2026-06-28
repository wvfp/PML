#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Render Backend — Abstract Base Class
// ==========================================================================================================================================================================================================================================═
//
// All render backends (Null, Skia, Cairo, etc.) inherit from RenderBackend
// and implement the pure virtual methods below.
//
// Surface is a base struct with virtual destructor; backends subclass it to
// store their own pixel/raster/context data.
// ==========================================================================================================================================================================================================================================═

#include "capabilities.h"
#include "pml/core/error.h"
#include "pml/core/types.h"
#include "pml/filter/filter_backend.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// Forward declarations
// ==========================================================================================================================================================================================================================================═

struct GraphicObject;  ///< Defined in pml/graphics/graphic_object.h (Task 10)
enum class BlendMode : uint8_t; ///< Defined in pml/layer/blend_mode.h

// ==========================================================================================================================================================================================================================================═
// Surface — abstract render target
// ==========================================================================================================================================================================================================================================═

/// Abstract render target (canvas / raster image).
/// Each backend defines its own surface by inheriting from this struct.
struct Surface {
    int width{};     ///< Surface width in pixels
    int height{};    ///< Surface height in pixels

    Surface() = default;
    Surface(int w, int h) : width(w), height(h) {}
    virtual ~Surface() = default;

    // Non-copyable, movable.
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface(Surface&&) = default;
    Surface& operator=(Surface&&) = default;
};

// ==========================================================================================================================================================================================================================================═
// ShaderChildInfo — descriptor for a child shader input (backend-agnostic)
// ==========================================================================================================================================================================================================================================═

/// Describes a child shader image input for shaders that accept child
/// shader references (e.g., SkSL shaders with `in shader child`).
struct ShaderChildInfo {
    int width{};   ///< Width of the child shader image in pixels
    int height{};  ///< Height of the child shader image in pixels
};

/// Describes a single uniform from a compiled shader (for introspection).
struct ShaderUniformInfo {
    std::string name;          ///< Uniform name as declared in SkSL
    std::string type_name;     ///< Type as string ("float", "float2", "int", etc.)
    size_t offset{};           ///< Byte offset in uniform data block
    size_t size_in_bytes{};    ///< Size of this uniform in bytes
};

/// Describes a single child shader slot from a compiled shader (for introspection).
struct ShaderChildSlotInfo {
    std::string name;          ///< Child slot name as declared in SkSL
    std::string type_name;     ///< Type as string ("shader", "colorfilter", "blender")
    int index{};               ///< Slot index for binding
};

// ==========================================================================================================================================================================================================================================═
// RenderBackend — abstract base class
// ==========================================================================================================================================================================================================================================═

/// Abstract base class for all render backends.
///
/// Methods use `Result<T>` (std::expected) for error reporting. A method
/// returning `Result<void>` succeeds with an empty expected or fails with
/// a PMLException describing what went wrong.
class RenderBackend : public FilterBackend {
public:
    ~RenderBackend() override = default;

    // ---- Metadata --------------------------------------------------------------------------------------------------------

    /// Return metadata describing this backend (name, description, capabilities).
    [[nodiscard]] virtual auto info() const -> BackendInfo = 0;

    // ---- Surface lifecycle ------------------------------------------------------------------------------------─

    /// Create a new surface (render target) with the given dimensions
    /// and background colour (packed as 0xRRGGBBAA).
    [[nodiscard]] virtual auto create_surface(
        int width, int height, uint32_t bg_color) -> std::unique_ptr<Surface> = 0;

    // ---- Drawing --------------------------------------------------------------------------------------------------------─

    /// Draw a GraphicObject onto the given surface.
    virtual auto draw(Surface& surface, const GraphicObject& obj) -> Result<void> = 0;

    // ---- Output ------------------------------------------------------------------------------------------------------------

    /// Save a surface to an image file. Format hint (e.g. "png", "jpg")
    /// may be backend-dependent.
    virtual auto save_image(
        Surface& surface,
        const std::string& path,
        const std::string& format) -> Result<void> = 0;

    /// Save a sequence of surfaces as an animated image (e.g. GIF).
    virtual auto save_animation(
        const std::vector<Surface*>& frames,
        const std::string& path,
        const std::string& format,
        int fps) -> Result<void> = 0;

    /// Load an image file into a backend-specific Surface.
    /// The caller owns the returned surface.
    [[nodiscard]] virtual auto load_image(
        const std::string& path) -> Result<std::unique_ptr<Surface>> = 0;

    // ---- Compositing --------------------------------------------------------------------------------------------------------

    /// Composite a source surface onto a destination surface at (x, y).
    /// The source surface is blended onto the destination using alpha
    /// compositing.  Used by spritesheet rendering.
    virtual auto composite(
        Surface& dst, Surface& src, int x, int y) -> Result<void> = 0;

    /// Composite with a specific blend mode and global opacity.
    virtual auto composite_with_blend(
        Surface& dst, Surface& src, int x, int y,
        BlendMode blend, float opacity) -> Result<void> = 0;

    /// Use the alpha channel of `mask` to modulate the alpha of `dst`.
    /// dst pixel alpha becomes dst_alpha * (mask_alpha / 255).
    virtual auto apply_mask(Surface& dst, Surface& mask) -> Result<void> = 0;

    /// Draw a GraphicObject into a fresh surface of the requested size.
    /// Used by the compositor to capture a layer as a raster image.
    [[nodiscard]] virtual auto draw_to_new_surface(
        const GraphicObject& obj,
        int width, int height,
        uint32_t bg_color) -> Result<std::unique_ptr<Surface>> = 0;

    /// True if this backend supports non-Normal blend modes.
    [[nodiscard]] virtual auto supports_blend_mode() const noexcept -> bool = 0;

    /// True if this backend can render layers to off-screen surfaces.
    [[nodiscard]] virtual auto supports_layer_compositing() const noexcept -> bool = 0;

    // ---- Shaders --------------------------------------------------------------------------------------------------------─

    /// Compile a GLSL shader string. Returns a backend-specific shader
    /// handle (opaque integer) on success, or an error on failure.
    virtual auto compile_shader(const std::string& glsl) -> Result<uint64_t> = 0;

    // ---- Noise Shaders ------------------------------------------------------------------------------------------------

    /// Noise shader type.
    enum class NoiseType {
        Fractal,    ///< Fractal noise (smooth, continuous)
        Turbulence,  ///< Turbulence noise (harsher, more variation)
    };

    /// Create a Perlin noise shader.
    /// @param type         Noise type (Fractal or Turbulence)
    /// @param base_freq_x  Base frequency X (usually 0.0 - 1.0)
    /// @param base_freq_y  Base frequency Y (usually 0.0 - 1.0)
    /// @param octaves      Number of octaves (1-8 typical)
    /// @param seed         Random seed
    /// @param tile_w       Tile width for seamless noise (0 = no tiling)
    /// @param tile_h       Tile height for seamless noise (0 = no tiling)
    /// @param lacunarity   Frequency multiplier per octave (default 2.0)
    /// @param persistence  Amplitude multiplier per octave (default 0.5)
    /// @return             Shader handle on success
    virtual auto create_noise_shader(NoiseType type,
                                    float base_freq_x, float base_freq_y,
                                    int octaves, float seed,
                                    int tile_w, int tile_h,
                                    float lacunarity = 2.0f,
                                    float persistence = 0.5f)
        -> Result<uint64_t>
    {
        (void)lacunarity;
        (void)persistence;
        return std::unexpected(general_error(
            "noise shaders not supported by this backend"));
    }

    /// Create a shader from an image file (PNG, etc.).
    /// The resulting shader can be used as a child shader input (e.g. with
    /// compose-with-child) or drawn via (rect ... :shader handle).
    /// @param path  Path to an image file
    /// @return      Shader handle on success (preshader_cache)
    virtual auto create_image_shader(const std::string& path)
        -> Result<uint64_t>
    {
        (void)path;
        return std::unexpected(general_error(
            "image shaders not supported by this backend"));
    }

    // ---- Shader Uniforms ----------------------------------------------------------------------------------------─

    /// Create a shader with uniform values bound.
    /// SkSL uniforms are passed as a raw byte vector (layout matches SkRuntimeEffect).
    /// @param shader_handle  Handle from compile_shader
    /// @param uniform_data  Raw uniform data (floats aligned to 4 bytes)
    /// @return              New shader handle with uniforms bound
    virtual auto create_shader_with_uniforms(uint64_t shader_handle,
                                             const std::vector<uint8_t>& uniform_data)
        -> Result<uint64_t>
    {
        return std::unexpected(general_error(
            "shader uniforms not supported by this backend"));
    }

    /// Create a shader with child shader inputs.
    /// SkSL shaders with `in shader child` parameters need child shader
    /// image descriptions to be bound alongside uniforms.
    /// @param src          SkSL shader source string
    /// @param childDescs   Descriptions of child shader image inputs
    /// @param uniforms     Uniform values to bind
    /// @return             A Value representing the created shader, or an error
    virtual auto create_shader_with_children(
        const std::string& src,
        const std::vector<ShaderChildInfo>& childDescs,
        const std::vector<Value>& uniforms) -> Result<Value>
    {
        return std::unexpected(general_error(
            "create_shader_with_children not supported by this backend"));
    }

    /// Compose an existing preshader (from preshader_cache_) as a child of a new
    /// SkSL wrapper shader.  The wrapper SkSL must declare `uniform shader src;`
    /// as a child slot; the preshader is bound to that slot.
    /// This lets us wrap noise shaders (or any baked shader) with custom SkSL code
    /// (e.g. color quantization) without rasterising the source to an intermediate image.
    /// @param preshader_handle  Handle returned by create_noise_shader or
    ///                          create_shader_with_uniforms (preshader_cache)
    /// @param sksl_wrapper_src  SkSL source with `uniform shader src;` child slot
    /// @return                  New shader handle for the composed shader
    virtual auto compose_with_child_shader(
        uint64_t preshader_handle,
        const std::string& sksl_wrapper_src) -> Result<uint64_t>
    {
        (void)preshader_handle;
        (void)sksl_wrapper_src;
        return std::unexpected(general_error(
            "compose_with_child_shader not supported by this backend"));
    }

    /// Compose multiple existing preshaders as children of a new SkSL wrapper.
    /// The SkSL wrapper declares `uniform shader child_N;` slots (N = 0, 1, ...)
    /// matching the order of preshader_handles.  Uniform data is passed as
    /// a raw byte vector matching the SkSL uniform layout.
    /// This is the multi-child generalization of compose_with_child_shader,
    /// needed for domain warp (base + warp field) and noise-blend (source A + B).
    /// @param preshader_handles  Vector of handles from preshader_cache_
    /// @param sksl_wrapper_src  SkSL source with N `uniform shader child_N;` slots
    /// @param uniform_data      Raw uniform data bytes (may be empty)
    /// @return                  New shader handle for the composed shader
    virtual auto compose_with_child_shaders(
        const std::vector<uint64_t>& preshader_handles,
        const std::string& sksl_wrapper_src,
        const std::vector<uint8_t>& uniform_data = {}) -> Result<uint64_t>
    {
        (void)preshader_handles;
        (void)sksl_wrapper_src;
        (void)uniform_data;
        return std::unexpected(general_error(
            "compose_with_child_shaders not supported by this backend"));
    }

    /// Bind texture GraphicObjects to a compiled shader's `uniform shader` slots.
    /// Renders each GraphicObject to an image and binds it as a child shader
    /// matched by SkSL `uniform shader <slot_name>` declaration.
    /// @param shader_handle  Handle from compile_shader
    /// @param textures       List of (slot_name, GraphicObject) pairs
    /// @return               New shader handle with children bound, or an error
    virtual auto bind_textures_to_shader(
        uint64_t shader_handle,
        const std::vector<std::pair<std::string, Value>>& textures) -> Result<uint64_t>
    {
        (void)shader_handle;
        (void)textures;
        return std::unexpected(general_error(
            "bind_textures_to_shader not supported by this backend"));
    }

    /// Get uniform descriptors for a compiled shader handle (introspection).
    /// @param shader_handle  Handle from compile_shader
    /// @return               List of uniform descriptors, or an error
    virtual auto get_shader_uniforms(uint64_t shader_handle)
        -> Result<std::vector<ShaderUniformInfo>>
    {
        (void)shader_handle;
        return std::unexpected(general_error(
            "shader introspection not supported by this backend"));
    }

    /// Get child shader slot descriptors for a compiled shader handle (introspection).
    /// Returns information about each `uniform shader` / `uniform colorFilter` /
    /// `uniform blender` declaration in the shader's SkSL source.
    /// @param shader_handle  Handle from compile_shader
    /// @return               List of child slot descriptors, or an error
    virtual auto get_shader_children(uint64_t shader_handle)
        -> Result<std::vector<ShaderChildSlotInfo>>
    {
        (void)shader_handle;
        return std::unexpected(general_error(
            "shader children introspection not supported by this backend"));
    }

    /// Create a shader from a rendered surface (for canvas->shader etc.).
    /// Snapshots the surface contents into a GPU-accessible shader stored in
    /// the preshader cache.
    /// @param surface  The rendered surface to capture
    /// @return         Shader handle (preshader_cache), or an error
    virtual auto create_surface_shader(Surface& surface)
        -> Result<uint64_t>
    {
        (void)surface;
        return std::unexpected(general_error(
            "create_surface_shader not supported by this backend"));
    }

    /// Evaluate a shader at a specific pixel coordinate and return its color.
    /// Creates a temporary 1×1 render target and samples the shader at (x, y).
    /// @param shader_handle  Handle from preshader_cache_
    /// @param x, y           Pixel coordinate to sample
    /// @return               (r g b a) as four floats, or an error
    virtual auto eval_shader(uint64_t shader_handle, float x, float y)
        -> Result<std::array<float, 4>>
    {
        (void)shader_handle;
        (void)x;
        (void)y;
        return std::unexpected(general_error(
            "eval_shader not supported by this backend"));
    }
};

// ==========================================================================================================================================================================================================================================═
// Force-link helper — NullBackend registration
// ==========================================================================================================================================================================================================================================═

/// Ensure the NullBackend (in null_backend.cpp) is linked into the binary.
///
/// When the NullBackend lives in a static library, MSVC's link.exe may skip
/// null_backend.obj because no public symbol from that translation unit is
/// referenced from other TUs.  Calling this function once forces the linker
/// to include the object file, which in turn runs the static registration
/// that adds "null" to BackendRegistry.
///
/// After calling this, use BackendRegistry::set_active("null") to activate
/// the null backend in test / headless environments.
void force_link_null_backend() noexcept;

}  // namespace pml
