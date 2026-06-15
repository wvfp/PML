import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { execSync, exec } from 'child_process';

/**
 * Webview panel that renders PML code to an image in real time.
 *
 * Workflow:
 *   1. User edits a .pml file
 *   2. Extension sends the source to a PML render process
 *   3. PML outputs PNG → webview displays it
 */
export class PreviewPanel {
  public static readonly viewType = 'pml.preview';

  private _panel: vscode.WebviewPanel | undefined;
  private _disposables: vscode.Disposable[] = [];
  private _currentDoc: vscode.TextDocument | undefined;
  private _onDispose: (() => void) | undefined;

  constructor(private readonly _extensionUri: vscode.Uri) {}

  /** Register a callback for when the panel is closed. */
  onDispose(cb: () => void) {
    this._onDispose = cb;
  }

  /** Show the preview panel for a given document. */
  show(document: vscode.TextDocument) {
    this._currentDoc = document;

    if (this._panel) {
      this._panel.reveal(vscode.ViewColumn.Beside);
    } else {
      this._panel = vscode.window.createWebviewPanel(
        PreviewPanel.viewType,
        'PML Preview',
        vscode.ViewColumn.Beside,
        {
          enableScripts: true,
          retainContextWhenHidden: true,
          localResourceRoots: [
            vscode.Uri.joinPath(this._extensionUri, 'media'),
          ],
        },
      );

      this._panel.onDidDispose(() => {
        this._panel = undefined;
        if (this._onDispose) this._onDispose();
      }, null, this._disposables);
    }

    this.render(document);
  }

  /** Render the current document to an image and display it. */
  async render(document: vscode.TextDocument) {
    if (!this._panel) return;

    this._currentDoc = document;
    const source = document.getText();
    const config = vscode.workspace.getConfiguration('pml');
    const bgColor = vscode.workspace.getConfiguration('pml.preview')
      .get<string>('backgroundColor', '#1a1a2e');

    // Get the PML CLI path (from environment or default)
    const pmlCmd = this._findPmlCommand();
    if (!pmlCmd) {
      this._showError('PML not found. Install with: pip install pml-markup');
      return;
    }

    // Create a temp file for the PML source
    const tmpDir = fs.mkdtempSync(path.join(
      require('os').tmpdir(), 'pml-preview-'
    ));
    const tmpFile = path.join(tmpDir, 'preview.pml');
    const tmpOutput = path.join(tmpDir, 'preview.png');

    // Wrap the source to output to our temp file
    const wrappedSource = source + `\n(render "${tmpOutput.replace(/\\/g, '/')}")`;
    fs.writeFileSync(tmpFile, wrappedSource, 'utf-8');

    try {
      // Execute PML
      execSync(`"${pmlCmd}" "${tmpFile}"`, {
        cwd: tmpDir,
        timeout: 15000,
        stdio: 'pipe',
        windowsHide: true,
      });

      // Read the rendered image as base64
      if (fs.existsSync(tmpOutput)) {
        const imageBuffer = fs.readFileSync(tmpOutput);
        const base64 = imageBuffer.toString('base64');
        const mime = tmpOutput.endsWith('.gif') ? 'image/gif' : 'image/png';

        this._panel.webview.html = this._buildHtml(base64, mime, bgColor);
      } else {
        this._showError('PML executed but no image was produced. Make sure to include (render ...) in your code.');
      }
    } catch (err: any) {
      const stderr = err.stderr?.toString() || err.message || 'Unknown error';
      this._showError(`Render failed:\n${stderr}`);
    } finally {
      // Cleanup temp files
      try {
        fs.rmSync(tmpDir, { recursive: true, force: true });
      } catch {}
    }
  }

  /** Save the current rendered image to a location chosen by the user. */
  async saveAs() {
    if (!this._panel) return;
    // Send a message to the webview to get the image data
    this._panel.webview.postMessage({ command: 'getImage' });
  }

  // ── Private helpers ──────────────────────────────────────────

  private _findPmlCommand(): string | null {
    // Check custom path from settings
    const customPath = vscode.workspace.getConfiguration('pml.lsp')
      .get<string>('serverPath', '');
    if (customPath && fs.existsSync(customPath)) return customPath;

    // Try common names
    const candidates = ['pml', 'pml-markup'];
    for (const cmd of candidates) {
      try {
        execSync(`${process.platform === 'win32' ? 'where' : 'which'} ${cmd}`, {
          stdio: 'pipe',
          windowsHide: true,
        });
        return cmd;
      } catch {}
    }

    // Try uv run pml
    try {
      execSync('uv run pml --help', { stdio: 'pipe', windowsHide: true });
      return 'uv run pml';
    } catch {}

    return null;
  }

  private _buildHtml(imageBase64: string, mime: string, bgColor: string): string {
    return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>PML Preview</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      background: ${bgColor === 'transparent' ? '#1a1a2e' : bgColor};
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    }
    .container {
      background: ${bgColor === 'transparent' ? 'transparent' : bgColor};
      padding: 16px;
      border-radius: 8px;
      max-width: 100%;
    }
    img {
      max-width: 100%;
      height: auto;
      display: block;
      image-rendering: pixelated;
      box-shadow: 0 4px 24px rgba(0,0,0,.3);
      border-radius: 4px;
    }
    .info {
      margin-top: 12px;
      color: rgba(255,255,255,.6);
      font-size: 12px;
      text-align: center;
    }
    .error {
      color: #e74c3c;
      background: rgba(231,76,60,.1);
      padding: 16px;
      border-radius: 8px;
      white-space: pre-wrap;
      font-family: 'Cascadia Code', 'Fira Code', monospace;
      font-size: 13px;
      max-width: 90vw;
    }
  </style>
</head>
<body>
  <div class="container">
    <img src="data:${mime};base64,${imageBase64}" alt="PML Render" />
    <div class="info">PML Preview — auto-refreshes on save</div>
  </div>
  <script>
    const vscode = acquireVsCodeApi();
    window.addEventListener('message', event => {
      if (event.data.command === 'getImage') {
        const img = document.querySelector('img');
        if (img) vscode.postMessage({ command: 'saveImage', data: img.src });
      }
    });
  </script>
</body>
</html>`;
  }

  private _showError(message: string) {
    if (!this._panel) return;
    const bgColor = vscode.workspace.getConfiguration('pml.preview')
      .get<string>('backgroundColor', '#1a1a2e');
    this._panel.webview.html = `<!DOCTYPE html>
<html><head><meta charset="UTF-8">
<style>
  body {
    background: ${bgColor}; color: #e74c3c;
    font-family: monospace; padding: 20px;
    white-space: pre-wrap; font-size: 13px;
  }
</style></head>
<body><div class="error">${message.replace(/</g, '&lt;')}</div></body></html>`;
  }
}
