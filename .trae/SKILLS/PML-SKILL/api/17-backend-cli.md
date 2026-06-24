# 17 — Backend / CLI

## Backend 管理

PML 支持多个渲染后端，通过 registry 注册和选择。

| 函数 | 签名 | 说明 |
|------|------|------|
| `list-backends` | `(list-backends)` | 列出所有可用后端名称 |
| `set-backend!` | `(set-backend! name)` | 设置活动后端 |
| `current-backend` | `(current-backend)` | 获取当前后端名称 |
| `backend-available?` | `(backend-available? name)` | 检查后端是否可用 |
| `backend-capabilities` | `(backend-capabilities)` | 查询当前后端能力位掩码 |

```scheme
(list-backends)          → ("skia" "gif" "null")
(set-backend! "skia")    ; 切换到 Skia 后端
(current-backend)        → "skia"
```

### 后端能力 (BackendCap)

`backend-capabilities` 返回整数位掩码，每个位表示一种能力：

| 位 | 能力 | 值 | 说明 |
|----|------|----|------|
| 0 | Shader | `0x01` | 支持 SkSL 着色器 |
| 1 | Animation | `0x02` | 支持 GIF 动画 |
| 2 | 3D | `0x04` | 支持 3D 渲染 |
| 3 | Tilemap | `0x08` | 支持 Tilemap 渲染 |
| 4 | RenderChannels | `0x10` | 支持多通道导出 |

```scheme
(define caps (backend-capabilities))
; 检查是否支持着色器
(if (not (= (bitwise-and caps 1) 0))
    (println "Shader supported"))
```

### 可用后端

| 后端 | 能力 | 说明 |
|------|------|------|
| `"skia"` | 全部 | GPU 渲染，优先选择 |
| `"gif"` | 动画 | GIF 导出，CPU 渲染 |
| `"null"` | 无 | 测试用，所有操作返回错误 |

---

## CLI 选项

### pml.exe

基本使用：

```powershell
pml.exe file.pml                       # 执行文件
pml.exe file.pml --json -o .\out       # JSON 结果输出到目录
pml.exe file.pml --watch               # 监听文件变化自动重渲染
pml.exe                                # REPL 模式
pml.exe file.pml -o output_dir         # 指定输出目录
```

完整选项：

| 标志 | 说明 |
|------|------|
| `file.pml` | 要执行的 PML 文件 |
| `--json` | 以 JSON 格式输出结果 |
| `-o, --output-dir DIR` | 指定输出目录 |
| `--watch` | 监听文件变化自动重渲染 |
| `-h, --help` | 显示帮助 |

### pml-mcp.exe

MCP JSON-RPC 服务：

```powershell
pml-mcp.exe    # 通过 stdio 启动 MCP 服务
```

与 OpenCode 等 AI 工具集成，提供 PML 代码执行和渲染能力。

---

## 编译时选项

| CMake 选项 | 默认 | 说明 |
|------------|------|------|
| `PML_BUILD_TESTS` | ON | GTest + smoke tests |
| `PML_BUILD_CLI` | ON | pml.exe CLI |
| `PML_BUILD_MCP` | ON | pml-mcp.exe MCP 服务 |
| `PML_BUILD_GIF` | ON | GIF 导出后端 |
| `PML_BUILD_CAIRO` | OFF | Cairo 后端（Windows MSVC 不稳定） |
| `PML_BUILD_SKIA` | ON | Skia GPU 后端 |
| `PML_BUILD_FREETYPE` | OFF | FreeType 字体 |

```powershell
cmake --preset debug -DPML_BUILD_CAIRO=ON -DPML_BUILD_GIF=OFF
cmake --build --preset debug
```

---

## 测试

```powershell
# 运行所有测试
.\build\debug\tests\Debug\pml-builtins-smoke.exe

# 预期 272/272 测试通过（持续增长中）
# 测试覆盖：core builtins, graphics, animation, tilemap, render-channels, shaders
```
