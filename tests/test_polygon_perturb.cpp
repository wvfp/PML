#include <gtest/gtest.h>
#include "test_helpers.h"
#include "pml/graphics/perlin_noise.h"
#include "pml/graphics/polygon_perturb.h"
#include "pml/graphics/rough.h"

#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================================
// PerlinNoise2D
// ============================================================================

TEST(PerlinNoiseTest, NoiseAtOriginIsZero) {
    pml::PerlinNoise2D noise(0);
    // Perlin noise at integer lattice points is 0.
    EXPECT_DOUBLE_EQ(noise.noise(0.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(noise.noise(1.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(noise.noise(0.0, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(noise.noise(1.0, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(noise.noise(2.0, 3.0), 0.0);
}

TEST(PerlinNoiseTest, NonIntegerInputReturnsNonZero) {
    pml::PerlinNoise2D noise(42);
    double v = noise.noise(0.5, 0.5);
    // At non-integer coordinates, noise should be non-zero (typically).
    // It could theoretically be very close to zero, but with the Perlin
    // algorithm this is astronomically unlikely at (0.5, 0.5).
    EXPECT_NE(v, 0.0);
}

TEST(PerlinNoiseTest, Deterministic) {
    pml::PerlinNoise2D noise(42);
    double a = noise.noise(3.14159, 2.71828);
    double b = noise.noise(3.14159, 2.71828);
    EXPECT_DOUBLE_EQ(a, b);
}

TEST(PerlinNoiseTest, DifferentSeedsDifferentOutput) {
    pml::PerlinNoise2D noise1(1);
    pml::PerlinNoise2D noise2(2);
    double v1 = noise1.noise(0.5, 0.5);
    double v2 = noise2.noise(0.5, 0.5);
    EXPECT_NE(v1, v2);
}

TEST(PerlinNoiseTest, ValueInRange) {
    pml::PerlinNoise2D noise(0);
    // Sample many points, all should be in [-1, 1].
    for (int x = -5; x <= 5; ++x) {
        for (int y = -5; y <= 5; ++y) {
            double v = noise.noise(x + 0.3, y + 0.7);
            EXPECT_GE(v, -1.0);
            EXPECT_LE(v, 1.0);
        }
    }
}

TEST(PerlinNoiseTest, FbmInRange) {
    pml::PerlinNoise2D noise(42);
    double v = noise.fbm(0.5, 0.5, 4, 0.5, 2.0);
    EXPECT_GE(v, -1.5);  // fbm can slightly overshoot [-1, 1] at high octaves
    EXPECT_LE(v, 1.5);
}

TEST(PerlinNoiseTest, FbmDeterministic) {
    pml::PerlinNoise2D noise(42);
    double a = noise.fbm(1.5, 2.5, 4, 0.5, 2.0);
    double b = noise.fbm(1.5, 2.5, 4, 0.5, 2.0);
    EXPECT_DOUBLE_EQ(a, b);
}

TEST(PerlinNoiseTest, TurbulenceNonNegative) {
    pml::PerlinNoise2D noise(42);
    double v = noise.turbulence(0.5, 0.5, 4);
    EXPECT_GE(v, 0.0);
}

TEST(PerlinNoiseTest, TurbulenceDeterministic) {
    pml::PerlinNoise2D noise(42);
    double a = noise.turbulence(1.5, 2.5, 4, 0.5, 2.0);
    double b = noise.turbulence(1.5, 2.5, 4, 0.5, 2.0);
    EXPECT_DOUBLE_EQ(a, b);
}

// ============================================================================
// expand_scalar
// ============================================================================

TEST(ExpandScalarTest, VectorAlreadyCorrectSize) {
    std::vector<double> v = {1.0, 2.0, 3.0};
    auto r = pml::expand_scalar(v, 3);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_DOUBLE_EQ(r[0], 1.0);
    EXPECT_DOUBLE_EQ(r[1], 2.0);
    EXPECT_DOUBLE_EQ(r[2], 3.0);
}

TEST(ExpandScalarTest, ScalarExpands) {
    std::vector<double> v = {0.15};
    auto r = pml::expand_scalar(v, 4);
    ASSERT_EQ(r.size(), 4u);
    for (auto x : r) EXPECT_DOUBLE_EQ(x, 0.15);
}

TEST(ExpandScalarTest, WrongSizeThrows) {
    std::vector<double> v = {1.0, 2.0};
    EXPECT_THROW(pml::expand_scalar(v, 4), std::invalid_argument);
}

TEST(ExpandScalarTest, BoolVectorCorrectSize) {
    std::vector<bool> v = {true, false, true};
    auto r = pml::expand_scalar(v, 3);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_TRUE(r[0]);
    EXPECT_FALSE(r[1]);
    EXPECT_TRUE(r[2]);
}

TEST(ExpandScalarTest, BoolScalarExpands) {
    std::vector<bool> v = {true};
    auto r = pml::expand_scalar(v, 5);
    ASSERT_EQ(r.size(), 5u);
    for (auto x : r) EXPECT_TRUE(x);
}

// ============================================================================
// Catmull-Rom subdivision
// ============================================================================

TEST(CatmullRomTest, ZeroCountReturnsEmpty) {
    pml::RoughPoint p0{0,0}, p1{1,0}, p2{2,0}, p3{3,0};
    auto r = pml::catmull_rom_subdivide(p0, p1, p2, p3, 0);
    EXPECT_TRUE(r.empty());
}

TEST(CatmullRomTest, SubdividesStraightLine) {
    // Four colinear points: (0,0) -> (1,0) -> (2,0) -> (3,0)
    // Interpolating between (1,0) and (2,0) should produce points on the line.
    pml::RoughPoint p0{0,0}, p1{1,0}, p2{2,0}, p3{3,0};
    auto r = pml::catmull_rom_subdivide(p0, p1, p2, p3, 3);
    ASSERT_EQ(r.size(), 3u);
    for (const auto& pt : r) {
        EXPECT_GE(pt.x, 1.0);
        EXPECT_LE(pt.x, 2.0);
        EXPECT_DOUBLE_EQ(pt.y, 0.0);
    }
    // Points should be evenly spaced.
    EXPECT_NEAR(r[0].x, 1.25, 0.01);
    EXPECT_NEAR(r[1].x, 1.5,  0.01);
    EXPECT_NEAR(r[2].x, 1.75, 0.01);
}

TEST(CatmullRomTest, SubdividesCurve) {
    // Non-colinear points.
    pml::RoughPoint p0{0,0}, p1{1,0}, p2{2,1}, p3{3,0};
    auto r = pml::catmull_rom_subdivide(p0, p1, p2, p3, 5);
    ASSERT_EQ(r.size(), 5u);
    // All points should have y >= 0 (curve goes above the baseline).
    for (const auto& pt : r) {
        EXPECT_GE(pt.y, 0.0);
    }
    // The middle point should be near the peak (y ≈ 0.5 * something).
    EXPECT_GT(r[2].y, 0.3);
}

// ============================================================================
// Corner rounding
// ============================================================================

TEST(RoundCornerTest, ZeroRadiusReturnsEmpty) {
    auto r = pml::round_corner_bezier({0,0}, {1,0}, {2,1}, 0.0);
    EXPECT_TRUE(r.empty());
}

TEST(RoundCornerTest, ReturnsArcPoints) {
    // Right-angle corner at B=(0,0): A=(-1,0), B=(0,0), C=(0,1)
    auto r = pml::round_corner_bezier({-1,0}, {0,0}, {0,1}, 0.3);
    ASSERT_FALSE(r.empty());
    // First point should be near the tangent point on AB (at x ≈ -0.3, y ≈ 0).
    EXPECT_NEAR(r.front().x, -0.3, 0.05);
    EXPECT_NEAR(r.front().y,  0.0, 0.05);
    // Last point should be near the tangent point on BC (at x ≈ 0, y ≈ 0.3).
    EXPECT_NEAR(r.back().x, 0.0,  0.05);
    EXPECT_NEAR(r.back().y, 0.3,  0.05);
    // All points should be in the quadrant [-0.3,0] x [0,0.3].
    for (const auto& pt : r) {
        EXPECT_GE(pt.x, -0.35);
        EXPECT_LE(pt.x, 0.05);
        EXPECT_GE(pt.y, -0.05);
        EXPECT_LE(pt.y, 0.35);
    }
}

TEST(RoundCornerTest, LargeRadiusClamped) {
    // Radius larger than both edges → clamped.
    auto r = pml::round_corner_bezier({-0.5, 0}, {0, 0}, {0, 0.5}, 999.0);
    ASSERT_FALSE(r.empty());
    // Should still produce valid points, just clamped.
    EXPECT_GT(r.front().x, -0.5);
    EXPECT_LT(r.front().x, 0.0);
}

// ============================================================================
// perturb_polygon — integration
// ============================================================================

TEST(PerturbPolygonTest, ZeroNoiseReturnsSameVertices) {
    // Square with zero noise → output edges contain the original vertices.
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.0};
    cfg.edge_mask  = {false};
    cfg.edge_subdivisions = {0};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 0;

    pml::PerlinNoise2D noise(0);
    auto result = pml::perturb_polygon(verts, cfg, noise);

    // With zero noise and no subdivision, each edge has 2 points (endpoints).
    ASSERT_EQ(result.size(), 4u);
    for (size_t i = 0; i < 4; ++i) {
        ASSERT_EQ(result[i].size(), 2u);
        // First point of edge i should equal vertex i.
        EXPECT_DOUBLE_EQ(result[i][0].x, verts[i].x);
        EXPECT_DOUBLE_EQ(result[i][0].y, verts[i].y);
    }
}

TEST(PerturbPolygonTest, SubdivisionWithoutNoise) {
    // Square with subdivisions but zero noise displacement.
    // Catmull-Rom uses adjacent vertices as tangents, so even without noise
    // the curve overshoots the endpoints (a known Catmull-Rom property).
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.0};
    cfg.edge_mask  = {true};
    cfg.edge_subdivisions = {3};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 0;

    pml::PerlinNoise2D noise(0);
    auto result = pml::perturb_polygon(verts, cfg, noise);

    // Each edge: first endpoint + 3 subdiv points + last endpoint = 5 points.
    ASSERT_EQ(result.size(), 4u);
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(result[i].size(), 5u) << "edge " << i;
        // First point should match the start vertex.
        EXPECT_DOUBLE_EQ(result[i][0].x, verts[i].x);
        EXPECT_DOUBLE_EQ(result[i][0].y, verts[i].y);
        // Last point should match the end vertex.
        size_t j = (i + 1) % 4;
        EXPECT_DOUBLE_EQ(result[i][4].x, verts[j].x);
        EXPECT_DOUBLE_EQ(result[i][4].y, verts[j].y);
    }
    // Bottom edge (0→1): adjacent vertices are above it (0,10) and (10,10),
    // which pull the Catmull-Rom downward → interior points have y < 0.
    for (size_t k = 1; k < 4; ++k) {
        EXPECT_LT(result[0][k].y, 0.0) << "edge 0, point " << k;
    }
    // Top edge (2→3): adjacent vertices are below it (10,0) and (0,0),
    // which pull the Catmull-Rom upward → interior points have y > 10.
    for (size_t k = 1; k < 4; ++k) {
        EXPECT_GT(result[2][k].y, 10.0) << "edge 2, point " << k;
    }
}

TEST(PerturbPolygonTest, NoiseChangesVertices) {
    // Square with noise → vertices should move.
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.2};
    cfg.edge_mask  = {true};
    cfg.edge_subdivisions = {2};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 42;

    pml::PerlinNoise2D noise(42);
    auto result = pml::perturb_polygon(verts, cfg, noise);

    ASSERT_EQ(result.size(), 4u);
    // With noise > 0, interior points should move away from the original line.
    // Since noise is deterministic, we can check that at least some points differ.
    bool any_different = false;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 1; j < result[i].size() - 1; ++j) {
            // Interior points should differ from the straight-line Catmull-Rom position.
            if (result[i][j].y != 0.0 || result[i][j].x != 0.0) {
                any_different = true;
                break;
            }
        }
    }
    EXPECT_TRUE(any_different);
}

TEST(PerturbPolygonTest, DeterministicOutput) {
    // Same inputs → same output.
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.2};
    cfg.edge_mask  = {true};
    cfg.edge_subdivisions = {2};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 42;

    pml::PerlinNoise2D noise(42);
    auto r1 = pml::perturb_polygon(verts, cfg, noise);
    auto r2 = pml::perturb_polygon(verts, cfg, noise);

    ASSERT_EQ(r1.size(), r2.size());
    for (size_t i = 0; i < r1.size(); ++i) {
        ASSERT_EQ(r1[i].size(), r2[i].size());
        for (size_t j = 0; j < r1[i].size(); ++j) {
            EXPECT_DOUBLE_EQ(r1[i][j].x, r2[i][j].x);
            EXPECT_DOUBLE_EQ(r1[i][j].y, r2[i][j].y);
        }
    }
}

TEST(PerturbPolygonTest, EdgeMaskSelective) {
    // Mask: perturb edges 1 and 3, leave 0 and 2 straight.
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.3};
    cfg.edge_mask  = {false, true, false, true};
    cfg.edge_subdivisions = {0, 3, 0, 3};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 42;

    pml::PerlinNoise2D noise(42);
    auto result = pml::perturb_polygon(verts, cfg, noise);

    ASSERT_EQ(result.size(), 4u);
    // Edge 0 (mask=false): just 2 endpoints.
    EXPECT_EQ(result[0].size(), 2u);
    // Edge 2 (mask=false): just 2 endpoints.
    EXPECT_EQ(result[2].size(), 2u);
    // Edge 1 (mask=true, subdiv=3): 2 + 3 = 5 points.
    EXPECT_EQ(result[1].size(), 5u);
    // Edge 3 (mask=true, subdiv=3): 5 points.
    EXPECT_EQ(result[3].size(), 5u);
}

