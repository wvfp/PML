# PML Examples 全景实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 PML 的每一个主要功能写一个可运行的 example，并补充若干复杂项目示例，使用户能通过 `examples/` 目录自学 PML 全部能力。

**Architecture：** 每个 example 是一个独立的 `.pml` 文件（复杂项目拆成主文件 + 辅助模块），输出到 `examples/output/`；通过 `examples/README.md` 建立功能索引与运行说明；最终提供一键脚本渲染并验证所有 example。

**Tech Stack：** PML C++ CLI (`pml.exe`)、Skia/GIF backend、`examples/assets/gen.pml` 生成的占位素材。

---

## 目录结构

```
examples/
├── README.md                         # 功能索引表与运行说明
├── run_all.ps1                       # 一键渲染所有 example
├── assets/
│   ├── gen.pml                       # 已存在：生成占位素材
│   ├── walk_sheet.json               # 已存在：图集描述
│   └── (生成的 .png 素材)
├── 01-core/                          # 核心语言
├── 02-graphics/                      # 图形原语
├── 03-canvas-render/                 # Canvas 与渲染
├── 04-animation/                     # 动画
├── 05-layer-composition-filter/      # 图层、合成、滤镜
├── 06-sprites-style-palette/         # 精灵组件、样式、调色板
├── 07-skeleton-ik/                   # 骨骼与 IK
├── 08-asset-bitmap/                  # 资源加载（已存在 4 个）
└── 09-projects/                      # 复杂项目
```

---

### Task 1: 核心语言 Examples

**Files:**
- Create: `examples/01-core/01_variables_and_math.pml`
- Create: `examples/01-core/02_functions.pml`
- Create: `examples/01-core/03_conditionals_and_loops.pml`
- Create: `examples/01-core/04_lists_and_higher_order.pml`
- Create: `examples/01-core/05_vectors_and_hashes.pml`
- Create: `examples/01-core/06_macros.pml`
- Create: `examples/01-core/07_modules/
           examples/01-core/07_modules/shapes.pml
           examples/01-core/07_modules/main.pml`
- Modify: `examples/README.md`（新增核心语言索引）

- [ ] **Step 1: 变量、算术与字符串**
  - 文件：`examples/01-core/01_variables_and_math.pml`
  - 内容：定义变量、四则运算、`string-append`、`number->string`、`print`；输出计算结果到终端。
  - 运行：`pml.exe examples/01-core/01_variables_and_math.pml`
  - 预期：终端打印变量与计算结果。

- [ ] **Step 2: 函数与 lambda**
  - 文件：`examples/01-core/02_functions.pml`
  - 内容：`(define (area r) ...)`、`(lambda (x) ...)`、高阶函数 `compose`；用 `(circle 0 0 r)` 验证。
  - 运行：`pml.exe examples/01-core/02_functions.pml -o examples/output`
  - 预期：生成 `02_functions.png`（三个不同半径的圆）。

- [ ] **Step 3: 条件与循环**
  - 文件：`examples/01-core/03_conditionals_and_loops.pml`
  - 内容：`if`/`cond`/`and`/`or`、递归生成列表、`map`、`filter`、`reduce` 绘制条状图。
  - 运行：`pml.exe examples/01-core/03_conditionals_and_loops.pml -o examples/output`
  - 预期：生成 `03_conditionals_and_loops.png`（不同颜色的条形）。

- [ ] **Step 4: 列表与高阶函数**
  - 文件：`examples/01-core/04_lists_and_higher_order.pml`
  - 内容：用 `range`/`map` 生成一排圆，用 `reduce` 计算总面积并在画布上 `text` 显示。
  - 运行：`pml.exe examples/01-core/04_lists_and_higher_order.pml -o examples/output`
  - 预期：生成 `04_lists_and_higher_order.png`。

- [ ] **Step 5: 向量与哈希表**
  - 文件：`examples/01-core/05_vectors_and_hashes.pml`
  - 内容：`make-vector`、`vector-set!`、`make-hash`、`hash-set!`、用哈希表做颜色配置表。
  - 运行：`pml.exe examples/01-core/05_vectors_and_hashes.pml -o examples/output`
  - 预期：生成 `05_vectors_and_hashes.png`（用哈希表颜色绘制的网格）。

