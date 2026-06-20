# PML 原生 3D 支持实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 PML C++ 运行时中加入原生 3D 原语，支持立方体/长方体/倒角立方体/锥形/平面/球，每面可绑定 PML 2D 图形贴图，支持 3D 变换与可切换透视/正交相机。

**Architecture:** 新增 `src/pml/graphics3d/` 模块负责 3D 数学、网格、相机；`GraphicObject` 扩展 `"mesh3d"` shape type；Skia 后端在 `draw_object` 中识别 mesh3d，执行投影、背面剔除、深度排序，并用 `SkVertices` 将 material 贴图绘制到投影多边形。

**Tech Stack:** C++23, Skia（预编译静态库）, CMake, GTest

---

## 文件结构总览

| 文件 | 类型 | 职责 |
| --- | --- | --- |
| `src/pml/graphics3d/vec3.h` | 新建 | `Vec3` 3D 向量/点 |
| `src/pml/graphics3d/mat4.h` | 新建 | `Mat4` 4×4 矩阵（translate/rotate/scale/lookAt/perspective/orthographic） |
| `src/pml/graphics3d/transform3d.h` | 新建 | `Transform3D` 模型变换组合 |
| `src/pml/graphics3d/camera3d.h` | 新建 | `Camera3D` + 全局相机单例 |
| `src/pml/graphics3d/mesh3d.h` | 新建 | `Mesh3D`：顶点、面、每面 material `GraphicObject` |
| `src/pml/graphics3d/primitive_factory.h` | 新建 | 生成 cube/cuboid/rounded-cuboid/cone/plane/sphere 网格 |
| `src/pml/graphics3d/builtins_3d.h` | 新建 | PML 内置函数：camera, cube3d, cuboid3d, rotate-x/y/z, translate3d, scale3d |
| `src/pml/graphics3d/CMakeLists.txt` | 新建 | 模块构建 |
| `src/pml/backend/skia/skia_backend.cpp` | 修改 | 新增 `draw_mesh3d` 分支 |
| `src/pml/api/api.cpp` | 修改 | `register_3d_builtins(m_env)` |
| `CMakeLists.txt` | 修改 | 添加 `graphics3d` 子目录 |
| `tests/builtins_smoke.cpp` | 修改 | 添加 3D 渲染 smoke 测试 |
| `AGENTS.md` | 修改 | 记录 3D 为 C++ 端新增能力（Python 无对应） |

---

## Task 1: 3D 数学基础 — Vec3 与 Mat4

**Files:**
- Create: `src/pml/graphics3d/vec3.h`
- Create: `src/pml/graphics3d/mat4.h`
- Create: `src/pml/graphics3d/mat4.cpp`
- Test: `tests/test_graphics3d.cpp`（新建，本 task 先写 Vec3/Mat4 用例）

- [ ] **Step 1: 实现 Vec3**

```cpp
// src/pml/graphics3d/vec3.h
#pragma once
#include <cmath>

namespace pml {

struct Vec3 {
    double x{0}, y{0}, z{0};

    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
    Vec3 operator*(double s)    const { return Vec3(x*s, y*s, z*s); }
    Vec3 operator/(double s)    const { return Vec3(x/s, y/s, z/s); }

    double dot(const Vec3& o)   const { return x*o.x + y*o.y + z*o.z; }
    Vec3 cross(const Vec3& o)   const {
        return Vec3(y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x);
    }
    double length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const {
        double len = length();
        if (len < 1e-12) return Vec3(0,0,0);
        return *this / len;
    }
};

} // namespace pml
```

- [ ] **Step 2: 实现 Mat4 核心操作**

```cpp
// src/pml/graphics3d/mat4.h
#pragma once
#include "vec3.h"
#include <array>
#include <cmath>

namespace pml {

struct Mat4 {
    std::array<double, 16> m{};

    Mat4() { identity(); }

    static Mat4 identity();
    static Mat4 translate(double x, double y, double z);
    static Mat4 scale(double sx, double sy, double sz);
    static Mat4 rotate_x(double angle_deg);
    static Mat4 rotate_y(double angle_deg);
    static Mat4 rotate_z(double angle_deg);
    static Mat4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Mat4 perspective(double fov_deg, double aspect, double near, double far);
    static Mat4 orthographic(double size, double aspect, double near, double far);

    Mat4 operator*(const Mat4& o) const;
    Vec3 transform_point(const Vec3& p) const;   // 假设 w=1，做透视除法
    Vec3 transform_vector(const Vec3& v) const;  // 假设 w=0
};

} // namespace pml
```

