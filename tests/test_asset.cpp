// ==========================================================================================================================================================================================================================================═
// Asset / Bitmap I/O — Google Test Suite
// ==========================================================================================================================================================================================================================================═
//
// Tests for the asset loading pipeline:
//   - AssetLoader path resolution (absolute, relative, search roots)
//   - AssetCache decode caching and clear behaviour
//   - PML builtins: asset-path?, load-spritesheet
// ==========================================================================================================================================================================================================================================═

#include <gtest/gtest.h>

#include <format>

#include "pml/api/api.h"
#include "pml/asset/asset_cache.h"
#include "pml/asset/asset_loader.h"
#include "pml/backend/backend.h"
#include "pml/layer/blend_mode.h"

#include <filesystem>
#include <fstream>
#include <regex>

using namespace pml;
namespace fs = std::filesystem;

namespace {

struct MockSurface final : Surface {
    using Surface::Surface;
};

class MockBackend final : public RenderBackend {
public:
    BackendInfo info() const override {
        return BackendInfo{"mock", "mock", BackendCap::RasterCPU};
    }

    std::unique_ptr<Surface> create_surface(int w, int h, uint32_t) override {
        return std::make_unique<MockSurface>(w, h);
    }

    Result<void> draw(Surface&, const GraphicObject&) override { return {}; }

    Result<void> save_image(Surface&, const std::string&, const std::string&) override {
        return {};
    }

    Result<void> save_animation(const std::vector<Surface*>&, const std::string&,
                                const std::string&, int) override {
        return {};
    }

    Result<std::unique_ptr<Surface>> load_image(const std::string&) override {
        return std::make_unique<MockSurface>(16, 16);
    }

    Result<void> composite(Surface&, Surface&, int, int) override { return {}; }

    Result<void> composite_with_blend(Surface&, Surface&, int, int,
                                      BlendMode, float) override { return {}; }

    Result<void> apply_mask(Surface&, Surface&) override { return {}; }

    Result<std::unique_ptr<Surface>> draw_to_new_surface(
        const GraphicObject&, int w, int h, uint32_t bg) override {
        return create_surface(w, h, bg);
    }

    bool supports_blend_mode() const noexcept override { return true; }
    bool supports_layer_compositing() const noexcept override { return true; }

    Result<uint64_t> compile_shader(const std::string&) override { return 1; }

    Result<void> apply_color_matrix(Surface&, const std::array<float, 20>&) override { return {}; }
    Result<void> apply_blur(Surface&, float, float, BlurType) override { return {}; }
    Result<void> apply_convolution(Surface&, const ConvolutionKernel&) override { return {}; }
    Result<void> apply_color_table(Surface&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&,
        const std::array<uint8_t, 256>&) override { return {}; }
    Result<void> apply_offset(Surface&, float, float) override { return {}; }
    Result<void> apply_drop_shadow(Surface&, float, float, float, float, uint32_t) override { return {}; }
    Result<void> apply_inner_shadow(Surface&, float, float, float, uint32_t) override { return {}; }
    Result<void> apply_outer_glow(Surface&, float, uint32_t) override { return {}; }
    Result<void> apply_inner_glow(Surface&, float, uint32_t) override { return {}; }
    Result<void> apply_bevel_emboss(Surface&, float, float, float, uint32_t, uint32_t) override { return {}; }
};

class AssetLoaderTest : public ::testing::Test {};
class AssetCacheTest : public ::testing::Test {};
class AssetBuiltinTest : public ::testing::Test {
protected:
    pml::PMLRuntime rt;
};

} // namespace

TEST_F(AssetLoaderTest, ReportsMissingFile) {
    auto r = resolve_asset_path("definitely-missing-xyz-12345.pml");
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::ResourceError);
    EXPECT_FALSE(r.error().message.empty());
}

TEST_F(AssetBuiltinTest, AssetPathPredicateMissing) {
    auto r = rt.execute("(asset-path? \"definitely-missing-xyz-12345.pml\")");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.value, false);
}

TEST_F(AssetBuiltinTest, LoadSpritesheetParsesAsepriteJson) {
    // Write a minimal Aseprite-style JSON descriptor next to this test file.
    fs::path json_path = fs::temp_directory_path() / "pml_test_spritesheet.json";
    fs::path image_path = fs::temp_directory_path() / "sheet.png";
    {
        std::ofstream ofs(json_path);
        ofs << R"({
            "frames": [
                {"filename": "frame1.png", "frame": {"x": 0, "y": 0, "w": 10, "h": 10}},
                {"filename": "frame2.png", "frame": {"x": 10, "y": 0, "w": 20, "h": 20}}
            ]
        })";
    }
    {
        // Create a dummy image file so resolve_asset_path succeeds.
        std::ofstream ofs(image_path, std::ios::binary);
        ofs << "dummy";
    }

    auto r = rt.execute(std::format(
        "(length (load-spritesheet \"{}\" \"{}\"))",
        std::regex_replace(image_path.string(), std::regex(R"(\\)"), R"(\\)"),
        std::regex_replace(json_path.string(), std::regex(R"(\\)"), R"(\\)")));

    std::error_code ec;
    fs::remove(json_path, ec);
    fs::remove(image_path, ec);

    ASSERT_TRUE(r.success) << (r.error ? r.error->dump() : "unknown error");
    EXPECT_EQ(r.value, 2);
}

TEST_F(AssetBuiltinTest, LoadSpritesheetParsesTexturePackerJson) {
    fs::path json_path = fs::temp_directory_path() / "pml_test_tp.json";
    fs::path image_path = fs::temp_directory_path() / "sheet.png";
    {
        std::ofstream ofs(json_path);
        ofs << R"({
            "frames": {
                "frame1.png": {"frame": {"x": 0, "y": 0, "w": 8, "h": 8}},
                "frame2.png": {"frame": {"x": 8, "y": 0, "w": 16, "h": 16}},
                "frame3.png": {"frame": {"x": 0, "y": 0, "w": 0, "h": 0}}
            }
        })";
    }
    {
        // Create a dummy image file so resolve_asset_path succeeds.
        std::ofstream ofs(image_path, std::ios::binary);
        ofs << "dummy";
    }

    auto r = rt.execute(std::format(
        "(length (load-spritesheet \"{}\" \"{}\"))",
        std::regex_replace(image_path.string(), std::regex(R"(\\)"), R"(\\)"),
        std::regex_replace(json_path.string(), std::regex(R"(\\)"), R"(\\)")));

    std::error_code ec;
    fs::remove(json_path, ec);
    fs::remove(image_path, ec);

    ASSERT_TRUE(r.success) << (r.error ? r.error->dump() : "unknown error");
    EXPECT_EQ(r.value, 2);  // zero-area frame is skipped
}