- [ ] **Step 6: 宏**
  - 文件：`examples/01-core/06_macros.pml`
  - 内容：`defmacro` 定义 `when`、`unless`、`for-each` 简化语法；用宏批量绘制形状。
  - 运行：`pml.exe examples/01-core/06_macros.pml -o examples/output`
  - 预期：生成 `06_macros.png`。

- [ ] **Step 7: 模块导入**
  - 文件：
    - `examples/01-core/07_modules/shapes.pml`：`(provide circle-row rect-grid)`
    - `examples/01-core/07_modules/main.pml`：`(import "shapes.pml" as shapes)` 并渲染。
  - 运行：`pml.exe examples/01-core/07_modules/main.pml -o examples/output`
  - 预期：生成 `main.png`（模块导出的图形函数绘制结果）。

---

### Task 2: 图形原语 Examples

**Files:**
- Create: `examples/02-graphics/01_circle_rect_ellipse_line.pml`
- Create: `examples/02-graphics/02_polygon_and_path.pml`
- Create: `examples/02-graphics/03_text_and_labels.pml`
- Create: `examples/02-graphics/04_groups_and_transforms.pml`
- Create: `examples/02-graphics/05_colors_and_styles.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 基本形状**
  - 文件：`examples/02-graphics/01_circle_rect_ellipse_line.pml`
  - 内容：`circle`、`rect`（含圆角）、`ellipse`、`line` 综合展示。
  - 运行：`pml.exe examples/02-graphics/01_circle_rect_ellipse_line.pml -o examples/output`
  - 预期：生成 `01_circle_rect_ellipse_line.png`。

- [ ] **Step 2: 多边形与路径**
  - 文件：`examples/02-graphics/02_polygon_and_path.pml`
  - 内容：`polygon` 绘制星形、`path` 绘制贝塞尔曲线/心形/箭头。
  - 运行：`pml.exe examples/02-graphics/02_polygon_and_path.pml -o examples/output`
  - 预期：生成 `02_polygon_and_path.png`。

- [ ] **Step 3: 文本与标签**
  - 文件：`examples/02-graphics/03_text_and_labels.pml`
  - 内容：`text` 显示标题、坐标标签、用图形做图例说明。
  - 运行：`pml.exe examples/02-graphics/03_text_and_labels.pml -o examples/output`
  - 预期：生成 `03_text_and_labels.png`。

- [ ] **Step 4: 组合与变换**
  - 文件：`examples/02-graphics/04_groups_and_transforms.pml`
  - 内容：`group` + `with-transform`、`translate-object`、`rotate-object`、`scale-object` 绘制旋转齿轮/花朵。
  - 运行：`pml.exe examples/02-graphics/04_groups_and_transforms.pml -o examples/output`
  - 预期：生成 `04_groups_and_transforms.png`。

- [ ] **Step 5: 颜色函数**
  - 文件：`examples/02-graphics/05_colors_and_styles.pml`
  - 内容：`rgb`、`rgba`、`fill`、`stroke`、`stroke-width`、`no-fill`、`no-stroke` 绘制色轮/渐变条。
  - 运行：`pml.exe examples/02-graphics/05_colors_and_styles.pml -o examples/output`
  - 预期：生成 `05_colors_and_styles.png`。

---

### Task 3: Canvas 与渲染 Examples

**Files:**
- Create: `examples/03-canvas-render/01_canvas_and_add.pml`
- Create: `examples/03-canvas-render/02_sprite_canvas.pml`
- Create: `examples/03-canvas-render/03_render_set.pml`
- Create: `examples/03-canvas-render/04_render_spritesheet.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: Canvas 与 add**
  - 文件：`examples/03-canvas-render/01_canvas_and_add.pml`
  - 内容：创建 `(canvas 400 400)`，多次 `add` 不同图形，调用 `(render "canvas.png")`。
  - 运行：`pml.exe examples/03-canvas-render/01_canvas_and_add.pml -o examples/output`
  - 预期：生成 `01_canvas_and_add.png`。