```cpp
// src/pml/graphics3d/mat4.cpp
#include "mat4.h"

namespace pml {

Mat4 Mat4::identity() {
    Mat4 r;
    r.m = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    return r;
}

// ... translate/scale/rotate/look_at/perspective/orthographic 实现 ...
// 注意：列主序（column-major），与 OpenGL 一致。

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            r.m[col*4 + row] =
                m[0*4 + row] * o.m[col*4 + 0] +
                m[1*4 + row] * o.m[col*4 + 1] +
                m[2*4 + row] * o.m[col*4 + 2] +
                m[3*4 + row] * o.m[col*4 + 3];
    return r;
}

Vec3 Mat4::transform_point(const Vec3& p) const {
    double x = m[0]*p.x + m[4]*p.y + m[8]*p.z + m[12];
    double y = m[1]*p.x + m[5]*p.y + m[9]*p.z + m[13];
    double z = m[2]*p.x + m[6]*p.y + m[10]*p.z + m[14];
    double w = m[3]*p.x + m[7]*p.y + m[11]*p.z + m[15];
    if (std::abs(w) < 1e-12) w = 1.0;
    return Vec3(x/w, y/w, z/w);
}

} // namespace pml
```

- [ ] **Step 3: 添加 GTest 测试**

```cpp
// tests/test_graphics3d.cpp
#include <gtest/gtest.h>
#include "pml/graphics3d/vec3.h"
#include "pml/graphics3d/mat4.h"

using namespace pml;

TEST(Graphics3D, Vec3Cross) {
    Vec3 a(1,0,0), b(0,1,0);
    Vec3 c = a.cross(b);
    EXPECT_NEAR(c.z, 1.0, 1e-9);
}

TEST(Graphics3D, Mat4PerspectiveProjection) {
    Mat4 proj = Mat4::perspective(45, 1.0, 1.0, 1000.0);
    Mat4 view = Mat4::look_at(Vec3(0,0,300), Vec3(0,0,0), Vec3(0,1,0));
    Mat4 vp = proj * view;
    Vec3 p = vp.transform_point(Vec3(0,0,0));
    EXPECT_NEAR(p.x, 0.0, 1e-9);
    EXPECT_NEAR(p.y, 0.0, 1e-9);
}
```

- [ ] **Step 4: 运行测试**

Run:
```powershell
cmake --build --preset debug --target pml-tests
.\build\debug\tests\Debug\pml-tests.exe --gtest_filter=Graphics3D.*
```
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add src/pml/graphics3d/vec3.h src/pml/graphics3d/mat4.h src/pml/graphics3d/mat4.cpp tests/test_graphics3d.cpp
git commit -m "feat(graphics3d): add Vec3 and Mat4"
```

---

## Task 2: Transform3D 与 Camera3D

**Files:**
- Create: `src/pml/graphics3d/transform3d.h`
- Create: `src/pml/graphics3d/camera3d.h`
- Modify: `tests/test_graphics3d.cpp`

- [ ] **Step 1: 实现 Transform3D**

```cpp
// src/pml/graphics3d/transform3d.h
#pragma once
#include "mat4.h"

namespace pml {

struct Transform3D {
    Mat4 matrix{Mat4::identity()};

    static Transform3D identity() { return Transform3D{}; }

    Transform3D translated(double x, double y, double z) const {
        Transform3D r;
        r.matrix = Mat4::translate(x,y,z) * matrix;
        return r;
    }
    Transform3D rotated_x(double deg) const {
        Transform3D r;
        r.matrix = Mat4::rotate_x(deg) * matrix;
        return r;
    }
    Transform3D rotated_y(double deg) const {
        Transform3D r;
        r.matrix = Mat4::rotate_y(deg) * matrix;
        return r;
    }
    Transform3D rotated_z(double deg) const {
        Transform3D r;
        r.matrix = Mat4::rotate_z(deg) * matrix;
        return r;
    }
    Transform3D scaled(double sx, double sy, double sz) const {
        Transform3D r;
        r.matrix = Mat4::scale(sx,sy,sz) * matrix;
        return r;
    }

