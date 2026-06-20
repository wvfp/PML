# PML Examples

本目录包含 PML 语言全部主要功能的可运行示例，以及若干复杂项目示例。所有示例默认使用 Skia 后端，输出到 `examples/output/`。

## 快速开始

```powershell
# 1. 生成示例素材（首次运行必需）
pml.exe examples/assets/gen.pml -o examples/assets

# 2. 运行单个示例
pml.exe examples/02-graphics/01_circle_rect_ellipse_line.pml -o examples/output

# 3. 一键运行所有示例
powershell -ExecutionPolicy Bypass -File examples/run_all.ps1
```

> 若 `pml.exe` 不在 PATH 中，请使用构建产物路径，例如：
> `g:\Project\PML\build\debug\src\pml\cli\Debug\pml.exe`

## 目录结构

```
examples/
├── README.md                         # 本文件
├── run_all.ps1                       # 一键渲染脚本
├── assets/                           # 示例素材与生成脚本
├── output/                           # 渲染输出目录
├── 01-core/                          # 核心语言
├── 02-graphics/                      # 图形原语
├── 03-canvas-render/                 # Canvas 与渲染
├── 04-animation/                     # 动画
├── 05-layer-composition-filter/      # 图层、合成、滤镜
├── 06-sprites-style-palette/         # 精灵组件、样式、调色板
├── 08-asset-bitmap/                  # 资源加载与 Bitmap
└── 09-projects/                      # 复杂项目
```

## 功能索引

### 01 核心语言

| 功能 | 文件 | 命令 | 输出 |
|------|------|------|------|
| 变量、算术、字符串 | `01-core/01_variables_and_math.pml` | `pml.exe 01-core/01_variables_and_math.pml` | 终端输出 |
| 函数与 lambda | `01-core/02_functions.pml` | `pml.exe 01-core/02_functions.pml -o output` | `02_functions.png` |
| 条件与循环 | `01-core/03_conditionals_and_loops.pml` | `pml.exe 01-core/03_conditionals_and_loops.pml -o output` | `03_conditionals_and_loops.png` |
| 列表与高阶函数 | `01-core/04_lists_and_higher_order.pml` | `pml.exe 01-core/04_lists_and_higher_order.pml -o output` | `04_lists_and_higher_order.png` |
| 向量与哈希表 | `01-core/05_vectors_and_hashes.pml` | `pml.exe 01-core/05_vectors_and_hashes.pml -o output` | `05_vectors_and_hashes.png` |
| 宏 | `01-core/06_macros.pml` | `pml.exe 01-core/06_macros.pml -o output` | `06_macros.png` |
| 模块导入 | `01-core/07_modules/main.pml` | `pml.exe 01-core/07_modules/main.pml -o output` | `main.png` |

### 02 图形原语

| 功能 | 文件 | 输出 |
|------|------|------|
| 圆、矩形、椭圆、线段 | `02-graphics/01_circle_rect_ellipse_line.pml` | `01_circle_rect_ellipse_line.png` |
| 多边形与路径 | `02-graphics/02_polygon_and_path.pml` | `02_polygon_and_path.png` |
| 文本与标签 | `02-graphics/03_text_and_labels.pml` | `03_text_and_labels.png` |
| 组合与变换 | `02-graphics/04_groups_and_transforms.pml` | `04_groups_and_transforms.png` |
| 颜色函数 | `02-graphics/05_colors_and_styles.pml` | `05_colors_and_styles.png` |

### 03 Canvas 与渲染

| 功能 | 文件 | 输出 |
|------|------|------|
| Canvas 与 add | `03-canvas-render/01_canvas_and_add.pml` | `01_canvas_and_add.png` |
| 精灵画布 | `03-canvas-render/02_sprite_canvas.pml` | `02_sprite_canvas.png` + `.meta.json` |
| Render Set | `03-canvas-render/03_render_set.pml` | `render_set_icon@1x.png` 等 |
| Sprite Sheet | `03-canvas-render/04_render_spritesheet.pml` | `04_render_spritesheet.png` + `.json` |

### 04 动画

| 功能 | 文件 | 输出 |
|------|------|------|
| animate / play / stop | `04-animation/01_animate_and_play.pml` | `01_animate_and_play.gif` |
| seek / animation-state | `04-animation/02_seek_and_state.pml` | `02_seek_and_state.png` |
| parallel / sequence | `04-animation/03_parallel_and_sequence.pml` | `03_parallel_and_sequence.gif` |
| every-frame | `04-animation/04_every_frame.pml` | `04_every_frame.gif` |
| 合成层动画 | `04-animation/05_animated_composition.pml` | `05_animated_composition.gif` |