- [ ] **Step 2: Sprite Canvas**
  - 文件：`examples/03-canvas-render/02_sprite_canvas.pml`
  - 内容：`(sprite-canvas ...)` 创建带透明背景的精灵画布，渲染 PNG。
  - 运行：`pml.exe examples/03-canvas-render/02_sprite_canvas.pml -o examples/output`
  - 预期：生成 `02_sprite_canvas.png`。

- [ ] **Step 3: Render Set**
  - 文件：`examples/03-canvas-render/03_render_set.pml`
  - 内容：用 `render-set` 批量导出多帧/多尺寸图标。
  - 运行：`pml.exe examples/03-canvas-render/03_render_set.pml -o examples/output`
  - 预期：生成多个 `render_set_*.png`。

- [ ] **Step 4: Render Spritesheet**
  - 文件：`examples/03-canvas-render/04_render_spritesheet.pml`
  - 内容：用 `render-spritesheet` 把多个图形排列成一张 spritesheet。
  - 运行：`pml.exe examples/03-canvas-render/04_render_spritesheet.pml -o examples/output`
  - 预期：生成 `04_render_spritesheet.png` + JSON。

---

### Task 4: 动画 Examples

**Files:**
- Create: `examples/04-animation/01_animate_and_play.pml`
- Create: `examples/04-animation/02_seek_and_state.pml`
- Create: `examples/04-animation/03_parallel_and_sequence.pml`
- Create: `examples/04-animation/04_every_frame.pml`
- Create: `examples/04-animation/05_animated_composition.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: animate / play / stop**
  - 文件：`examples/04-animation/01_animate_and_play.pml`
  - 内容：对图形属性做 `(animate ...)`，然后 `(play)`、`(stop)`，导出 GIF/PNG 序列。
  - 运行：`pml.exe examples/04-animation/01_animate_and_play.pml -o examples/output`
  - 预期：生成 `01_animate_and_play.gif` 或序列帧。

- [ ] **Step 2: seek 与 animation-state**
  - 文件：`examples/04-animation/02_seek_and_state.pml`
  - 内容：创建动画后 `(seek 0.5)` 抓中间帧，用 `(animation-state)` 查询时间。
  - 运行：`pml.exe examples/04-animation/02_seek_and_state.pml -o examples/output`
  - 预期：生成 `02_seek_and_state.png`。

- [ ] **Step 3: parallel / sequence**
  - 文件：`examples/04-animation/03_parallel_and_sequence.pml`
  - 内容：用 `(parallel ...)` 和 `(sequence ...)` 组合多个动画，导出关键帧。
  - 运行：`pml.exe examples/04-animation/03_parallel_and_sequence.pml -o examples/output`
  - 预期：生成 `03_parallel_and_sequence.gif`。

- [ ] **Step 4: every-frame**
  - 文件：`examples/04-animation/04_every_frame.pml`
  - 内容：`(every-frame ...)` 自定义逐帧逻辑，生成粒子/轨迹效果。
  - 运行：`pml.exe examples/04-animation/04_every_frame.pml -o examples/output`
  - 预期：生成 `04_every_frame.gif`。

- [ ] **Step 5: 合成层动画**
  - 文件：`examples/04-animation/05_animated_composition.pml`
  - 内容：对 composition 的 layer offset/opacity 做动画。
  - 运行：`pml.exe examples/04-animation/05_animated_composition.pml -o examples/output`
  - 预期：生成 `05_animated_composition.gif`。

---

### Task 5: 图层、合成与滤镜 Examples

**Files：** 已有 `examples/05-layer-composition-filter/`（把现有 4 个 example 移入并补全）。
- Create: `examples/05-layer-composition-filter/01_basic_layers.pml`
- Create: `examples/05-layer-composition-filter/02_layer_groups.pml`
- Create: `examples/05-layer-composition-filter/03_blend_modes.pml`
- Create: `examples/05-layer-composition-filter/04_drop_shadow_and_glow.pml`
- Create: `examples/05-layer-composition-filter/05_color_filters.pml`
- Create: `examples/05-layer-composition-filter/06_blur_sharpen_convolution.pml`
- Move: 现有 `examples/01_bitmap_layer_scene.pml` → `examples/05-layer-composition-filter/07_bitmap_layer_scene.pml`
- Move: 现有 `examples/02_ui_panel.pml` → `examples/05-layer-composition-filter/08_ui_panel.pml`
- Move: 现有 `examples/04_photo_filters.pml` → `examples/05-layer-composition-filter/09_photo_filters.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 基础图层**
  - 文件：`examples/05-layer-composition-filter/01_basic_layers.pml`
  - 内容：用 `make-layer` 创建纯色/形状图层，调整 opacity、visible、locked。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `01_basic_layers.png`。