    Vec3 apply(const Vec3& v) const { return matrix.transform_point(v); }
};

} // namespace pml
```

- [ ] **Step 2: 实现 Camera3D + 全局单例**

```cpp
// src/pml/graphics3d/camera3d.h
#pragma once
#include "mat4.h"
#include <string>

namespace pml {

struct Camera3D {
    enum class Projection { Perspective, Orthographic };

    Vec3 position{0, 0, 300};
    Vec3 target{0, 0, 0};
    Vec3 up{0, 1, 0};
    Projection projection{Projection::Orthographic};
    double fov{45.0};
    double ortho_size{200.0};
    double near_plane{1.0};
    double far_plane{1000.0};
    double aspect{1.0};
    bool backface_culling{true};

    Mat4 view_matrix() const { return Mat4::look_at(position, target, up); }
    Mat4 projection_matrix() const;
};

// 全局相机（类似 g_current_canvas）
Camera3D& current_camera();
void reset_camera();

} // namespace pml
```

```cpp
// src/pml/graphics3d/camera3d.cpp
#include "camera3d.h"

namespace pml {

Mat4 Camera3D::projection_matrix() const {
    if (projection == Projection::Orthographic) {
        return Mat4::orthographic(ortho_size, aspect, near_plane, far_plane);
    }
    return Mat4::perspective(fov, aspect, near_plane, far_plane);
}

Camera3D& current_camera() {
    static Camera3D cam;
    return cam;
}

void reset_camera() {
    current_camera() = Camera3D{};
}

} // namespace pml
```

- [ ] **Step 3: 添加测试**

```cpp
TEST(Graphics3D, CameraOrthoProjection) {
    Camera3D cam;
    cam.projection = Camera3D::Projection::Orthographic;
    cam.ortho_size = 200;
    cam.aspect = 1.0;
    Mat4 vp = cam.projection_matrix() * cam.view_matrix();
    Vec3 p = vp.transform_point(Vec3(0,0,0));
    EXPECT_NEAR(p.x, 0.0, 1e-9);
    EXPECT_NEAR(p.y, 0.0, 1e-9);
}
```

- [ ] **Step 4: 运行测试**

Run:
```powershell
cmake --build --preset debug --target pml-tests
.\build\debug\tests\Debug\pml-tests.exe --gtest_filter=Graphics3D.*
```
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add src/pml/graphics3d/transform3d.h src/pml/graphics3d/camera3d.h src/pml/graphics3d/camera3d.cpp tests/test_graphics3d.cpp
git commit -m "feat(graphics3d): add Transform3D and Camera3D"
```

---

## Task 3: Mesh3D 数据结构

**Files:**
- Create: `src/pml/graphics3d/mesh3d.h`
- Create: `src/pml/graphics3d/mesh3d.cpp`

- [ ] **Step 1: 定义 Mesh3D**

```cpp
// src/pml/graphics3d/mesh3d.h
#pragma once
#include "vec3.h"
#include "pml/graphics/objects.h"
#include <vector>
#include <memory>

namespace pml {

struct Mesh3D {
    struct Vertex {
        Vec3 position;
        Vec3 normal;
    };
    struct Face {
        std::vector<int> indices;      // 3 或 4 个顶点索引
        std::vector<Vec3> uvs;         // 与 indices 对应的 UV（z=0）
        GraphicObject material;        // 该面的 2D 贴图素材
        Vec3 face_normal;              // 局部空间法线
    };

    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

} // namespace pml
```

- [ ] **Step 2: 实现辅助函数（法线计算、包围盒）**

```cpp
// src/pml/graphics3d/mesh3d.cpp
#include "mesh3d.h"

namespace pml {

Vec3 compute_face_normal(const Mesh3D::Face& face, const std::vector<Mesh3D::Vertex>& vertices) {
    if (face.indices.size() < 3) return Vec3(0,0,0);
    Vec3 a = vertices[face.indices[0]].position;
    Vec3 b = vertices[face.indices[1]].position;
    Vec3 c = vertices[face.indices[2]].position;
    return (b - a).cross(c - a).normalized();
}

} // namespace pml
```

- [ ] **Step 3: Commit**

```bash
git add src/pml/graphics3d/mesh3d.h src/pml/graphics3d/mesh3d.cpp
git commit -m "feat(graphics3d): add Mesh3D data structure"
```

