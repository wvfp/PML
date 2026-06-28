# PML VSCode 预览体验增强 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 pml-vscode 扩展实现 7 项预览体验增强，覆盖 GIF 预览、缩放平移、代码补全、Live/Manual 渲染切换、输出日志面板、可配置 pml.exe 路径、渲染历史快照对比

**Architecture:** 所有改动在 `pml-vscode/` 目录内完成，不涉及 C++ 核心代码。每项增强独立可测。TypeScript 编译目标 ES2021/commonjs。新版 pml.exe 从 build 目录复制到 `pml-bin/` 后打包。

**Tech Stack:** TypeScript, VSCode Extension API (CustomEditor, WebView, CompletionItemProvider, OutputChannel, StatusBarItem), WebView (vanilla JS, CSS)

## Global Constraints

- `.vscodeignore` 过滤了 `src/` 和 `.ts` 文件——编译后的 `.js` 在 `out/` 中，打包前必须 `tsc`
- 所有 Phase 完成后 `npx tsc -p tsconfig.json` 无错误
- 打包前复制最新 `pml.exe`：`cp build/debug/src/pml/cli/Debug/pml.exe pml-vscode/pml-bin/pml.exe`
- 打包：`cd pml-vscode && npx vsce package` — 无 warning（已知 warning: 无 LICENSE 文件除外）
- 遵循现有代码风格：2-space indent, TypeScript strict mode

---

## 文件结构

| 文件 | 归属 | 职责 |
|------|------|------|
| `src/previewProvider.ts` | Phases 1,4,5,7 | 预览渲染核心逻辑 |
| `src/executor.ts` | Phase 6 | CLI 二进制路径解析 |
| `src/extension.ts` | Phases 3,4,5 | 扩展激活入口 |
| `src/completionProvider.ts` | Phase 3 | 代码补全提供者 |
| `src/outputChannel.ts` | Phase 5 | PML 输出日志通道 |
| `media/preview.html` | Phases 2,7 | WebView 前端 UI |
| `package.json` | Phases 4,6 | 配置项与命令注册 |

## 数据流

```
编辑器文本 → onDidChange/Save → _render() → executePML() → pml.exe (--json)
                                                      ↓
                                           JSON 解析 → _buildFileUris() → postMessage → WebView
                                                                                          ↓
                                                                                   缩放/平移/历史 (纯前端)
```

---

## Phase 1: GIF 动画预览

**Files:**
- Modify: `src/previewProvider.ts:419-451` — `_buildFileUris()` MIME type 选择

**Interfaces:**
- Consumes: `result.files: string[]` (文件路径列表)
- Produces: `dataUri: string` (带正确 MIME 的 base64 URI)

- [ ] **Step 1: 修改 MIME type 逻辑**

在 `_buildFileUris()` 的数据 URI 构建处：

```ts
// previewProvider.ts, _buildFileUris() 方法内
// 改前:
dataUri = `data:image/png;base64,${buf.toString('base64')}`;

// 改后:
const ext = path.extname(file).toLowerCase();
const mime = ext === '.gif' ? 'image/gif' : 'image/png';
dataUri = `data:${mime};base64,${buf.toString('base64')}`;
```

- [ ] **Step 2: 编译验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误，`out/previewProvider.js` 更新

- [ ] **Step 3: 打包验证**

```bash
cp "G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe" "G:/Project/PML/pml-vscode/pml-bin/pml.exe"
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx vsce package
```

Expected: vsix 生成成功

- [ ] **Step 4: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): detect GIF MIME type for animated preview"
```

---

## Phase 2: 图片缩放与平移

**Files:**
- Modify: `media/preview.html` — 新增缩放/平移 UI + JS 逻辑

**Interfaces:**
- Consumes: WebView 中的 `<img id="preview-main-img">`
- Produces: 鼠标交互（滚轮缩放、拖拽平移）、UI 控件（适应宽度、重置）

- [ ] **Step 1: 在 preview.html 中新增缩放容器结构**

在 `<div id="app">` 的渲染结果外包裹缩放容器：

```html
<!-- media/preview.html, 在 <div id="app"> 内部 -->
<div id="zoom-container" style="overflow:hidden;position:relative;width:100%;height:100%;cursor:grab;">
  <div id="zoom-target" style="transform-origin:0 0;transition:none;">
    <!-- 渲染内容会被移入这里 -->
  </div>
