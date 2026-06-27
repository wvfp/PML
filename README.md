<div align="center">

![PML Logo](docs/assets/logo.png)

# PML — 图片标记语言

**代码即图像**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C++-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Skia](https://img.shields.io/badge/Skia-GPU-red)](https://skia.org/)

PML（Picture Markup Language）是一种基于 S 表达式的领域特定语言，用代码描述图像。
编写 PML 脚本，即可生成矢量图、像素艺术、UI 界面、游戏素材、动画 GIF 乃至 3D 场景。

</div>

---

## 简介

PML 将编程语言的表达能力与图像渲染引擎结合。你可以用变量、函数、循环和条件语句来描述图形，用模块化组织素材，用 Shader 实现程序化生成。

```scheme
; hello.pml — 你的第一张 PML 图像
(set-backend! "skia")
(canvas 400 300 :bg "#F8F9FA")

(add (circle 200 150 80
             :fill "#FF6B6B"
             :stroke "#C92A2A"
             :stroke-width 3))

(add (rect 150 100 100 80
           :fill "#51CF66"
           :rx 8))

(add (text 140 180 "Hello, PML!"
           :fill "#333"
           :font-size 20))

(render "hello.png")
```

---

## 特性一览

| 类别 | 能力 |
|------|------|
| **图形绘制** | 圆形、矩形、椭圆、线条、多边形、路径、文字、图像 |
| **画布系统** | 多画布、Sprite 画布、分组、变换（平移/旋转/缩放） |
| **颜色与样式** | RGB/RGBA、渐变色、描边/填充、描边对齐、透明度、混合模式 |
| **风格化渲染** | 手绘风格（Rough.js 算法）、噪点填充、随机种子 |
| **图层合成** | 图层、图层组、合成画布、混合模式、投影/发光 |
| **滤镜系统** | 颜色调整、曲线、阈值、马赛克、模糊/锐化、边缘检测、卷积 |
| **Shader** | SkSL 着色器、多纹理绑定、分形噪声、域扭曲 |
| **3D 图形** | 立方体、球体、圆锥、平面、3D 变换、透视/正交相机 |
| **动画** | 时间线、补间、并行动画、序列动画、逐帧控制 |
| **精灵组件** | 角色（身体/头部/眼睛/头发/服装）、UI（按钮/面板/血条）、物品、场景装饰 |
| **调色板** | 预置调色板、自定义调色板、风格定义与引用 |
| **瓦片地图** | 正交瓦片地图、等距瓦片地图、瓦片集定义 |
| **位图资产** | 图片加载、精灵表单、位图图层、裁剪 |
| **渲染通道** | 反照率、法线、高光分离渲染 |
| **GIF 导出** | 动画序列导出为 GIF |
| **模块系统** | 文件导入、符号导出、循环依赖检测 |

---

## 示例展示



### 🏝️ 等距瓦片地图
正交与等距两种瓦片地图渲染，支持多层地图和独立瓦片设置。

<div align="center">
  <img src="examples/09-projects/09_isometric_tilemap/09_isometric_tilemap.png" width="400" alt="等距瓦片地图">
  <img src="examples/09-projects/10_iso_tiles/iso_tiles.png" width="400" alt="等距瓦片">
</div>

---

### 🎮 卡牌游戏 UI
用 PML 构建游戏界面元素：卡片、面板、装饰与文字排版。

<div align="center">
  <img src="examples/09-projects/03_card_game_ui/03_card_game_ui.png" width="500" alt="卡牌游戏 UI">
</div>

---

### 🌄 程序化生成风景
Shader 着色器 + 分形噪声 + 域扭曲，生成独特的地形景观。

<div align="center">
  <img src="examples/09-projects/04_generative_landscape/04_generative_landscape.png" width="500" alt="程序化风景">
</div>

---

### 🎨 动漫风格野外场景
多层合成、自定义 Shader、噪声纹理与等距结合的完整场景。

```scheme
; 使用 apply-shader! 为图形应用着色器
(define perlin (noise-fractal 200 200 :seed 42))
(define shader-handle (shader perlin :name "perlin"))

(define uniforms (make-uniforms "float time"))
(apply-uniforms shader-handle :time 0.5)

(add (rect 0 0 800 600 :fill "#fff"))
(apply-shader! shader-handle)
(render "wilderness.png")
```

<div align="center">
  <img src="examples/09-projects/11_anime_wilderness/anime_wilderness.png" width="600" alt="动漫场景">
</div>

---

### ✏️ 手绘风格
Rough.js 算法实现的手绘效果：抖动描边、填充纹理。

```scheme
(add (circle 100 100 50
             :fill "#FF6B6B"
             :roughness 2
             :bowing 1
             :fill-weight 3))
```

<div align="center">
  <img src="examples/11-rough-style/output/rough_stroke_basics.png" width="400" alt="手绘风格">
</div>

---

### 🎬 动画
时间线驱动的补间动画系统，支持并行动画与序列动画。

```scheme
(define a (animate circle-obj
                    :x 100 :y 100 :r 30
                    :duration 2
                    :ease "ease-in-out"))
(play a)
```

---

## 快速开始

### 构建

```bash
# 克隆
git clone https://github.com/wvfp/PML.git
cd PML

# 配置（CMake 预设）
cmake --preset debug

# 构建
cmake --build --preset debug

# 运行测试
ctest --preset debug
```

**依赖说明**：第三方库通过 `FetchContent` 自动拉取（HTTPS），无需 vcpkg。Skia 需预编译，通过 `PML_SKIA_DIR` 和 `PML_SKIA_OUT` 指定路径。

### 运行

```bash
# REPL 交互模式
./build/debug/src/pml/cli/Debug/pml.exe

# 执行文件
./build/debug/src/pml/cli/Debug/pml.exe hello.pml

# 指定输出目录
./build/debug/src/pml/cli/Debug/pml.exe hello.pml -o ./output

# 监听文件变更自动重渲染
./build/debug/src/pml/cli/Debug/pml.exe hello.pml --watch

# JSON 输出
./build/debug/src/pml/cli/Debug/pml.exe hello.pml --json
```

### 作为 VSCode 扩展使用

安装 `pml-vscode` 扩展后，打开 `.pml` 文件即可实时预览。编辑导入的依赖文件会自动触发主文件重渲染。

---

## 项目结构

```
PML/
├── src/pml/           # C++23 核心源码
│   ├── core/          # 值系统、错误处理
│   ├── frontend/      # 词法、语法、宏展开
│   ├── evaluator/     # 求值器与内置函数
│   ├── graphics/      # 图形对象与变换
│   ├── graphics3d/    # 3D 图形
│   ├── backend/       # 渲染后端（Skia、GIF）
│   ├── animation/     # 动画时间线
│   ├── sprites/       # 精灵组件与样式
│   ├── skeleton/      # 骨骼与 IK
│   ├── layer/         # 图层合成
│   ├── filter/        # 滤镜
│   ├── shader/        # SkSL 着色器
│   ├── asset/         # 位图 I/O
│   └── api/           # PMLRuntime 统一接口
├── examples/          # 示例脚本与输出
├── docs/assets/       # 文档资源
├── tests/             # 单元测试（630+ 用例）
└── pml-vscode/        # VSCode 扩展
```

---

## 许可证

MIT © PML Contributors

---

<div align="center">
  <sub>使用 PML 自身渲染的 Logo | <a href="docs/assets/logo.pml">查看源码</a></sub>
</div>