---

## Task 4: 3D 原语工厂（cube/cuboid）

**Files:**
- Create: `src/pml/graphics3d/primitive_factory.h`
- Create: `src/pml/graphics3d/primitive_factory.cpp`

- [ ] **Step 1: 实现立方体/长方体生成**

```cpp
// src/pml/graphics3d/primitive_factory.h
#pragma once
#include "mesh3d.h"
#include "pml/graphics/objects.h"
#include <memory>

namespace pml {

std::shared_ptr<Mesh3D> make_cube(double size,
    const GraphicObject& front, const GraphicObject& back,
    const GraphicObject& left, const GraphicObject& right,
    const GraphicObject& top, const GraphicObject& bottom);

std::shared_ptr<Mesh3D> make_cuboid(double w, double h, double d,
    const GraphicObject& front, const GraphicObject& back,
    const GraphicObject& left, const GraphicObject& right,
    const GraphicObject& top, const GraphicObject& bottom);

} // namespace pml
```

```cpp
// src/pml/graphics3d/primitive_factory.cpp
#include "primitive_factory.h"
#include "mesh3d.h"

namespace pml {

static void add_quad(Mesh3D& mesh, int i0, int i1, int i2, int i3,
                     const GraphicObject& mat,
                     const std::vector<Vec3>& uvs) {
    Mesh3D::Face face;
    face.indices = {i0, i1, i2, i3};
    face.uvs = uvs;
    face.material = mat;
    face.face_normal = compute_face_normal(face, mesh.vertices);
    mesh.faces.push_back(face);
}

std::shared_ptr<Mesh3D> make_cuboid(double w, double h, double d,
    const GraphicObject& front, const GraphicObject& back,
    const GraphicObject& left, const GraphicObject& right,
    const GraphicObject& top, const GraphicObject& bottom) {
    auto mesh = std::make_shared<Mesh3D>();
    double x = w * 0.5, y = h * 0.5, z = d * 0.5;

    // 0..7 顶点顺序：左下后、右下后、右下前、左下前、左上后、右上后、右上前、左上前
    mesh->vertices = {
        Vertex{{-x,-y,-z}}, Vertex{{ x,-y,-z}}, Vertex{{ x,-y, z}}, Vertex{{-x,-y, z}},
        Vertex{{-x, y,-z}}, Vertex{{ x, y,-z}}, Vertex{{ x, y, z}}, Vertex{{-x, y, z}},
    };

    std::vector<Vec3> uv_quad = {Vec3(0,0,0), Vec3(1,0,0), Vec3(1,1,0), Vec3(0,1,0)};

    add_quad(*mesh, 3,2,6,7, front, uv_quad);   // +Z front
    add_quad(*mesh, 1,0,4,5, back, uv_quad);    // -Z back
    add_quad(*mesh, 0,3,7,4, left, uv_quad);    // -X left
    add_quad(*mesh, 2,1,5,6, right, uv_quad);   // +X right
    add_quad(*mesh, 7,6,5,4, top, uv_quad);     // +Y top
    add_quad(*mesh, 0,1,2,3, bottom, uv_quad);  // -Y bottom

    return mesh;
}

std::shared_ptr<Mesh3D> make_cube(double size,
    const GraphicObject& front, const GraphicObject& back,
    const GraphicObject& left, const GraphicObject& right,
    const GraphicObject& top, const GraphicObject& bottom) {
    return make_cuboid(size, size, size, front, back, left, right, top, bottom);
}

} // namespace pml
```

- [ ] **Step 2: Commit**

```bash
git add src/pml/graphics3d/primitive_factory.h src/pml/graphics3d/primitive_factory.cpp
git commit -m "feat(graphics3d): add cube and cuboid primitive factory"
```

---

## Task 5: PML 内置函数 builtins_3d

**Files:**
- Create: `src/pml/graphics3d/builtins_3d.h`
- Create: `src/pml/graphics3d/builtins_3d.cpp`
- Modify: `src/pml/core/types.h`（如需要新 Value 构造，本期不需要）

- [ ] **Step 1: 实现 `cube3d` / `cuboid3d` / 变换 / `camera`**

```cpp
// src/pml/graphics3d/builtins_3d.h
#pragma once
#include "pml/evaluator/environment.h"
#include <memory>

namespace pml {

void register_3d_builtins(std::shared_ptr<Environment> env);

} // namespace pml
```