</div>
```

修改 `handleRender()` 函数，把渲染 HTML 插入 `#zoom-target` 而非 `#app`。

添加 CSS：

```css
/* 缩放控件容器 */
.zoom-controls {
  position: fixed;
  bottom: 16px;
  right: 16px;
  display: flex;
  gap: 4px;
  background: var(--vscode-sideBar-background, #252526);
  border: 1px solid var(--vscode-widget-border, #333);
  border-radius: 6px;
  padding: 4px;
  z-index: 100;
  opacity: 0.7;
  transition: opacity 0.2s;
}
.zoom-controls:hover { opacity: 1; }
.zoom-controls button {
  background: transparent;
  border: none;
  color: var(--vscode-foreground, #ccc);
  cursor: pointer;
  padding: 4px 8px;
  font-size: 13px;
  border-radius: 3px;
}
.zoom-controls button:hover {
  background: var(--vscode-toolbar-hoverBackground, #333);
}
.zoom-controls .zoom-label {
  padding: 4px 6px;
  font-size: 11px;
  min-width: 40px;
  text-align: center;
  color: var(--vscode-descriptionForeground, #888);
}
```

- [ ] **Step 2: 添加缩放/平移 JS 逻辑**

在 `</script>` 前追加：

```js
// ── Zoom & Pan ────────────────────────────────────────────
var zoomState = { scale: 1, panX: 0, panY: 0 };
var isPanning = false, panStart = { x: 0, y: 0 };
var ZOOM_MIN = 0.1, ZOOM_MAX = 10;

function applyTransform() {
  var target = document.getElementById('zoom-target');
  if (!target) return;
  target.style.transform = 'translate(' + zoomState.panX + 'px,' + zoomState.panY + 'px) scale(' + zoomState.scale + ')';
  updateZoomLabel();
}

function updateZoomLabel() {
  var label = document.querySelector('.zoom-label');
  if (label) label.textContent = Math.round(zoomState.scale * 100) + '%';
}

// 滚轮缩放（以鼠标位置为中心）
document.getElementById('zoom-container').addEventListener('wheel', function (e) {
  if (e.ctrlKey || e.metaKey) {
    e.preventDefault();
    var rect = this.getBoundingClientRect();
    var mx = e.clientX - rect.left, my = e.clientY - rect.top;
    var delta = -e.deltaY * 0.001;
    var newScale = Math.min(ZOOM_MAX, Math.max(ZOOM_MIN, zoomState.scale * (1 + delta)));
    var ratio = newScale / zoomState.scale;
    zoomState.panX = mx - (mx - zoomState.panX) * ratio;
    zoomState.panY = my - (my - zoomState.panY) * ratio;
    zoomState.scale = newScale;
    applyTransform();
  }
}, { passive: true });

// 拖拽平移
document.getElementById('zoom-container').addEventListener('mousedown', function (e) {
  if (e.button === 0) {
    isPanning = true;
    panStart.x = e.clientX - zoomState.panX;
    panStart.y = e.clientY - zoomState.panY;
    this.style.cursor = 'grabbing';
  }
});
document.addEventListener('mousemove', function (e) {
  if (isPanning) {
    zoomState.panX = e.clientX - panStart.x;
    zoomState.panY = e.clientY - panStart.y;
    applyTransform();
  }
});
document.addEventListener('mouseup', function () {
  if (isPanning) {
    isPanning = false;
    var c = document.getElementById('zoom-container');
    if (c) c.style.cursor = 'grab';
  }
});

// 适应宽度
function zoomToFit() {
  var img = document.getElementById('preview-main-img');
  if (!img) return;
  var container = document.getElementById('zoom-container');
  if (!container) return;
  var cw = container.clientWidth - 40;
  var iw = img.naturalWidth || img.width;
  if (iw === 0) return;
  zoomState.scale = Math.min(cw / iw, 1);
  zoomState.panX = 0;
  zoomState.panY = 0;
  applyTransform();
}

// 重置
function zoomReset() {
  zoomState.scale = 1; zoomState.panX = 0; zoomState.panY = 0;
  applyTransform();
}
```

