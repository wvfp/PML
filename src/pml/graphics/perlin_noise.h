#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Deterministic 2D Perlin Noise
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// CPU-side implementation of classic Perlin noise (Ken Perlin, 2002 improved).
// Provides:
//   - raw noise(x, y) → [-1, 1]
//   - octave_noise(fbm) with lacunarity/persistence
//   - turbulence
//
// Fully deterministic: same seed + same (x, y) → same result on all platforms.
// ==========================================================================================================================================================================================================================================═

#include <cstdint>
#include <vector>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// PerlinNoise2D
// ==========================================================================================================================================================================================================================================═

class PerlinNoise2D {
public:
    /// Construct with an integer seed.  Seed 0 uses a fixed default permutation.
    /// Different seeds produce entirely different noise fields.
    explicit PerlinNoise2D(int seed = 0);

    /// Raw Perlin noise at (x, y).  Returns value in [-1, 1].
    double noise(double x, double y) const noexcept;

    /// Fractional Brownian Motion (fBM) — sum of octaves.
    ///   x, y      : sample coordinates
    ///   octaves   : number of octaves (1 = plain noise)
    ///   lacunarity: frequency multiplier per octave (typical 2.0)
    ///   persistence: amplitude multiplier per octave (typical 0.5)
    /// Returns value roughly in [-1, 1] (may exceed slightly at high octaves).
    double fbm(double x, double y, int octaves = 4,
               double lacunarity = 2.0, double persistence = 0.5) const noexcept;

    /// Turbulence — sum of absolute values of octaves.
    /// Same parameters as fbm.  Returns non-negative value in [0, ~1].
    double turbulence(double x, double y, int octaves = 4,
                      double lacunarity = 2.0, double persistence = 0.5) const noexcept;

private:
    /// Gradient function: dot(gradient at perm[p], (dx, dy))
    double grad(int hash, double dx, double dy) const noexcept;

    /// Fade curve: 6t⁵ − 15t⁴ + 10t³
    static double fade(double t) noexcept;

    /// Linear interpolation
    static double lerp(double a, double b, double t) noexcept;

    /// Permutation table (size 512 — double-up of the 256-entry table)
    std::vector<int> perm_;
};

} // namespace pml
