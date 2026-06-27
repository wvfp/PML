# PML Preview

VS Code extension for [PML](https://github.com/wvfp/PML) (Picture Markup Language) — a C++23 S-expression DSL for code-to-image generation.

## Features

- Side-by-side preview for `.pml` files
- Live render on keystroke or manual render on save
- Zoom and pan controls
- Render history snapshot navigation
- Configurable `pml.exe` binary path
- Syntax highlighting
- Code completion

## Requirements

- **Windows** (the bundled `pml.exe` is Windows-only)
- Visual Studio Code 1.85+
- Skia backend for rendering (included in bundled binary)

## Extension Settings

| Setting | Description |
|---------|-------------|
| `pml.preview.autoRender` | `"live"` (render on keystroke) or `"manual"` (render on save) |
| `pml.binaryPath` | Custom `pml.exe` path. Leave empty to use the bundled binary. |

## Usage

1. Open a `.pml` file
2. Press `Ctrl+Shift+P` → `PML: Open PML Preview to the Side`
3. Edit the file — the preview updates automatically (in live mode)