- [ ] **Step 3: 在渲染结果中添加缩放控件**

修改 `buildRenderHtml()`，在返回值前追加缩放控件 HTML：

```js
html += '<div class="zoom-controls">' +
  '<button onclick="zoomToFit()" title="适应宽度">⊞</button>' +
  '<button onclick="zoomReset()" title="重置 (Ctrl+0)">⟲</button>' +
  '<button onclick="zoomState.scale=Math.min(ZOOM_MAX,zoomState.scale*1.2);applyTransform()" title="放大">＋</button>' +
  '<button onclick="zoomState.scale=Math.max(ZOOM_MIN,zoomState.scale/1.2);applyTransform()" title="缩小">−</button>' +
  '<span class="zoom-label">100%</span>' +
  '</div>';
```

- [ ] **Step 4: 编译 + 打包验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误

- [ ] **Step 5: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add zoom and pan controls to preview"
```

---

## Phase 3: PML 代码补全

**Files:**
- Create: `src/completionProvider.ts`
- Modify: `src/extension.ts` — 在 activate() 中注册 provider

**Interfaces:**
- Consumes: VSCode 编辑事件（输入 `:` 或普通字符时触发）
- Produces: `CompletionItem[]` 列表

- [ ] **Step 1: 创建 `src/completionProvider.ts`**

```ts
import * as vscode from 'vscode';

/**
 * 内置函数列表：名称 + 签名 + 说明
 */
interface PMLBuiltin {
  label: string;
  detail: string;
  documentation: string;
  kind: vscode.CompletionItemKind;
}