TEST(PerturbPolygonTest, CornerRoundingAddsPoints) {
    // Square with corner rounding → more points near each corner.
    std::vector<pml::RoughPoint> verts = {{0,0}, {10,0}, {10,10}, {0,10}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.0};
    cfg.edge_mask  = {false};
    cfg.edge_subdivisions = {0};
    cfg.corner_radius = {2.0};
    cfg.corner_mask  = {true};
    cfg.seed = 0;

    pml::PerlinNoise2D noise(0);
    auto result = pml::perturb_polygon(verts, cfg, noise);

    // Without corner rounding, each edge has 2 pts → total 8.
    // With rounding, arcs replace corner points → more points.
    size_t total = 0;
    for (const auto& edge : result) total += edge.size();
    EXPECT_GT(total, 8u);
}

TEST(PerturbPolygonTest, Triangle) {
    // Minimum polygon (3 vertices).
    std::vector<pml::RoughPoint> verts = {{0,0}, {5,0}, {2.5,5}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.0};
    cfg.edge_mask  = {false};
    cfg.edge_subdivisions = {0};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 0;

    pml::PerlinNoise2D noise(0);
    auto result = pml::perturb_polygon(verts, cfg, noise);
    ASSERT_EQ(result.size(), 3u);
}

TEST(PerturbPolygonTest, Pentagon) {
    // 5-vertex polygon.
    std::vector<pml::RoughPoint> verts = {{0,0}, {5,0}, {6,4}, {3,6}, {-1,4}};
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.1};
    cfg.edge_mask  = {true};
    cfg.edge_subdivisions = {2};
    cfg.corner_radius = {1.0};
    cfg.corner_mask  = {true};
    cfg.seed = 7;

    pml::PerlinNoise2D noise(7);
    auto result = pml::perturb_polygon(verts, cfg, noise);
    ASSERT_EQ(result.size(), 5u);
    // Total points should exceed  (5 edges × 2 endpoints = 10).
    size_t total = 0;
    for (const auto& edge : result) total += edge.size();
    EXPECT_GT(total, 10u);
}

