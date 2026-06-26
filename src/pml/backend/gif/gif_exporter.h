#pragma once

// ==========================================================================================================================================================================================================================================═
// PML GIF Exporter
// ==========================================================================================================================================================================================================================================═
//
// Standalone animated GIF export using giflib.
// Takes RGBA frame data, composites onto white background, quantizes to 256
// colors per frame using giflib's built-in quantizer, and writes an animated
// GIF89a file with proper disposal mode and frame delays.
//
// No dependency on any PML backend or graphics code — pure utility.
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <string>
#include <vector>

namespace pml::backend::gif {

/// Export a sequence of RGBA frames as an animated GIF.
///
/// @param frames  Vector of frames; each frame is RGBA pixel data
///                (4 bytes/pixel, row-major, top-to-bottom).
/// @param width   Frame width in pixels.
/// @param height  Frame height in pixels.
/// @param path    Output file path.
/// @param fps     Frames per second (default: 10.0).
///
/// Each frame's RGBA data is composited onto a white background (matching
/// Python Pillow behavior), then quantized to a 256-color adaptive palette
/// per frame (matching Pillow ADAPTIVE).
///
/// Disposal mode is set to 2 (restore to background color).
///
/// @throws std::runtime_error on giflib errors (file I/O, quantization failure).
void export_gif(const std::vector<std::vector<uint8_t>>& frames,
                int width, int height,
                const std::string& path,
                double fps = 10.0);

} // namespace pml::backend::gif