const BUILTINS: PMLBuiltin[] = [
  // ── 形状 ──
  { label: 'circle', detail: '(circle cx cy r [:fill] [:stroke] [:stroke-width])', documentation: '创建圆形 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'rect', detail: '(rect x y w h [:rx] [:fill] [:stroke])', documentation: '创建矩形 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'ellipse', detail: '(ellipse cx cy rx ry [:fill] [:stroke])', documentation: '创建椭圆 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'line', detail: '(line x1 y1 x2 y2 [:stroke] [:stroke-width])', documentation: '创建线条 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'polygon', detail: '(polygon points [:fill] [:stroke] [:edge-noise])', documentation: '创建多边形 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'text', detail: '(text x y content [:fill] [:font-size])', documentation: '创建文字 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'path', detail: '(path [:d svg-string] [:fill] [:stroke])', documentation: '创建路径 GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'group', detail: '(group obj1 obj2 ...)', documentation: '创建分组', kind: vscode.CompletionItemKind.Function },
  // ── 画布 ──
  { label: 'canvas', detail: '(canvas w h [:bg color])', documentation: '创建画布', kind: vscode.CompletionItemKind.Function },
  { label: 'add', detail: '(add graphic-object)', documentation: '将图形添加到当前画布', kind: vscode.CompletionItemKind.Function },
  { label: 'render', detail: '(render "filename.png")', documentation: '渲染输出到文件', kind: vscode.CompletionItemKind.Function },
  { label: 'set-backend!', detail: '(set-backend! "skia")', documentation: '设置渲染后端', kind: vscode.CompletionItemKind.Function },
  // ── 样式 ──
  { label: 'with-transform', detail: '(with-transform obj :transform t)', documentation: '应用变换', kind: vscode.CompletionItemKind.Function },
  { label: 'apply-shader!', detail: '(apply-shader! handle)', documentation: '应用着色器', kind: vscode.CompletionItemKind.Function },
  // ── 编程 ──
  { label: 'define', detail: '(define name value)', documentation: '定义变量', kind: vscode.CompletionItemKind.Keyword },
  { label: 'let', detail: '(let ((var val) ...) body)', documentation: '顺序绑定', kind: vscode.CompletionItemKind.Keyword },
  { label: 'lambda', detail: '(lambda (params) body)', documentation: '创建匿名函数', kind: vscode.CompletionItemKind.Keyword },
  { label: 'if', detail: '(if cond then-expr else-expr)', documentation: '条件分支', kind: vscode.CompletionItemKind.Keyword },
  { label: 'import', detail: '(import "filename.pml")', documentation: '导入模块', kind: vscode.CompletionItemKind.Keyword },
  { label: 'defn', detail: '(defn name (params) body)', documentation: '定义函数', kind: vscode.CompletionItemKind.Keyword },
  // ── Lisp ──
  { label: 'first', detail: '(first lst)', documentation: '取列表第一个元素', kind: vscode.CompletionItemKind.Function },
  { label: 'rest', detail: '(rest lst)', documentation: '取列表剩余元素', kind: vscode.CompletionItemKind.Function },
  { label: 'cons', detail: '(cons val lst)', documentation: '构造新列表', kind: vscode.CompletionItemKind.Function },
  { label: 'list', detail: '(list ...)', documentation: '创建列表', kind: vscode.CompletionItemKind.Function },
  { label: 'map', detail: '(map fn lst)', documentation: '映射', kind: vscode.CompletionItemKind.Function },
  { label: 'filter', detail: '(filter pred lst)', documentation: '过滤', kind: vscode.CompletionItemKind.Function },
  { label: 'apply', detail: '(apply fn args)', documentation: '调用函数', kind: vscode.CompletionItemKind.Function },
];

const KEYWORD_ARGS: vscode.CompletionItem[] = [
  ':fill', ':stroke', ':stroke-width', ':opacity', ':op',
  ':bg', ':background', ':rx', ':font-size', ':font-name',
  ':blend-mode', ':roughness', ':bowing', ':seed', ':style',
  ':stroke-align', ':edge-noise', ':edge-seed', ':edge-subdiv',
  ':corner-radius', ':d', ':align', ':transform',
].map(kw => {
  const item = new vscode.CompletionItem(kw, vscode.CompletionItemKind.Property);
  item.detail = 'PML keyword argument';
  return item;
});

export function registerCompletionProvider(context: vscode.ExtensionContext): void {
  // Provider 1: 通用函数名补全
  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider('pml', {
      provideCompletionItems(): vscode.CompletionItem[] {
        return BUILTINS.map(b => {
          const item = new vscode.CompletionItem(b.label, b.kind);
          item.detail = b.detail;
          item.documentation = b.documentation;
          return item;
        });
      },
    }),
  );

  // Provider 2: 关键字参数补全（触发字符 ':')
  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider('pml', {
      provideCompletionItems(): vscode.CompletionItem[] {
        return KEYWORD_ARGS;
      },
    }, ':'),
  );
}
```

- [ ] **Step 2: 在 `extension.ts` 中注册**

在 `extension.ts` 的 `activate()` 末尾，添加：

```ts
import { registerCompletionProvider } from './completionProvider';

// 在 activate() 内，statusBarItem.show() 之后：
registerCompletionProvider(context);
```

- [ ] **Step 3: 编译验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误，`out/completionProvider.js` 生成

- [ ] **Step 4: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add PML code completion provider"
```

---

## Phase 4: Live/Manual 渲染模式切换

**Files:**
- Modify: `package.json` — 新增命令和配置项
- Modify: `src/extension.ts` — 新增状态栏切换按钮和命令
- Modify: `src/previewProvider.ts` — 根据模式决定是否触发渲染

**Interfaces:**
- Consumes: `pml.preview.autoRender` 配置值
- Produces: 状态栏图标切换、`onDidChangeTextDocument` 中跳过渲染

- [ ] **Step 1: 在 `package.json` 新增配置项**

```json
// package.json, contributes 中添加
"configuration": {
  "title": "PML",
  "properties": {
    "pml.preview.autoRender": {
      "type": "string",
      "enum": ["live", "manual"],
      "default": "live",
      "description": "PML 预览渲染模式：live（按键即渲）或 manual（仅保存时渲染）"
    }
  }
}
```

- [ ] **Step 2: 在 `extension.ts` 中新增状态栏切换按钮和命令**

