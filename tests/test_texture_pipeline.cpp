#include <gtest/gtest.h>

#include "pml/core/texture.h"
#include "pml/graphics/graphics_types.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/texture_pipeline.h"
#include "pml/graphics/triangulation.h"
#include "pml/graphics/uv_solver.h"

#include <memory>
#include <vector>

namespace pml {

// ============================================================================
// pipeline_extract_params
// ============================================================================

TEST(TexturePipelineTest, ExtractParamsNoUvReturnsNullTexture) {
    GraphicObject obj(ShapeType::Rect);
    auto p = pipeline_extract_params(obj);
    EXPECT_FALSE(p.texture);
    EXPECT_EQ(p.uv_mode, 1);
}

TEST(TexturePipelineTest, ExtractParamsWithTexture) {
    auto tex_go = std::make_shared<GraphicObject>(ShapeType::Circle);
    auto tex = std::make_shared<TextureBox>(tex_go, 64, 64);
    GraphicObject obj(ShapeType::Rect, Params{
        {ParamKey::uv, Value(tex)}
    });
    auto p = pipeline_extract_params(obj);
    ASSERT_TRUE(p.texture);
    EXPECT_EQ(p.texture->stable_id, tex->stable_id);
    EXPECT_EQ(p.uv_mode, 1);  // default
    EXPECT_EQ(p.wrap_x, WrapMode::Clamp);
    EXPECT_EQ(p.filter, FilterMode::Linear);
}

TEST(TexturePipelineTest, ExtractParamsWithUvMode) {
    auto tex = std::make_shared<TextureBox>(
        std::make_shared<GraphicObject>(ShapeType::Circle));
    GraphicObject obj(ShapeType::Rect, Params{
        {ParamKey::uv, Value(tex)},
        {ParamKey::uv_mode, Value(int64_t{0})}  // planar
    });
    auto p = pipeline_extract_params(obj);
    EXPECT_EQ(p.uv_mode, 0);
}

TEST(TexturePipelineTest, ExtractParamsWithWrapFilter) {
    auto tex = std::make_shared<TextureBox>(
        std::make_shared<GraphicObject>(ShapeType::Circle));
    GraphicObject obj(ShapeType::Rect, Params{
        {ParamKey::uv, Value(tex)},
        {ParamKey::wrap_x, Value(int64_t{static_cast<int>(WrapMode::Repeat)})},
        {ParamKey::wrap_y, Value(int64_t{static_cast<int>(WrapMode::Mirror)})},
        {ParamKey::filter, Value(int64_t{static_cast<int>(FilterMode::Nearest)})}
    });
    auto p = pipeline_extract_params(obj);
    EXPECT_EQ(p.wrap_x, WrapMode::Repeat);
    EXPECT_EQ(p.wrap_y, WrapMode::Mirror);
    EXPECT_EQ(p.filter, FilterMode::Nearest);
}

TEST(TexturePipelineTest, ExtractParamsUsesTextureDefaultsWhenNoOverrides) {
    auto tex = std::make_shared<TextureBox>(
        std::make_shared<GraphicObject>(ShapeType::Circle));
    tex->wrap_x = WrapMode::Repeat;
    tex->wrap_y = WrapMode::Mirror;
    tex->filter = FilterMode::Nearest;
    GraphicObject obj(ShapeType::Rect, Params{
        {ParamKey::uv, Value(tex)}
    });
    auto p = pipeline_extract_params(obj);
    EXPECT_EQ(p.wrap_x, WrapMode::Repeat);
    EXPECT_EQ(p.wrap_y, WrapMode::Mirror);
    EXPECT_EQ(p.filter, FilterMode::Nearest);
}

// ============================================================================
// pipeline_solve_uv
// ============================================================================

static TriangulatedMesh make_tri_mesh() {
    TriangulatedMesh mesh;
    mesh.vertices = {{0, 0}, {100, 0}, {0, 100}};
    mesh.indices = {0, 1, 2};
    mesh.contour_map = {0, 1, 2};
    return mesh;
}

static TriangulatedMesh make_quad_mesh() {
    TriangulatedMesh mesh;
    mesh.vertices = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    mesh.indices = {0, 1, 2, 0, 2, 3};
    mesh.contour_map = {0, 1, 2, 3};
    return mesh;
}

TEST(TexturePipelineTest, SolveUvPlanar) {
    auto mesh = make_quad_mesh();
    GraphicObject obj(ShapeType::Rect);
    auto result = pipeline_solve_uv(mesh, 0, obj);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.uvs.size(), 4);
    EXPECT_NEAR(result.uvs[0].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[0].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[1].x, 1.0, 1e-9);
    EXPECT_NEAR(result.uvs[1].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[2].x, 1.0, 1e-9);
    EXPECT_NEAR(result.uvs[2].y, 1.0, 1e-9);
    EXPECT_NEAR(result.uvs[3].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[3].y, 1.0, 1e-9);
}

TEST(TexturePipelineTest, SolveUvHarmonicDefault) {
    auto mesh = make_quad_mesh();
    GraphicObject obj(ShapeType::Rect);
    auto result = pipeline_solve_uv(mesh, 1, obj);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.uvs.size(), 4);
    for (const auto& uv : result.uvs) {
        EXPECT_GE(uv.x, 0.0);
        EXPECT_LE(uv.x, 1.0);
        EXPECT_GE(uv.y, 0.0);
        EXPECT_LE(uv.y, 1.0);
    }
}

TEST(TexturePipelineTest, SolveUvExplicit) {
    auto mesh = make_quad_mesh();
    GraphicObject obj(ShapeType::Rect, Params{
        {ParamKey::uv_vertices, make_list_value({
            Value(0.0), Value(0.0),
            Value(0.5), Value(0.0),
            Value(0.5), Value(0.5),
            Value(0.0), Value(0.5)
        })}
    });
    auto result = pipeline_solve_uv(mesh, 2, obj);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.uvs.size(), 4);
    EXPECT_NEAR(result.uvs[0].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[0].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[1].x, 0.5, 1e-9);
    EXPECT_NEAR(result.uvs[1].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[2].x, 0.5, 1e-9);
    EXPECT_NEAR(result.uvs[2].y, 0.5, 1e-9);
    EXPECT_NEAR(result.uvs[3].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[3].y, 0.5, 1e-9);
}

TEST(TexturePipelineTest, SolveUvExplicitEmptyUvsFallsBack) {
    auto mesh = make_tri_mesh();
    GraphicObject obj(ShapeType::Rect);
    auto result = pipeline_solve_uv(mesh, 2, obj);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.uvs.size(), 3);
    EXPECT_NEAR(result.uvs[0].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[0].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[1].x, 1.0, 1e-9);
    EXPECT_NEAR(result.uvs[1].y, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[2].x, 0.0, 1e-9);
    EXPECT_NEAR(result.uvs[2].y, 1.0, 1e-9);
}

}  // namespace pml
