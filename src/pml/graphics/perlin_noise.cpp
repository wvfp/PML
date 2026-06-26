// ==========================================================================================================================================================================================================================================═
// PML Deterministic 2D Perlin Noise — Implementation
// ==========================================================================================================================================================================================================================================═

#include "perlin_noise.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace pml {

namespace {

// ---- Fixed source permutation (original Perlin 2002 table) --------------------------------─
// Used when seed == 0 for a well-known reference output.

constexpr int kPermTable[256] = {
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
     57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
     74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
     60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
     65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
    200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
     52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
     81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180
};

// ---- Simple deterministic LCG for permutation shuffle --------------------------------------------
// Using constants from Numerical Recipes (the classic "quick and dirty" LCG).
// Produces platform-independent, reproducible integer sequence from a seed.

class DeterministicRng {
public:
    explicit DeterministicRng(int seed) noexcept
        : state_(static_cast<uint32_t>(seed == 0 ? 1 : seed)) {}  // avoid 0

    uint32_t next() noexcept {
        state_ = 1664525u * state_ + 1013904223u;
        return state_;
    }

    int range(int max_exclusive) noexcept {
        // Rejection-based uniform sample in [0, max_exclusive).
        // Mask to avoid biasing toward low values.
        uint32_t mask = 1;
        while (mask < static_cast<uint32_t>(max_exclusive))
            mask = (mask << 1) | 1u;
        uint32_t v;
        do {
            v = next() & mask;
        } while (v >= static_cast<uint32_t>(max_exclusive));
        return static_cast<int>(v);
    }

private:
    uint32_t state_;
};

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Construction — seed-based permutation table
// ==========================================================================================================================================================================================================================================═

PerlinNoise2D::PerlinNoise2D(int seed) {
    // Build the base 256-element permutation.
    std::vector<int> perm(256);
    if (seed == 0) {
        // Use the fixed reference table for seed 0.
        std::copy_n(kPermTable, 256, perm.begin());
    } else {
        // Deterministic Fisher–Yates shuffle of [0..255].
        std::iota(perm.begin(), perm.end(), 0);
        DeterministicRng rng(seed);
        for (int i = 255; i > 0; --i) {
            int j = rng.range(i + 1);
            std::swap(perm[i], perm[j]);
        }
    }

    // Double up to 512 for fast wrapping (avoids modulo in hot path).
    perm_.resize(512);
    for (int i = 0; i < 512; ++i) {
        perm_[i] = perm[i & 255];
    }
}

// ==========================================================================================================================================================================================================================================═
// Core Perlin noise
// ==========================================================================================================================================================================================================================================═

double PerlinNoise2D::noise(double x, double y) const noexcept {
    // Find unit square that contains the point.
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    // Relative coordinates within the square.
    double dx = x - std::floor(x);
    double dy = y - std::floor(y);

    // Fade curves.
    double u = fade(dx);
    double v = fade(dy);

    // Hash four corners.
    int aa = perm_[perm_[X] + Y];
    int ab = perm_[perm_[X] + Y + 1];
    int ba = perm_[perm_[X + 1] + Y];
    int bb = perm_[perm_[X + 1] + Y + 1];

    // Blend contributions from each corner.
    double gx0y0 = grad(aa, dx,     dy);
    double gx1y0 = grad(ba, dx - 1, dy);
    double gx0y1 = grad(ab, dx,     dy - 1);
    double gx1y1 = grad(bb, dx - 1, dy - 1);

    double xblend0 = lerp(gx0y0, gx1y0, u);
    double xblend1 = lerp(gx0y1, gx1y1, u);

    return lerp(xblend0, xblend1, v);
}

// ==========================================================================================================================================================================================================================================═
// fBM / Turbulence
// ==========================================================================================================================================================================================================================================═

double PerlinNoise2D::fbm(double x, double y, int octaves,
                          double lacunarity, double persistence) const noexcept {
    double value = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double max_amplitude = 0.0;

    for (int i = 0; i < octaves; ++i) {
        value += amplitude * noise(x * frequency, y * frequency);
        max_amplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // Normalize so output stays roughly in [-1, 1].
    return value / max_amplitude;
}

double PerlinNoise2D::turbulence(double x, double y, int octaves,
                                 double lacunarity, double persistence) const noexcept {
    double value = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double max_amplitude = 0.0;

    for (int i = 0; i < octaves; ++i) {
        value += amplitude * std::abs(noise(x * frequency, y * frequency));
        max_amplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / max_amplitude;
}

// ==========================================================================================================================================================================================================================================═
// Internal helpers
// ==========================================================================================================================================================================================================================================═

double PerlinNoise2D::grad(int hash, double dx, double dy) const noexcept {
    // Use the lower 3 bits of the hash to select one of 8 gradient directions.
    int h = hash & 7;
    double u = (h < 4) ? dx : dy;
    double v = (h < 4) ? dy : dx;
    // Alternate sign based on bits 1 and 2.
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double PerlinNoise2D::fade(double t) noexcept {
    // 6t⁵ − 15t⁴ + 10t³
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double PerlinNoise2D::lerp(double a, double b, double t) noexcept {
    return a + t * (b - a);
}

} // namespace pml