```ts
// 在 activate() 中，现有的 statusBarItem 之后添加：
const liveToggle = vscode.window.createStatusBarItem(
  vscode.StatusBarAlignment.Right, 99,
);

function updateLiveToggle() {
  const mode = vscode.workspace.getConfiguration('pml').get('preview.autoRender');
  if (mode === 'manual') {
    liveToggle.text = '$(debug-pause) Manual';
    liveToggle.tooltip = '仅保存时渲染 | 点击切换为 Live';
  } else {
    liveToggle.text = '$(sync) Live';
    liveToggle.tooltip = '按键即渲（500ms 防抖） | 点击切换为 Manual';
  }
}

updateLiveToggle();
liveToggle.command = 'pml.toggleAutoRender';
liveToggle.show();
context.subscriptions.push(liveToggle);

context.subscriptions.push(
  vscode.commands.registerCommand('pml.toggleAutoRender', () => {
    const current = vscode.workspace.getConfiguration('pml').get('preview.autoRender');
    const next = current === 'manual' ? 'live' : 'manual';
    vscode.workspace.getConfiguration('pml').update('preview.autoRender', next, true);
    updateLiveToggle();
    vscode.window.showInformationMessage(`PML preview mode: ${next}`);
  }),
);
```

- [ ] **Step 3: 在 `previewProvider.ts` 中限制渲染**

在 `_debouncedRender` 的计时器回调中（或在 `onDidChangeTextDocument` handler 入口）添加：

```ts
// previewProvider.ts, resolveCustomEditor 中 onDidChangeTextDocument handler 内
if (changedKey === fileKey) {
  const mode = vscode.workspace.getConfiguration('pml').get('preview.autoRender');
  if (mode !== 'manual') {
    this._debouncedRender(document, webviewPanel, tempDir);
  }
  return;
}
```

- [ ] **Step 4: 编译验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误

- [ ] **Step 5: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add live/manual render mode toggle"
```

---

## Phase 5: 输出日志面板

**Files:**
- Create: `src/outputChannel.ts`
- Modify: `src/extension.ts` — 创建 OutputChannel + 注册显示命令
- Modify: `src/previewProvider.ts` — `_render()` 中写入日志

**Interfaces:**
- Consumes: 渲染开始/结束/错误事件
- Produces: `vscode.OutputChannel` 内容

- [ ] **Step 1: 创建 `src/outputChannel.ts`**

```ts
import * as vscode from 'vscode';

let _channel: vscode.OutputChannel | undefined;

export function initOutputChannel(): vscode.OutputChannel {
  _channel = vscode.window.createOutputChannel('PML Render');
  return _channel;
}

export function getOutputChannel(): vscode.OutputChannel | undefined {
  return _channel;
}

export function logRender(filePath: string, binaryPath: string, cwd: string,
                          success: boolean, elapsed: number,
                          files: string[], error?: { type: string; message: string; hint?: string } | null): void {
  if (!_channel) return;
  const ts = new Date().toISOString().replace('T', ' ').substring(0, 19);
  _channel.appendLine(`[${ts}] Render ${success ? 'OK' : 'FAIL'} ${filePath}`);
  _channel.appendLine(`  binary: ${binaryPath}`);
  _channel.appendLine(`  cwd: ${cwd}`);
  _channel.appendLine(`  elapsed: ${elapsed}ms`);
  if (files.length > 0) {
    _channel.appendLine(`  files: ${files.join(', ')}`);
  }
  if (error) {
    _channel.appendLine(`  error: ${error.type} — ${error.message}`);
    if (error.hint) _channel.appendLine(`  hint: ${error.hint}`);
  }
  _channel.appendLine('');
}
```

- [ ] **Step 2: 在 `extension.ts` 中初始化**

```ts
// extension.ts 顶部
import { initOutputChannel } from './outputChannel';

// activate() 中，diagnostics 初始化之后：
initOutputChannel();

// 新增显示输出面板的命令
context.subscriptions.push(
  vscode.commands.registerCommand('pml.showOutput', () => {
    const { getOutputChannel } = require('./outputChannel');
    const ch = getOutputChannel();
    if (ch) ch.show();
  }),
);
```

- [ ] **Step 3: 在 `previewProvider.ts` 的 `_render()` 中写入日志**

```ts
// previewProvider.ts 顶部
import { logRender } from './outputChannel';