```cpp
// src/pml/graphics3d/builtins_3d.cpp
#include "builtins_3d.h"
#include "camera3d.h"
#include "mesh3d.h"
#include "primitive_factory.h"
#include "transform3d.h"
#include "pml/graphics/objects.h"
#include "pml/core/kwargs.h"
#include "pml/core/error.h"
#include <memory>

namespace pml {

using namespace pml::kwargs;

// 后续步骤会用到 kw_int/kw_string/kw_double

static GraphicObject empty_material() {
    return GraphicObject("group", {}, "transparent", std::nullopt, 0.0);
}

static const GraphicObject* get_material(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key) {
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return nullptr;
    if (auto* go = it->second.as_graphic_object()) {
        return go->get();
    }
    return nullptr;
}

static Result<Value> builtin_cube3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double size = kw_double(kwargs, "size", 80.0);

    const GraphicObject* front  = get_material(kwargs, "front");
    const GraphicObject* back   = get_material(kwargs, "back");
    const GraphicObject* left   = get_material(kwargs, "left");
    const GraphicObject* right  = get_material(kwargs, "right");
    const GraphicObject* top    = get_material(kwargs, "top");
    const GraphicObject* bottom = get_material(kwargs, "bottom");

    auto empty = empty_material();
    auto mesh = make_cube(size,
        front  ? *front  : empty,
        back   ? *back   : empty,
        left   ? *left   : empty,
        right  ? *right  : empty,
        top    ? *top    : empty,
        bottom ? *bottom : empty);

    Params params;
    params.set("mesh", Value(mesh));            // shared_ptr<Mesh3D>
    params.set("transform", Value(std::make_shared<Transform3D>(Transform3D::identity())));

    GraphicObject obj("mesh3d", params, std::nullopt, std::nullopt, 0.0);
    return Value(std::make_shared<GraphicObject>(obj));
}

// rotate-y, translate3d 等类似：取出 mesh3d 的 transform，修改后返回新 GraphicObject

void register_3d_builtins(std::shared_ptr<Environment> env) {
    env->define("cube3d", Value(std::make_shared<BuiltinProcedure>("cube3d", builtin_cube3d, true)));
    // ... cuboid3d, rotate-x, rotate-y, rotate-z, translate3d, scale3d, camera
}

} // namespace pml
```

- [ ] **Step 2: 实现 rotate-y / translate3d 等变换**

每个变换函数：
1. 检查第一个参数是 mesh3d `GraphicObject`。
2. 从 params 取出当前 `Transform3D`。
3. 应用新的旋转/平移/缩放。
4. 返回新的 `GraphicObject`（浅拷贝 mesh，新 transform）。

- [ ] **Step 3: 实现 `camera` 内置函数**

解析 kwargs，设置 `current_camera()` 的各个字段。返回 `nil`。

- [ ] **Step 4: Commit**

```bash
git add src/pml/graphics3d/builtins_3d.h src/pml/graphics3d/builtins_3d.cpp
git commit -m "feat(graphics3d): add PML builtins for cube3d, transforms, camera"
```

---

## Task 6: Skia 后端 mesh3d 渲染

**Files:**
- Modify: `src/pml/backend/skia/skia_backend.cpp`
- Modify: `src/pml/graphics/render.cpp`（在渲染前根据 canvas 尺寸设置相机 aspect）

- [ ] **Step 1: 在 `draw_object` 中新增 mesh3d 分支**

```cpp
// 在 skia_backend.cpp 的 draw_object 分发中：
if (obj.shape_type == "mesh3d") {
    return draw_mesh3d(canvas, obj);
}
```

- [ ] **Step 2: 实现 `draw_mesh3d`**