### 05 图层、合成与滤镜

| 功能 | 文件 | 输出 |
|------|------|------|
| 基础图层 | `05-layer-composition-filter/01_basic_layers.pml` | `01_basic_layers.png` |
| 图层组 | `05-layer-composition-filter/02_layer_groups.pml` | `02_layer_groups.png` |
| 混合模式 | `05-layer-composition-filter/03_blend_modes.pml` | `03_blend_modes.png` |
| 阴影与发光 | `05-layer-composition-filter/04_drop_shadow_and_glow.pml` | `04_drop_shadow_and_glow.png` |
| 色彩滤镜 | `05-layer-composition-filter/05_color_filters.pml` | `05_color_filters.png` |
| 模糊/锐化/卷积 | `05-layer-composition-filter/06_blur_sharpen_convolution.pml` | `06_blur_sharpen_convolution.png` |
| Bitmap Layer 场景 | `05-layer-composition-filter/07_bitmap_layer_scene.pml` | `07_bitmap_layer_scene.png` |
| UI 面板 | `05-layer-composition-filter/08_ui_panel.pml` | `08_ui_panel.png` |
| 照片级滤镜 | `05-layer-composition-filter/09_photo_filters.pml` | `09_photo_filters.png` |

### 06 精灵组件、样式、调色板

| 功能 | 文件 | 输出 |
|------|------|------|
| 预定义样式 | `06-sprites-style-palette/01_predefined_styles.pml` | `01_predefined_styles.png` |
| 自定义样式 | `06-sprites-style-palette/02_define_style.pml` | `02_define_style.png` |
| 预定义调色板 | `06-sprites-style-palette/03_predefined_palettes.pml` | `03_predefined_palettes.png` |
| 自定义调色板 | `06-sprites-style-palette/04_define_palette.pml` | `04_define_palette.png` |
| 角色组件 | `06-sprites-style-palette/05_character_component.pml` | `05_character_component.png` |
| UI 组件 | `06-sprites-style-palette/06_ui_components.pml` | `06_ui_components.png` |

### 08 资源加载与 Bitmap

| 功能 | 文件 | 输出 |
|------|------|------|
| 加载 Sprite Sheet | `08-asset-bitmap/01_load_spritesheet.pml` | `01_load_spritesheet.png` |
| image 对象 | `08-asset-bitmap/02_image_object.pml` | `02_image_object.png` |
| asset-path? / clear-assets! | `08-asset-bitmap/03_asset_path_and_cache.pml` | `03_asset_path_and_cache.png` |
| Bitmap Layer 裁剪 | `08-asset-bitmap/04_bitmap_layer_cropping.pml` | `04_bitmap_layer_cropping.png` |

### 09 复杂项目

| 项目 | 入口文件 | 输出 |
|------|----------|------|
| 视差卷轴平台场景 | `09-projects/01_parallax_platformer/main.pml` | `01_parallax_platformer.png` |
| 行走循环角色 | `09-projects/02_walk_cycle_character/main.pml` | `02_walk_cycle_character.gif` |
| 卡牌游戏 UI | `09-projects/03_card_game_ui/main.pml` | `03_card_game_ui.png` |
| 程序化风景 | `09-projects/04_generative_landscape/main.pml` | `04_generative_landscape.png` |
| 多模块 logo 复刻 | `09-projects/05_doubao_logo/main.pml` | `03_doubao_logo.png` |

## 新增示例约定

1. 每个示例一个主 `.pml` 文件；复杂项目可拆分子模块。
2. 输出文件名与示例文件名一致（项目示例使用 `01_` 前缀）。
3. 素材统一放在 `examples/assets/`，示例内引用使用相对路径，如 `../assets/xxx.png`。
4. 模块文件使用 `(provide ...)` 导出，主文件使用 `(import "xxx.pml" as prefix)` 并访问 `prefix/member`。
5. 需要 Skia 后端的示例开头调用 `(set-backend! "skia")`。
6. 动画示例尽量输出 GIF；静态示例输出 PNG。
7. 将新示例添加到本 README 索引表，并更新 `run_all.ps1` 的扫描逻辑（默认递归已自动覆盖）。