// _render() 中，executePML 调用之后，postMessage 之前：
logRender(
  document.uri.fsPath,
  binaryPath,
  sourceDir,
  result.success,
  elapsed,
  result.files,
  result.error,
);
```

- [ ] **Step 4: 编译验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误

- [ ] **Step 5: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add PML render output channel"
```

---

## Phase 6: 可配置 pml.exe 路径

**Files:**
- Modify: `package.json` — 新增 `pml.binaryPath` 配置项
- Modify: `src/executor.ts:125-128` — `resolveBinaryPath()` 优先使用配置

**Interfaces:**
- Consumes: `pml.binaryPath` 配置字符串
- Produces: 解析后的二进制绝对路径

- [ ] **Step 1: 在 `package.json` 添加配置**

```json
// package.json, configuration.properties 中添加
"pml.binaryPath": {
  "type": "string",
  "default": "",
  "description": "自定义 pml.exe 路径。留空使用插件自带的 pml-bin/pml.exe。修改后需重新加载窗口。"
}
```

- [ ] **Step 2: 修改 `executor.ts`**

```ts
// executor.ts, resolveBinaryPath 函数
export function resolveBinaryPath(context: vscode.ExtensionContext): string {
  const config = vscode.workspace.getConfiguration('pml');
  const customPath = config.get<string>('binaryPath');
  if (customPath) {
    const trimmed = customPath.trim();
    if (trimmed && fs.existsSync(trimmed)) {
      return trimmed;
    }
    console.warn(`[PML] Custom binary path not found: "${trimmed}", falling back to bundled`);
  }
  const binaryName = process.platform === 'win32' ? 'pml.exe' : 'pml';
  return context.asAbsolutePath(path.join('pml-bin', binaryName));
}
```

- [ ] **Step 3: 编译验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误

- [ ] **Step 4: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add configurable pml.exe binary path"
```

---

## Phase 7: 渲染历史快照对比

**Files:**
- Modify: `media/preview.html` — WebView 端实现历史管理 + 导航 UI

**Interfaces:**
- Consumes: postMessage 的 `{ command: 'render', files }` 消息
- Produces: 历史导航 UI（⇦ ⇨ 按钮 + 计数 + 时间戳）

- [ ] **Step 1: 添加历史管理 JS 状态**

在 WebView JS 中追加：

```js
// ── Render History ────────────────────────────────────────
var renderHistory = [];
var historyIndex = -1;
var MAX_HISTORY = 5;

function pushHistory(files) {
  renderHistory.push({
    files: files,
    timestamp: new Date().toLocaleTimeString(),
  });
  if (renderHistory.length > MAX_HISTORY) renderHistory.shift();
  historyIndex = renderHistory.length - 1;
  updateHistoryUI();
}

function navigateHistory(delta) {
  var newIndex = historyIndex + delta;
  if (newIndex < 0 || newIndex >= renderHistory.length) return;
  historyIndex = newIndex;
  var entry = renderHistory[historyIndex];
  if (!entry) return;
  // 重新渲染历史快照的内容
  var app = document.getElementById('app');
  if (!app) return;
  app.innerHTML = '<div class="state-render">' + buildRenderHtml(entry.files) + '</div>';
  attachThumbnailClickHandlers(app);
  updateHistoryUI();
  // 重新绑定缩放
  setTimeout(function () {
    zoomReset();
    var c = document.getElementById('zoom-container');
    if (c) c.style.cursor = 'grab';
  }, 50);
}