TEST(PerturbPolygonTest, DegenerateInputThrows) {
    std::vector<pml::RoughPoint> verts = {{0,0}, {1,0}};  // 2 vertices
    pml::PerturbConfig cfg;
    cfg.edge_noise = {0.0};
    cfg.edge_mask  = {false};
    cfg.edge_subdivisions = {0};
    cfg.corner_radius = {0.0};
    cfg.corner_mask  = {false};
    cfg.seed = 0;

    pml::PerlinNoise2D noise(0);
    EXPECT_THROW(pml::perturb_polygon(verts, cfg, noise), std::invalid_argument);
}

// ============================================================================
// flatten_perturb_result
// ============================================================================

TEST(FlattenTest, ConcatenatesEdges) {
    pml::PerturbResult edges = {
        {{0,0}, {1,0}},
        {{1,0}, {1,1}},
        {{1,1}, {0,1}},
        {{0,1}, {0,0}}
    };
    auto flat = pml::flatten_perturb_result(edges);
    ASSERT_EQ(flat.size(), 8u);
    EXPECT_DOUBLE_EQ(flat[0].x, 0.0);
    EXPECT_DOUBLE_EQ(flat[1].x, 1.0);
    EXPECT_DOUBLE_EQ(flat[3].y, 1.0);
}

// ============================================================================
// Config field: mixed scalar/vector
// ============================================================================

TEST(ExpandScalarTest, IntVectorCorrectSize) {
    std::vector<int> v = {3, 5, 7};
    auto r = pml::expand_scalar(v, 3);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 3);
    EXPECT_EQ(r[1], 5);
    EXPECT_EQ(r[2], 7);
}

TEST(ExpandScalarTest, IntScalarExpands) {
    std::vector<int> v = {3};
    auto r = pml::expand_scalar(v, 4);
    ASSERT_EQ(r.size(), 4u);
    for (auto x : r) EXPECT_EQ(x, 3);
}

TEST(ExpandScalarTest, IntWrongSizeThrows) {
    std::vector<int> v = {1, 2};
    EXPECT_THROW(pml::expand_scalar(v, 4), std::invalid_argument);
}

TEST(ExpandScalarTest, BoolWrongSizeThrows) {
    std::vector<bool> v = {true, false, true};
    EXPECT_THROW(pml::expand_scalar(v, 4), std::invalid_argument);
}
