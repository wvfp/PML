# PML VSCode 插件审阅

## 整体评价

插件质量很高，代码结构清晰、注释完整、TypeScript 严格模式、错误处理周全。以下是发现的几个问题。

---

## 1. 二进制捆绑问题

**文件**：`pml-vscode/pml-bin/pml.exe` + `MSVCP140.dll` 等

**问题**：捆绑了 3 个 VC++ 运行时 DLL（~3MB），但仍需用户机器安装 VC++ 运行时。缺少 `ucrtbase.dll` 等系统组件时仍会失败。

**建议**：
- 将 pml.exe 改为静态链接（`/MT`），完全消除 DLL 依赖
- 或增加 post-install 脚本检测运行时

## 2. `_importRegex` 不支持带 `:prefix` 的 import

**文件**：`previewProvider.ts:493`

```typescript
private static _importRegex = /\(import\s+"([^"]+)"\)/g;
```

**影响**：`(import "tools" :prefix t)` 不会被匹配，当 `tools.pml` 变化时不会触发重新渲染。

**修复**：改为 `/\(import\s+"([^"]+)"(?:\s+[^)]*)?\)/g`

## 3. `resolveBinaryPath` 硬编码 `pml.exe`

**文件**：`executor.ts:120-122`

**问题**：硬编码了 `pml.exe`，Linux/macOS 应该是 `pml`。

**影响**：插件在非 Windows 平台完全不可用。

**修复**：
```typescript
const binaryName = process.platform === 'win32' ? 'pml.exe' : 'pml';
```

## 4. `pml-bin` 中的二进制不应提交到 git

**文件**：`pml-vscode/pml-bin/pml.exe` + DLL（~8MB）

**建议**：加入 `.gitignore`，改为 postinstall 从 GitHub Releases 下载。

## 5. `selectMainFile` 启发式脆弱

**文件**：`executor.ts:103-109`

**问题**：`return files[files.length - 1]` 对 `render-set` 多输出场景不准确。

**建议**：选择第一个不带 `@2x`/`@4x` 的文件。

## 6. 语法高亮缺失纹理函数

**文件**：`syntaxes/pml.tmLanguage.json`

**缺失**：`define-texture`、`texture-map`、`texture?`、`texture-wrap!`、`texture-filter!`、`perturb-polygon` 等。

## 7. 缩进规则过于宽泛

**文件**：`language-configuration.json:36`

```json
"increaseIndentPattern": "^\\([^)]*$"
```

**问题**：匹配注释中的 `(`、字符串中的 `(` 等。

## 8. 临时目录堆积

**文件**：`previewProvider.ts:147`

**影响**：VS Code 崩溃时 `%TEMP%/pml-preview/` 不会清理。

**建议**：启动时清理超过 24 小时的旧目录。

## 优先级

| 优先级 | 问题 |
|--------|------|
| 🔴 高 | 2（import 正则）、3（仅 Windows） |
| 🟡 中 | 4（二进制提交到 git）、5（主文件选择） |
| 🟢 低 | 6（语法高亮）、7（缩进）、8（临时目录） |