function updateHistoryUI() {
  var container = document.getElementById('history-nav');
  if (!container) return;
  if (renderHistory.length <= 1) {
    container.style.display = 'none';
    return;
  }
  container.style.display = 'flex';
  container.innerHTML =
    '<button onclick="navigateHistory(-1)" ' + (historyIndex <= 0 ? 'disabled' : '') + '>‹</button>' +
    '<span class="history-label">' + (historyIndex + 1) + '/' + renderHistory.length + '</span>' +
    '<button onclick="navigateHistory(1)" ' + (historyIndex >= renderHistory.length - 1 ? 'disabled' : '') + '>›</button>';
}
```

- [ ] **Step 2: 添加历史导航 CSS**

```css
.history-nav {
  display: none;
  align-items: center;
  justify-content: center;
  gap: 8px;
  margin-bottom: 12px;
  padding: 4px 8px;
}
.history-nav button {
  background: var(--vscode-button-background, #0e639c);
  color: var(--vscode-button-foreground, #fff);
  border: none;
  border-radius: 4px;
  cursor: pointer;
  padding: 4px 12px;
  font-size: 16px;
  line-height: 1;
}
.history-nav button:disabled {
  opacity: 0.4;
  cursor: default;
}
.history-nav .history-label {
  font-size: 12px;
  color: var(--vscode-descriptionForeground, #888);
  min-width: 32px;
  text-align: center;
}
```

- [ ] **Step 3: 集成到 `handleRender` 和 `handleError`**

在 `handleRender()` 中，`setContent()` 之前添加历史记录：

```js
function handleRender(app, msg) {
  if (!msg.files || msg.files.length === 0) {
    setContent(app, buildEmpty());
    return;
  }
  pushHistory(msg.files);  // ← 新增
  var html = '<div class="history-nav" id="history-nav"></div>';
  html += '<div class="state-render">' + buildRenderHtml(msg.files) + '</div>';
  setContent(app, html);
  attachThumbnailClickHandlers(app);
  updateHistoryUI();  // ← 新增
  setTimeout(function () { zoomReset(); }, 50);  // 重置缩放
}
```

- [ ] **Step 4: 编译打包验证**

```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无错误

- [ ] **Step 5: 打包最终版**

```bash
cp "G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe" "G:/Project/PML/pml-vscode/pml-bin/pml.exe"
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx vsce package
```

Expected: vsix 生成成功

- [ ] **Step 6: 提交**

```bash
cd G:/Project/PML && git add pml-vscode/ && git commit -m "feat(vscode): add render history snapshot navigation"
```

---

## 验证方式

### 全局编译
```bash
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx tsc -p tsconfig.json
```

Expected: 无类型错误，`out/` 下生成全部 4 个 `.js` 文件

### 全局打包
```bash
cp "G:/Project/PML/build/debug/src/pml/cli/Debug/pml.exe" "G:/Project/PML/pml-vscode/pml-bin/pml.exe"
cd G:/Project/PML/pml-vscode && /g/Program\ Files/nodejs/npx vsce package
```

Expected: `pml-vscode-0.1.0.vsix` 生成，包含 `out/extension.js`, `out/previewProvider.js`, `out/executor.js`, `out/completionProvider.js`, `out/outputChannel.js`, `media/preview.html`, `pml-bin/pml.exe`

### 功能验证（在 Extension Development Host 中 F5 启动）
| Phase | 操作 | 预期结果 |
|-------|------|---------|
| P1 | 打开含 `(render "a.gif")` 的文件 | 预览中 GIF 播放 |
| P2 | 滚轮 + Ctrl | 缩放；拖拽平移；适应宽度按钮工作 |
| P3 | 输入 `cir` + 输入 `:` | 弹出 `circle` 补全；弹出 `:fill` 等 |
| P4 | 点状态栏切换 Manual | 按键不再触发渲染，Ctrl+S 触发 |
| P5 | 渲染后运行 "PML: Show Output" | 输出面板显示渲染详情 |
| P6 | 设置 `pml.binaryPath` | 使用自定义二进制 |
| P7 | 多次编辑渲染 | ⇦ ⇨ 导航历史快照 |

## 工作量估算

| Phase | 文件改动 | 代码行 | 时间 |
|-------|----------|--------|------|
| P1 GIF | 1 文件 | ~3 | 5min |
| P2 Zoom | 1 文件 | ~150 | 30min |
| P3 Completion | 2 文件 (1 new) | ~120 | 20min |
| P4 Toggle | 3 文件 | ~40 | 15min |
| P5 Output | 3 文件 (1 new) | ~60 | 15min |
| P6 Binary | 2 文件 | ~10 | 5min |
| P7 History | 1 文件 | ~80 | 20min |
| **合计** | **8 文件 (2 new)** | **~460** | **~1.5h** |

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-06-27-vscode-preview-enhancements.md`.** Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
