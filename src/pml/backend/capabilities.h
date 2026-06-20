#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Backend Capabilities
// ═══════════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <string>

namespace pml {

// ═══════════════════════════════════════════════════════════════════════════════
// BackendCap — bitmask flags describing backend capabilities
// ═══════════════════════════════════════════════════════════════════════════════

/// Capability flags for render backends.
enum class BackendCap : uint8_t {
    RasterCPU      = 1 << 0,  ///< CPU-based raster rendering
    GPUAccel       = 1 << 1,  ///< GPU-accelerated rendering
    Shaders        = 1 << 2,  ///< GLSL shader compilation support
    VectorOutput   = 1 << 3,  ///< Vector graphics output (e.g., SVG)
    AnimationGIF   = 1 << 4,  ///< Animated GIF export
    FontRendering  = 1 << 5,  ///< Text / font rendering support
    LoadPNG        = 1 << 6,  ///< PNG image loading
    LoadImage      = 1 << 7,  ///< General raster image loading (PNG/JPEG/WebP/etc.)
};

// ── Bitmask operators ─────────────────────────────────────────────────────────

/// Bitwise OR for combining capability flags.
[[nodiscard]] inline constexpr BackendCap operator|(BackendCap a, BackendCap b) noexcept
{
    return static_cast<BackendCap>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/// Bitwise AND for testing capability flags.
[[nodiscard]] inline constexpr BackendCap operator&(BackendCap a, BackendCap b) noexcept
{
    return static_cast<BackendCap>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/// Bitwise OR assignment.
inline constexpr BackendCap& operator|=(BackendCap& a, BackendCap b) noexcept
{
    a = a | b;
    return a;
}

/// Bitwise AND assignment.
inline constexpr BackendCap& operator&=(BackendCap& a, BackendCap b) noexcept
{
    a = a & b;
    return a;
}

/// Bitwise NOT (complement).
[[nodiscard]] inline constexpr BackendCap operator~(BackendCap a) noexcept
{
    return static_cast<BackendCap>(~static_cast<uint8_t>(a));
}

/// Convenience: check whether a capability flag is set in a bitmask.
[[nodiscard]] inline constexpr bool has_capability(BackendCap caps, BackendCap cap) noexcept
{
    return (static_cast<uint8_t>(caps) & static_cast<uint8_t>(cap)) != 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// BackendInfo — metadata describing a registered backend
// ═══════════════════════════════════════════════════════════════════════════════

/// Metadata describing a render backend.
struct BackendInfo {
    std::string name;           ///< Canonical backend name (e.g., "skia", "null")
    std::string description;    ///< Human-readable description
    BackendCap capabilities{static_cast<BackendCap>(0)};  ///< Supported capabilities
};

}  // namespace pml