- [ ] **Step 2: 图层组**
  - 文件：`examples/05-layer-composition-filter/02_layer_groups.pml`
  - 内容：`make-group` 把多个 layer 编组，整体变换/位移。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `02_layer_groups.png`。

- [ ] **Step 3: 混合模式**
  - 文件：`examples/05-layer-composition-filter/03_blend_modes.pml`
  - 内容：多个彩色圆形用不同 `BlendMode`（multiply/screen/overlay 等）叠加。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `03_blend_modes.png`。

- [ ] **Step 4: 阴影/发光滤镜**
  - 文件：`examples/05-layer-composition-filter/04_drop_shadow_and_glow.pml`
  - 内容：`drop-shadow`、`inner-shadow`、`outer-glow`、`inner-glow`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `04_drop_shadow_and_glow.png`。

- [ ] **Step 5: 色彩滤镜**
  - 文件：`examples/05-layer-composition-filter/05_color_filters.pml`
  - 内容：`color-adjust`（brightness/contrast/saturation/hue/grayscale/sepia/invert）、`levels`、`curves`、`threshold`、`posterize`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `05_color_filters.png`。

- [ ] **Step 6: 模糊/锐化/卷积**
  - 文件：`examples/05-layer-composition-filter/06_blur_sharpen_convolution.pml`
  - 内容：`blur`、`sharpen`、`edge-detect`、`emboss`、`convolution`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `06_blur_sharpen_convolution.png`。

- [ ] **Step 7: 整理现有 example 到该目录**
  - 移动并更新内部 asset 路径（`assets/...` → `../assets/...`）。
  - 运行并确认输出一致。

---

### Task 6: 精灵组件、样式、调色板 Examples

**Files：**
- Create: `examples/06-sprites-style-palette/01_predefined_styles.pml`
- Create: `examples/06-sprites-style-palette/02_define_style.pml`
- Create: `examples/06-sprites-style-palette/03_predefined_palettes.pml`
- Create: `examples/06-sprites-style-palette/04_define_palette.pml`
- Create: `examples/06-sprites-style-palette/05_character_component.pml`
- Create: `examples/06-sprites-style-palette/06_ui_components.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 预定义样式**
  - 文件：`examples/06-sprites-style-palette/01_predefined_styles.pml`
  - 内容：用 `cel`、`pixel`、`flat` 等预定义 style 绘制角色。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `01_predefined_styles.png`。

- [ ] **Step 2: 自定义样式**
  - 文件：`examples/06-sprites-style-palette/02_define_style.pml`
  - 内容：`(define-style my-style ...)` + `(use-style my-style)`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `02_define_style.png`。

- [ ] **Step 3: 预定义调色板**
  - 文件：`examples/06-sprites-style-palette/03_predefined_palettes.pml`
  - 内容：使用内置 palette 上色。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `03_predefined_palettes.png`。

- [ ] **Step 4: 自定义调色板**
  - 文件：`examples/06-sprites-style-palette/04_define_palette.pml`
  - 内容：`(define-palette ...)` + `(palette-ref ...)`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `04_define_palette.png`。

- [ ] **Step 5: 角色组件**
  - 文件：`examples/06-sprites-style-palette/05_character_component.pml`
  - 内容：用 `(character ...)`、`(body ...)`、`(head ...)`、`(eyes ...)` 等组件拼角色。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `05_character_component.png`。

- [ ] **Step 6: UI 组件**
  - 文件：`examples/06-sprites-style-palette/06_ui_components.pml`
  - 内容：用 `(button ...)`、`(panel ...)`、`(health-bar ...)`、`(icon ...)` 拼 UI。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `06_ui_components.png`。

---

### Task 7: 骨骼与 IK Examples

**Files：**
- Create: `examples/07-skeleton-ik/01_def_skeleton.pml`
- Create: `examples/07-skeleton-ik/02_instantiate_and_joint_position.pml`
- Create: `examples/07-skeleton-ik/03_ik_solve_ccd.pml`
- Create: `examples/07-skeleton-ik/04_ik_solve_fabrik.pml`
- Create: `examples/07-skeleton-ik/05_bind_skin.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 定义骨骼**
  - 文件：`examples/07-skeleton-ik/01_def_skeleton.pml`
  - 内容：`(defskeleton arm ...)` 定义关节链，可视化骨骼连线。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `01_def_skeleton.png`。