```cpp
Result<void> draw_mesh3d(SkCanvas* canvas, const GraphicObject& obj) {
    auto mesh_val = obj.params.find("mesh");
    auto transform_val = obj.params.find("transform");
    if (!mesh_val || !transform_val) return {};

    auto* mesh_ptr = std::get_if<std::shared_ptr<Mesh3D>>(&mesh_val->data_or_null());
    // 实际取值需通过 Value::as_xxx；这里用伪代码表示。
    // 需要从 Value 中解出 shared_ptr<Mesh3D> 和 shared_ptr<Transform3D>。

    const Camera3D& cam = current_camera();
    Mat4 vp = cam.projection_matrix() * cam.view_matrix();

    struct ProjectedFace {
        std::vector<SkPoint> points;
        std::vector<SkPoint> uvs;
        const Mesh3D::Face* face;
        double depth;
    };
    std::vector<ProjectedFace> visible;

    for (const auto& face : mesh->faces) {
        // 背面剔除：在世界/相机空间判断法线朝向
        if (cam.backface_culling) {
            // TODO: 计算 world-space normal 与相机方向点积
        }

        // 投影顶点
        ProjectedFace pf;
        pf.face = &face;
        for (int idx : face.indices) {
            Vec3 world = transform->apply(mesh->vertices[idx].position);
            Vec3 ndc = vp.transform_point(world);
            // NDC [-1,1] → 屏幕坐标
            SkPoint p{
                static_cast<SkScalar>((ndc.x + 1.0) * 0.5 * canvas_width),
                static_cast<SkScalar>((1.0 - ndc.y) * 0.5 * canvas_height)
            };
            pf.points.push_back(p);
            pf.uvs.push_back({static_cast<SkScalar>(face.uvs[i].x * tex_width),
                              static_cast<SkScalar>(face.uvs[i].y * tex_height)});
        }
        pf.depth = ... // 面中心在相机空间的 Z
        visible.push_back(pf);
    }

    // 按 depth 从远到近排序
    std::sort(visible.begin(), visible.end(), [](const auto& a, const auto& b) {
        return a.depth > b.depth;
    });

    // 绘制每个面
    for (const auto& pf : visible) {
        // 将 material GraphicObject 绘制到 SkBitmap
        // 使用 backend.draw_to_new_surface(...) 或本地 SkSurface
        sk_sp<SkImage> tex = rasterize_material(pf.face->material, tex_width, tex_height);

        // 用 SkVertices 绘制（四边形拆为两个三角形）
        SkVertices::Builder builder(SkVertices::kTriangles_VertexMode, 6, 0);
        // 填充 6 个顶点 + UV...
        // canvas->drawVertices(builder.detach(), SkBlendMode::kSrcOver, paint);
    }

    return {};
}
```

- [ ] **Step 3: 设置相机 aspect**

在 `render_frame` 渲染前，根据 canvas 宽高更新 `current_camera().aspect = width / height`。

- [ ] **Step 4: Commit**

```bash
git add src/pml/backend/skia/skia_backend.cpp src/pml/graphics/render.cpp
git commit -m "feat(skia): render mesh3d with projection, backface culling, painter's algorithm"
```

---

## Task 7: 注册 3D 内置函数并构建模块

**Files:**
- Create: `src/pml/graphics3d/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `src/pml/api/api.cpp`

- [ ] **Step 1: 创建 CMakeLists.txt**

```cmake
# src/pml/graphics3d/CMakeLists.txt
add_library(pml_graphics3d STATIC
    mat4.cpp
    camera3d.cpp
    mesh3d.cpp
    primitive_factory.cpp
    builtins_3d.cpp
)

target_include_directories(pml_graphics3d PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/skia/include
)

target_link_libraries(pml_graphics3d PUBLIC pml_graphics pml_core)
```

- [ ] **Step 2: 顶层 CMakeLists.txt 添加子目录**

在合适位置添加：
```cmake
add_subdirectory(src/pml/graphics3d)
```

并确保链接 `pml_graphics3d` 到主目标（如 `pml_runtime` 或 `pml_api`）。

- [ ] **Step 3: api.cpp 注册**

```cpp
// src/pml/api/api.cpp
#include "pml/graphics3d/builtins_3d.h"