- [ ] **Step 2: 实例化与关节位置**
  - 文件：`examples/07-skeleton-ik/02_instantiate_and_joint_position.pml`
  - 内容：`(instantiate-skeleton ...)`、`(joint-position ...)` 读取关节坐标并绘制。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `02_instantiate_and_joint_position.png`。

- [ ] **Step 3: CCD IK**
  - 文件：`examples/07-skeleton-ik/03_ik_solve_ccd.pml`
  - 内容：`(ik-solve :method "ccd" ...)` 让机械臂/触手伸向目标点。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `03_ik_solve_ccd.png`。

- [ ] **Step 4: FABRIK IK**
  - 文件：`examples/07-skeleton-ik/04_ik_solve_fabrik.pml`
  - 内容：`(ik-solve :method "fabrik" ...)` 实现软体/链条跟随目标。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `04_ik_solve_fabrik.png`。

- [ ] **Step 5: 皮肤绑定**
  - 文件：`examples/07-skeleton-ik/05_bind_skin.pml`
  - 内容：`(bind-skin ...)` 把图形/组件绑定到骨骼并驱动变换。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `05_bind_skin.png`。

---

### Task 8: 资源加载 Examples（已存在，需整理）

**Files：**
- Move: `examples/03_spritesheet_character.pml` → `examples/08-asset-bitmap/01_load_spritesheet.pml`
- Create: `examples/08-asset-bitmap/02_image_object.pml`
- Create: `examples/08-asset-bitmap/03_asset_path_and_cache.pml`
- Create: `examples/08-asset-bitmap/04_bitmap_layer_cropping.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 整理已有 spritesheet example**
  - 移动并更新 asset 路径为 `../assets/...`。
  - 运行并确认输出一致。

- [ ] **Step 2: image 对象**
  - 文件：`examples/08-asset-bitmap/02_image_object.pml`
  - 内容：`(image "..." :x ... :y ... :width ... :height ... :crop ...)` 绘制。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `02_image_object.png`。

- [ ] **Step 3: asset-path? 与 clear-assets!**
  - 文件：`examples/08-asset-bitmap/03_asset_path_and_cache.pml`
  - 内容：演示 `asset-path?`、`load-image`、`clear-assets!`。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `03_asset_path_and_cache.png`。

- [ ] **Step 4: bitmap-layer 裁剪**
  - 文件：`examples/08-asset-bitmap/04_bitmap_layer_cropping.pml`
  - 内容：用 `bitmap-layer` + `:crop` 做九宫格/局部放大。
  - 运行：`pml.exe ... -o examples/output`
  - 预期：生成 `04_bitmap_layer_cropping.png`。

---

### Task 9: 复杂项目 Examples

**Files：**
- Create: `examples/09-projects/01_parallax_platformer/
           examples/09-projects/01_parallax_platformer/main.pml
           examples/09-projects/01_parallax_platformer/tiles.pml`
- Create: `examples/09-projects/02_walk_cycle_character/
           examples/09-projects/02_walk_cycle_character/main.pml
           examples/09-projects/02_walk_cycle_character/body.pml`
- Create: `examples/09-projects/03_card_game_ui/
           examples/09-projects/03_card_game_ui/main.pml`
- Create: `examples/09-projects/04_generative_landscape/
           examples/09-projects/04_generative_landscape/main.pml`
- Create: `examples/09-projects/05_ik_tentacle/
           examples/09-projects/05_ik_tentacle/main.pml`
- Modify: `examples/README.md`

- [ ] **Step 1: 视差卷轴平台场景**
  - 目录：`examples/09-projects/01_parallax_platformer/`
  - 内容：多层背景以不同速度移动，地面砖块随机排列，角色立于地面，导出静态帧。
  - 运行：`pml.exe examples/09-projects/01_parallax_platformer/main.pml -o examples/output`
  - 预期：生成 `01_parallax_platformer.png`。

- [ ] **Step 2: 行走循环角色**
  - 目录：`examples/09-projects/02_walk_cycle_character/`
  - 内容：组合 `(body ...)`、`(head ...)`、`(eyes ...)`、`(outfit ...)`，用动画让四肢摆动，导出 GIF。
  - 运行：`pml.exe .../main.pml -o examples/output`
  - 预期：生成 `02_walk_cycle_character.gif`。

- [ ] **Step 3: 卡牌游戏 UI**
  - 目录：`examples/09-projects/03_card_game_ui/`
  - 内容：面板、卡牌、按钮、血条、金币图标，带阴影与滤镜，展示完整 UI 布局。
  - 运行：`pml.exe .../main.pml -o examples/output`
  - 预期：生成 `03_card_game_ui.png`。

- [ ] **Step 4: 程序化风景**
  - 目录：`examples/09-projects/04_generative_landscape/`
  - 内容：用随机/噪声函数生成山脉、云层、太阳，用路径和多边形绘制。
  - 运行：`pml.exe .../main.pml -o examples/output`
  - 预期：生成 `04_generative_landscape.png`。

- [ ] **Step 5: IK 触手**
  - 目录：`examples/09-projects/05_ik_tentacle/`
  - 内容：骨骼链 + FABRIK 跟随鼠标/目标点，渲染多帧形成摆动触手 GIF。
  - 运行：`pml.exe .../main.pml -o examples/output`
  - 预期：生成 `05_ik_tentacle.gif`。

---

### Task 10: 索引 README 与一键脚本

**Files：**
- Create: `examples/README.md`
- Create: `examples/run_all.ps1`
- Modify: `docs/superpowers/plans/2026-06-20-pml-examples.md`（标记完成）

- [ ] **Step 1: 编写 README.md**
  - 功能索引表：feature → example 文件 → 运行命令 → 输出文件名。
  - 目录结构说明。
  - 新增 example 的约定（文件名、输出、asset 路径）。

- [ ] **Step 2: 编写 run_all.ps1**
  - 先运行 `assets/gen.pml` 生成素材。
  - 递归遍历 `examples/` 下所有 `.pml` 文件，过滤 `assets/`、`output/`、`README.md`。
  - 对每个 example 执行 `pml.exe <file> -o examples/output`。
  - 统计成功/失败并输出报告。

- [ ] **Step 3: 运行并验证**
  - 执行 `powershell -ExecutionPolicy Bypass -File examples/run_all.ps1`。
  - 确认无崩溃，所有 example 正常输出到 `examples/output/`。
  - 运行全部单元测试：`pml-builtins-smoke`、`pml-tests`、`pml-layer-test`、`pml-filter-test`。

---

## 验收标准

1. 每个功能至少有一个独立可运行的 example。
2. 复杂项目 example 覆盖“游戏场景、UI、动画、程序化生成、骨骼 IK”五个方向。
3. `examples/README.md` 能作为功能速查表。
4. `examples/run_all.ps1` 能一键渲染全部 example 并报告成功/失败。
5. 所有 example 渲染不崩溃，输出文件位于 `examples/output/`。
6. 现有测试套件继续通过。

## 已知依赖与限制

- 依赖 `examples/assets/gen.pml` 生成占位 PNG；首次运行 `run_all.ps1` 会自动生成。
- Skia backend 目前对 `load-spritesheet`/`image`/`bitmap-layer` 的 PNG 解码已走 libpng 路径，因此所有 asset example 可运行。
- 部分滤镜（inner-glow、outer-glow、bevel-emboss 等）在 Skia backend 可能返回 `filter_not_supported`，相关 example 应 gracefully 降级或仅使用已支持滤镜。
- 动画/IK example 若导出 GIF，需要 `PML_BUILD_GIF=ON`；若只导出关键帧 PNG 则不依赖 GIF backend。