void PMLRuntime::init_global_env() {
    // ... 现有注册 ...
    register_3d_builtins(m_env);
}
```

- [ ] **Step 4: 编译**

Run:
```powershell
cmake --preset debug
cmake --build --preset debug --target pml_graphics3d
```
Expected: 无编译错误。

- [ ] **Step 5: Commit**

```bash
git add src/pml/graphics3d/CMakeLists.txt CMakeLists.txt src/pml/api/api.cpp
git commit -m "build: wire pml_graphics3d module into build and runtime"
```

---

## Task 8: 更多 3D 形状

**Files:**
- Modify: `src/pml/graphics3d/primitive_factory.h`
- Modify: `src/pml/graphics3d/primitive_factory.cpp`
- Modify: `src/pml/graphics3d/builtins_3d.cpp`

- [ ] **Step 1: 实现 rounded-cuboid3d**

生成长方体主体 + 圆角边/角（用 8 个球角 + 12 个圆柱边）。先实现简化版：对每条边用分段圆柱近似，每个角用八分球近似。

- [ ] **Step 2: 实现 cone3d / plane3d / sphere3d**

- `cone3d`：圆锥底面圆 + 侧面三角扇。
- `plane3d`：单个四边形面。
- `sphere3d`：经纬度网格。

- [ ] **Step 3: 注册新内置函数**

```cpp
env->define("rounded-cuboid3d", ...);
env->define("cone3d", ...);
env->define("plane3d", ...);
env->define("sphere3d", ...);
```

- [ ] **Step 4: Commit**

```bash
git add src/pml/graphics3d/primitive_factory.h src/pml/graphics3d/primitive_factory.cpp src/pml/graphics3d/builtins_3d.cpp
git commit -m "feat(graphics3d): add rounded-cuboid, cone, plane, sphere primitives"
```

---

## Task 9: 测试与示例

**Files:**
- Modify: `tests/builtins_smoke.cpp`

- [ ] **Step 1: 烟雾测试**

在 `tests/builtins_smoke.cpp` 新增：

```cpp
TEST(BuiltinsSmoke, Cube3dCreatesGraphicObject) {
    PMLRuntime rt;
    auto result = rt.execute("(cube3d :size 80 :front (rect 0 0 80 80 :fill \"#7CB342\"))");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->is_graphic_object());
}

TEST(BuiltinsSmoke, CameraSetsGlobalState) {
    PMLRuntime rt;
    auto result = rt.execute("(camera :position '(0 0 300) :projection 'orthographic :size 200)");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_nil());
}
```

- [ ] **Step 2: 运行测试**

Run:
```powershell
cmake --build --preset debug --target pml-builtins-smoke
.\build\debug\tests\Debug\pml-builtins-smoke.exe
```
Expected: PASS

- [ ] **Step 3: Commit**

```bash
git add tests/builtins_smoke.cpp
git commit -m "test: add 3d smoke tests"
```

---

## Task 10: 文档更新

**Files:**
- Modify: `AGENTS.md`

- [ ] **Step 1: 在 AGENTS.md §3 新增差异项**

```markdown
### Implemented differently (intentional ports)
- **3D graphics**: C++ port adds native 3D primitives (`cube3d`, `cuboid3d`,
  `rounded-cuboid3d`, `cone3d`, `plane3d`, `sphere3d`), per-face PML 2D material
  mapping, 3D transforms (`rotate-x/y/z`, `translate3d`, `scale3d`), and a
  perspective/orthographic `camera`. Python reference has no 3D support.
```

- [ ] **Step 2: 在 §9 注册列表中追加**

```cpp
register_3d_builtins(m_env);          // cube3d, cuboid3d, rotate-x/y/z, camera ✓
```

- [ ] **Step 3: Commit**

```bash
git add AGENTS.md
git commit -m "docs: document new 3D support in AGENTS.md"
```

---

## 验证命令清单

```powershell
# 全量构建
cmake --build --preset debug

# 单元测试
.\build\debug\tests\Debug\pml-tests.exe --gtest_filter=Graphics3D.*

# 烟雾测试
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# 3D 示例（可在任意 PML 文件中使用 cube3d/rotate-y/camera 等 API）
.\build\debug\bin\Debug\pml.exe .\my_3d_demo.pml -o .\
```

---

## 注意事项

- 所有 C++ 代码必须放在 `namespace pml` 内。
- 不要对 3D 类型使用 `throw`，统一返回 `Result<T>`。
- 新增内置函数注册前，确保对应实现已存在，否则链接失败。
- `Value` 本期不新增 Box kind；`shared_ptr<Mesh3D>` 通过 `Value` 构造函数模板隐式支持，可能需要显式添加 `Value(std::shared_ptr<Mesh3D>)` 构造函数。
- Skia 贴图绘制需确保 material 的坐标系与 UV 方向一致：Skia 的 Y 轴向下，UV v=0 对应 material 顶部。
