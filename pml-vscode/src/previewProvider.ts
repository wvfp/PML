import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import * as crypto from 'crypto';
import * as os from 'os';
import { executePML, selectMainFile, resolveBinaryPath, RenderError, notifyErrorOnce, resetFirstErrorShown } from './executor';
import { getDiagnostics, updateStatusBar } from './extension';
import { setDiagnostics, clearDiagnostics } from './diagnostics';

// ═══════════════════════════════════════════════════════════════════════════════
// PreviewInfo — per-preview-panel state
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Per-preview-panel state stored in `_activePreviews`.
 *
 * Used by the dependency-tracking system to locate the correct document,
 * panel, and temp directory when an imported file changes and a re-render
 * of the main file needs to be triggered.
 */
interface PreviewInfo {
  readonly document: PMLDocument;
  readonly webviewPanel: vscode.WebviewPanel;
  readonly tempDir: string;
  readonly sourceDir: string;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Custom Document
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Minimal custom document for the PML preview editor.
 *
 * VSCode's `CustomReadonlyEditorProvider` expects a `CustomDocument`; this
 * lightweight implementation holds the document URI so the provider can
 * re-read and re-render on changes.
 */
class PMLDocument implements vscode.CustomDocument {
  constructor(public uri: vscode.Uri) {}

  dispose(): void {
    // Nothing to dispose — the document is a pure read-through handle.
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Preview Provider
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Custom readonly editor provider that renders `.pml` files as side-by-side
 * preview images in a VSCode WebView.
 *
 * ## Lifecycle
 *
 * 1. `openCustomDocument()`     — returns a `PMLDocument` handle.
 * 2. `resolveCustomEditor()`    — sets up the WebView HTML, subscribes to
 *                                 document changes (500 ms debounce), triggers
 *                                 the initial render, and handles cleanup.
 * 3. On each render cycle:
 *    a. Read the document text from the editor buffer.
 *    b. Write it to a temporary file under `%TEMP%/pml-preview/<hash>/`.
 *    c. Spawn `pml.exe` (via `executePML`) with `--json` output.
 *    d. On success → clear diagnostics, convert output file paths to WebView
 *       URIs, and `postMessage({ command: 'render', files })`.
 *    e. On failure → set diagnostics, `postMessage({ command: 'error', … })`.
 *
 * ## Temp file management
 *
 * A stable MD5 hash of the document's `fsPath` is used as the sub-directory
 * name so that re-opening the same file reuses (and overwrites) the same temp
 * directory. The entire directory is removed when the editor panel is closed
 * (`onDidDispose`).
 */
export class PMLPreviewProvider implements vscode.CustomReadonlyEditorProvider<PMLDocument> {
  /** Active debounce timer (500 ms). Cleared on dispose or on each new keystroke. */
  private _debounceTimer: ReturnType<typeof setTimeout> | undefined;

  /**
   * Per-file subscriptions to document change events.
   *
   * Key = lowercased `fsPath` of the source file. Each entry is a
   * composite `Disposable` that combines the `onDidChangeTextDocument`
   * and `onDidSaveTextDocument` handlers for one preview panel.
   * Disposed when that panel is closed (via `onDidDispose`).
   *
   * Using a Map (vs a single field) avoids cross-talk when multiple
   * `.pml` preview panels are open simultaneously.
   */
  private _changeSubscriptions: Map<string, vscode.Disposable> = new Map();

  // ── Dependency-tracking state ──────────────────────────────────────────────

  /**
   * All active preview panels, keyed by lowercased `fsPath` of the main file.
   * Used to locate a preview when an imported file needs to trigger a re-render.
   */
  private _activePreviews: Map<string, PreviewInfo> = new Map();

  /**
   * Dependency graph: mainFileKey → Set<importedFileKey (lowercased)>.
   *
   * Represents which files each main preview file depends on. Updated after
   * every render by re-scanning the editor buffer for `(import "...")` forms.
   */
  private _dependencyGraph: Map<string, Set<string>> = new Map();

  /**
   * Reverse dependency index: importedFileKey → Set<mainFileKey>.
   *
   * Allows O(1) lookup of which preview(s) need to be re-rendered when a
   * given imported file changes. Updated in sync with `_dependencyGraph`.
   */
  private _reverseDeps: Map<string, Set<string>> = new Map();

  /**
   * FileSystemWatcher disposables per source directory.
   *
   * Key = lowercased source directory path. Each value is a
   * `Disposable` representing the combined watcher + change listener.
   * When no previews remain in a given sourceDir the watcher is disposed.
   *
   * Watchers catch changes to `.pml` files made *outside* the editor
   * (e.g. git checkout, external editor). In-editor changes are already
   * caught by `onDidChangeTextDocument` / `onDidSaveTextDocument`.
   */
  private _depWatchers: Map<string, vscode.Disposable> = new Map();

  constructor(private context: vscode.ExtensionContext) {
    // Clean up temp directories older than 24 hours on startup.
    PMLPreviewProvider._cleanStaleTempDirs();
  }

  // ─── CustomDocument lifecycle ──────────────────────────────────────────────

  async openCustomDocument(uri: vscode.Uri): Promise<PMLDocument> {
    return new PMLDocument(uri);
  }

  // ─── Editor resolution ─────────────────────────────────────────────────────

  async resolveCustomEditor(
    document: PMLDocument,
    webviewPanel: vscode.WebviewPanel,
  ): Promise<void> {
    // ── Stable temp directory ─────────────────────────────────────────────
    const hash = crypto.createHash('md5').update(document.uri.fsPath).digest('hex');
    const tempDir = path.join(os.tmpdir(), 'pml-preview', hash);
    fs.mkdirSync(tempDir, { recursive: true });

    // ── WebView options ───────────────────────────────────────────────────
    const localResourceRoots = [
      vscode.Uri.file(path.join(this.context.extensionPath, 'media')),
      vscode.Uri.file(path.join(os.tmpdir(), 'pml-preview')),
    ];

    webviewPanel.webview.options = {
      enableScripts: true,
      localResourceRoots,
    };

    // ── Read and populate the HTML template ───────────────────────────────
    const templatePath = this.context.asAbsolutePath(path.join('media', 'preview.html'));
    const nonce = crypto.randomBytes(16).toString('hex');

    let html: string;
    try {
      html = fs.readFileSync(templatePath, 'utf-8');
    } catch {
      html = this._getFallbackHtml();
    }

    html = html
      .replace(/\{\{nonce\}\}/g, nonce)
      .replace(/\{\{cspSource\}\}/g, webviewPanel.webview.cspSource);

    webviewPanel.webview.html = html;

    // ── Initial render ────────────────────────────────────────────────────
    this._render(document, webviewPanel, tempDir);

    // ── Subscribe to document changes (500 ms debounce) ───────────────────
    // Use fsPath (case-insensitive on Windows) for robust URI matching.
    // Also back up with onDidSaveTextDocument so Cmd+S always triggers a
    // fresh render even if onDidChangeTextDocument misses an event.
    const fileKey = document.uri.fsPath.toLowerCase();
    const sourceDir = path.dirname(document.uri.fsPath);
    const existingSub = this._changeSubscriptions.get(fileKey);
    if (existingSub) existingSub.dispose();

    const changeSub = vscode.workspace.onDidChangeTextDocument(event => {
      const changedKey = event.document.uri.fsPath.toLowerCase();
      if (changedKey === fileKey) {
        this._debouncedRender(document, webviewPanel, tempDir);
        return;
      }
      // Imported file changed — re-render this preview
      const deps = this._dependencyGraph.get(fileKey);
      if (deps && deps.has(changedKey)) {
        this._debouncedRender(document, webviewPanel, tempDir);
      }
    });
    const saveSub = vscode.workspace.onDidSaveTextDocument(doc => {
      const savedKey = doc.uri.fsPath.toLowerCase();
      if (savedKey === fileKey) {
        this._debouncedRender(document, webviewPanel, tempDir);
        return;
      }
      // Imported file saved — re-render this preview
      const deps = this._dependencyGraph.get(fileKey);
      if (deps && deps.has(savedKey)) {
        this._debouncedRender(document, webviewPanel, tempDir);
      }
    });
    this._changeSubscriptions.set(
      fileKey,
      vscode.Disposable.from(changeSub, saveSub),
    );

    // ── Register in active-previews map ───────────────────────────────────
    this._activePreviews.set(fileKey, { document, webviewPanel, tempDir, sourceDir });

    // ── Set up dependency tracking ────────────────────────────────────────
    // Read the editor buffer content (may have unsaved changes) to scan
    // for (import "...") forms and build the initial dependency graph.
    vscode.workspace.openTextDocument(document.uri).then(td => {
      const initialContent = td.getText();
      this._updateDependencyTracking(fileKey, initialContent, sourceDir);
    });

    // ── Handle messages from the WebView ──────────────────────────────────
    webviewPanel.webview.onDidReceiveMessage(msg => {
      if (msg.command === 'ready' || msg.command === 'retry') {
        this._render(document, webviewPanel, tempDir);
      }
    });

    // ── Cleanup on editor close ───────────────────────────────────────────
    webviewPanel.onDidDispose(() => {
      this._clearDebounceTimer();

      // Remove change subscription for this file
      const sub = this._changeSubscriptions.get(fileKey);
      if (sub) {
        sub.dispose();
        this._changeSubscriptions.delete(fileKey);
      }

      // Remove from active-previews map
      this._activePreviews.delete(fileKey);

      // Clean up dependency graph entries for this file
      this._dependencyGraph.delete(fileKey);
      for (const [, mainKeys] of this._reverseDeps) {
        mainKeys.delete(fileKey);
      }

      // Clean up dep watcher if no other preview uses this sourceDir
      const dirKey = sourceDir.toLowerCase();
      const stillActive = Array.from(this._activePreviews.values())
        .some(p => p.sourceDir.toLowerCase() === dirKey);
      if (!stillActive) {
        const watcher = this._depWatchers.get(dirKey);
        if (watcher) {
          watcher.dispose();
          this._depWatchers.delete(dirKey);
        }
      }

      this._removeTempDir(tempDir);
    });
  }

  // ─── Debounced render ──────────────────────────────────────────────────────

  /**
   * Schedules a render with a 500 ms debounce. Each new call resets the
   * timer so that rapid typing does not trigger a burst of sub-processes.
   */
  private _debouncedRender(
    document: PMLDocument,
    webviewPanel: vscode.WebviewPanel,
    tempDir: string,
  ): void {
    this._clearDebounceTimer();
    this._debounceTimer = setTimeout(() => {
      this._debounceTimer = undefined;
      this._render(document, webviewPanel, tempDir);
    }, 500);
  }

  // ─── Render ────────────────────────────────────────────────────────────────

  /**
   * Full render cycle: read document → write temp file → spawn pml.exe →
   * update diagnostics → post message to WebView.
   *
   * Errors from `executePML` (binary not found, timeout, parse failure) and
   * any unexpected exceptions are caught and surfaced as WebView error
   * messages. Diagnostics are set for structured PML errors and cleared on
   * success.
   */
  private async _render(
    document: PMLDocument,
    webviewPanel: vscode.WebviewPanel,
    tempDir: string,
  ): Promise<void> {
    try {
      updateStatusBar('rendering');
      const renderStart = performance.now();

      // ── Step 1: read document text ──────────────────────────────────────
      const textDocument = await vscode.workspace.openTextDocument(document.uri);
      const content = textDocument.getText();

      // ── Step 2: write to temp file ──────────────────────────────────────
      const pmlPath = path.join(tempDir, 'source.pml');
      fs.writeFileSync(pmlPath, content, 'utf-8');

      // ── Step 3: spawn PML CLI ───────────────────────────────────────────
      // KEY DESIGN: cwd = original source directory, NOT tempDir.
      //   - cwd=sourceDir: (import "foo.pml") resolves against the actual
      //     project directory (e.g. W:\v\PML-Project), not a temp dir.
      //   - NOTE: -o is ignored in --json mode (run_json_mode doesn't read
      //     opts.output_dir). Output files are written to CWD (= sourceDir).
      //     We copy them to tempDir afterwards for WebView serving.
      const binaryPath = resolveBinaryPath(this.context);
      const sourceDir = path.dirname(document.uri.fsPath);
      const result = await executePML(pmlPath, sourceDir, binaryPath);
      const elapsed = Math.round(performance.now() - renderStart);

      // ── Step 4: copy output files from sourceDir → tempDir ────────────
      // The CLI writes to CWD (sourceDir); we need them in tempDir for
      // asWebviewUri() / localResourceRoots.
      for (const file of result.files) {
        const src = path.resolve(sourceDir, file);
        const dst = path.resolve(tempDir, file);
        try {
          if (fs.existsSync(src)) {
            fs.mkdirSync(path.dirname(dst), { recursive: true });
            fs.copyFileSync(src, dst);
          } else {
            console.warn(`[PML] Output file missing at source: ${src}`);
          }
        } catch (e) {
          console.warn(`[PML] Failed to copy ${src} -> ${dst}:`, e);
        }
      }

      // ── Step 5: build file URIs and post to WebView ───────────────────
      if (result.success) {
        // ── Success ─────────────────────────────────────────────────────
        resetFirstErrorShown();
        clearDiagnostics(document.uri);
        updateStatusBar('success', `${elapsed}ms`);

        const files = this._buildFileUris(result.files, tempDir, webviewPanel);
        if (files.length === 0) {
          console.warn('[PML] All output files missing — sending empty result');
        }
        this._postMessage(webviewPanel, { command: 'render', files });

        // ── Re-scan imports and update dependency tracking ─────────────
        // The user may have added or removed (import "...") forms since the
        // last render. Rebuild the dependency graph with the current editor
        // buffer content so that watchers stay in sync.
        const depFileKey = document.uri.fsPath.toLowerCase();
        this._updateDependencyTracking(depFileKey, content, sourceDir);
      } else {
        // ── PML error ────────────────────────────────────────────────────
        updateStatusBar('error');
        if (result.error) {
          setDiagnostics(document.uri, [result.error]);
          notifyErrorOnce(
            result.error.type ?? 'PMLExecutionError',
            result.error.message ?? 'PML execution failed',
          );
        }
        const err = result.error;
        this._postMessage(webviewPanel, {
          command: 'error',
          type: err?.type ?? 'PMLExecutionError',
          message: err?.message ?? 'PML execution failed with an unknown error.',
          line: err?.line,
          column: err?.column,
          hint: err?.hint,
          // Include any files that were successfully rendered despite the error
          files: this._buildFileUris(result.files, tempDir, webviewPanel),
        });
      }
    } catch (err) {
      // ── System error (rejected promise, file I/O, etc.) ─────────────────
      updateStatusBar('error');
      const message = err instanceof Error ? err.message : String(err);
      clearDiagnostics(document.uri);
      notifyErrorOnce('PMLSystemError', message);
      this._postMessage(webviewPanel, {
        command: 'error',
        type: 'PMLSystemError',
        message,
      });
    }
  }

  // ─── Helpers ───────────────────────────────────────────────────────────────

  /**
   * Build data:image/png;base64 URIs from the output file list.
   *
   * Files are expected to already exist in @p tempDir (they were copied from
   * sourceDir after CLI execution). Each file is read into a base64 data URI
   * that bypasses CSP and localResourceRoots entirely.
   *
   * This approach trades file-system serving (asWebviewUri) for inline data
   * transport via postMessage. For typical PML output sizes (10–200 KB) it
   * performs identically and eliminates all CSP / resource-root issues.
   */
  private _buildFileUris(
    fileList: string[],
    tempDir: string,
    _webviewPanel: vscode.WebviewPanel,
  ): { uri: string; name: string; isMain: boolean; size: number }[] {
    if (fileList.length === 0) return [];
    const mainFile = selectMainFile(fileList);
    return fileList
      .map(file => {
        const absPath = path.resolve(tempDir, file);
        if (!fs.existsSync(absPath)) {
          return null;
        }
        let fileSize = 0;
        try {
          fileSize = fs.statSync(absPath).size;
        } catch { /* best-effort */ }
        let dataUri: string;
        try {
          const buf = fs.readFileSync(absPath);
          const ext = path.extname(file).toLowerCase();
          const mime = ext === '.gif' ? 'image/gif' : 'image/png';
          dataUri = `data:${mime};base64,${buf.toString('base64')}`;
        } catch {
          return null;
        }
        return {
          uri: dataUri,
          name: path.basename(file),
          isMain: file === mainFile,
          size: fileSize,
        };
      })
      .filter((f): f is NonNullable<typeof f> => f !== null);
  }

  /** Safely post a message to the WebView, ignoring errors on disposed panels. */
  private _postMessage(
    webviewPanel: vscode.WebviewPanel,
    message: Record<string, unknown>,
  ): void {
    try {
      webviewPanel.webview.postMessage(message);
    } catch {
      // WebView has been disposed — nothing we can do.
    }
  }

  /** Clear the debounce timer if it exists. */
  private _clearDebounceTimer(): void {
    if (this._debounceTimer !== undefined) {
      clearTimeout(this._debounceTimer);
      this._debounceTimer = undefined;
    }
  }

  /** Best-effort removal of the per-document temp directory. */
  private _removeTempDir(dir: string): void {
    try {
      fs.rmSync(dir, { recursive: true, force: true });
    } catch {
      // Temp cleanup is best-effort; the OS will eventually reclaim it.
    }
  }

  /** Remove temp directories older than 24 hours to prevent accumulation. */
  private static _cleanStaleTempDirs(): void {
    try {
      const baseDir = path.join(os.tmpdir(), 'pml-preview');
      if (!fs.existsSync(baseDir)) return;
      const cutoff = Date.now() - 24 * 60 * 60 * 1000;
      for (const entry of fs.readdirSync(baseDir)) {
        const dirPath = path.join(baseDir, entry);
        try {
          const stat = fs.statSync(dirPath);
          if (stat.isDirectory() && stat.mtimeMs < cutoff) {
            fs.rmSync(dirPath, { recursive: true, force: true });
          }
        } catch { /* best-effort */ }
      }
    } catch { /* best-effort */ }
  }

  // ═══════════════════════════════════════════════════════════════════════════════
  // Import dependency tracking
  // ═══════════════════════════════════════════════════════════════════════════════

  /**
   * Regex matching `(import "filename" ...)` — the only statically-detectable
   * import form. Matches both plain imports and imports with :prefix:
   *   (import "filename")
   *   (import "filename" :prefix my-prefix)
   *   (import "filename" as my-prefix)
   */
  private static _importRegex = /\(import\s+"([^"]+)"[^)]*\)/g;

  /**
   * Resolve an import path argument to an absolute candidate path.
   *
   * Mirrors the CLI's `ModuleLoader::resolve_path` logic:
   *   1. Relative to the importing file's directory (the most common case).
   *   2. With `.pml` appended if the path has no extension.
   *
   * If neither exists on disk the first candidate is still returned so the
   * caller can set up a watcher for when the file is created.
   */
  private static _resolveImportPath(
    importPath: string,
    sourceDir: string,
  ): string {
    // Normalise separators for cross-platform consistency
    const normalized = importPath.replace(/\\/g, '/');
    const firstTry = path.resolve(sourceDir, normalized);
    // If it already looks like it has an extension, that's the only candidate
    if (path.extname(firstTry)) {
      return firstTry;
    }
    // Second: append .pml
    const withExt = firstTry + '.pml';
    if (fs.existsSync(withExt)) {
      return withExt;
    }
    // Fall back to the no-extension candidate (file may be created later)
    return firstTry;
  }

  /**
   * Extract all import paths from PML source content.
   *
   * Returns resolved absolute paths. Paths that cannot be resolved are
   * excluded (they will be re-scanned on the next render cycle).
   */
  private static _scanImports(
    content: string,
    sourceDir: string,
  ): string[] {
    const imports: string[] = [];
    let match: RegExpExecArray | null;
    PMLPreviewProvider._importRegex.lastIndex = 0;
    while ((match = PMLPreviewProvider._importRegex.exec(content)) !== null) {
      const importPath = match[1];
      const resolved = PMLPreviewProvider._resolveImportPath(importPath, sourceDir);
      imports.push(resolved);
    }
    return imports;
  }

  /**
   * Build / update the dependency graph for a main file.
   *
   * BFS over the import tree starting from `content`, respecting the
   * `sourceDir` as the base for path resolution. Updates both
   * `_dependencyGraph` and `_reverseDeps`.
   *
   * @param mainFileKey  — lowercased `fsPath` of the main preview file.
   * @param content      — editor-buffer content of the main file.
   * @param sourceDir    — directory of the main file (imports resolve here).
   */
  private _buildDependencyGraph(
    mainFileKey: string,
    content: string,
    sourceDir: string,
  ): void {
    // ── BFS over the import tree ──────────────────────────────────────────
    const allDeps = new Set<string>();
    const visited = new Set<string>();
    const queue: string[] = [];

    // Seed with direct imports from the main file
    const rootImports = PMLPreviewProvider._scanImports(content, sourceDir);
    for (const imp of rootImports) {
      const impKey = imp.toLowerCase();
      if (!visited.has(impKey)) {
        visited.add(impKey);
        queue.push(imp);
        allDeps.add(impKey);
      }
    }

    // Transitively resolve imports of imported files
    while (queue.length > 0) {
      const filePath = queue.shift()!;
      let fileContent: string;
      try {
        fileContent = fs.readFileSync(filePath, 'utf-8');
      } catch {
        continue; // file not found / can't read → skip
      }
      const subImports = PMLPreviewProvider._scanImports(
        fileContent,
        path.dirname(filePath),
      );
      for (const sub of subImports) {
        const subKey = sub.toLowerCase();
        if (!visited.has(subKey)) {
          visited.add(subKey);
          queue.push(sub);
          allDeps.add(subKey);
        }
      }
    }

    // ── Update `_dependencyGraph` ─────────────────────────────────────────
    this._dependencyGraph.set(mainFileKey, allDeps);

    // ── Update `_reverseDeps` ─────────────────────────────────────────────
    // First, remove this mainFileKey from every old reverse-dep entry
    for (const [, mainKeys] of this._reverseDeps) {
      mainKeys.delete(mainFileKey);
    }
    // Then add it back for each current dependency
    for (const depKey of allDeps) {
      let mainKeys = this._reverseDeps.get(depKey);
      if (!mainKeys) {
        mainKeys = new Set();
        this._reverseDeps.set(depKey, mainKeys);
      }
      mainKeys.add(mainFileKey);
    }
  }

  /**
   * Ensure a FileSystemWatcher exists for `sourceDir`.
   *
   * The watcher fires on `.pml` file changes made *outside* the editor
   * (e.g. git operations, external editors). When a watched file matches
   * an entry in `_reverseDeps`, all dependent preview panels are
   * re-rendered.
   *
   * Watchers are reference-counted: if a watcher already exists for this
   * `sourceDir` no second one is created.
   */
  private _setupDepWatcher(sourceDir: string): void {
    const dirKey = sourceDir.toLowerCase();
    if (this._depWatchers.has(dirKey)) {
      return; // already watching this directory
    }

    const pattern = new vscode.RelativePattern(sourceDir, '**/*.pml');
    const watcher = vscode.workspace.createFileSystemWatcher(
      pattern,
      false,  // watch creates
      false,  // watch changes (catches disk edits outside the editor)
      false,  // watch deletes
    );

    const handler = (uri: vscode.Uri): void => {
      const changedKey = uri.fsPath.toLowerCase();
      const mainKeys = this._reverseDeps.get(changedKey);
      if (!mainKeys || mainKeys.size === 0) {
        return;
      }
      // Trigger re-render for every dependent preview.
      // The 500 ms debounce on _debouncedRender handles deduplication
      // when the same file is also open in an editor (onDidChangeTextDocument
      // and FileSystemWatcher.onDidChange may both fire).
      for (const mainKey of mainKeys) {
        const info = this._activePreviews.get(mainKey);
        if (info) {
          this._debouncedRender(
            info.document,
            info.webviewPanel,
            info.tempDir,
          );
        }
      }
    };

    // Listen for all three events — debounce handles dedup with the
    // in-editor change subscription.
    watcher.onDidCreate(handler);
    watcher.onDidChange(handler);
    watcher.onDidDelete(handler);

    this._depWatchers.set(dirKey, watcher);
  }

  /**
   * Update dependency tracking after a render cycle.
   *
   * Called from `_render` after each successful execution:
   *   1. Re-scan the main file for imports (they may have changed).
   *   2. Rebuild `_dependencyGraph` / `_reverseDeps`.
   *   3. Ensure a FileSystemWatcher exists for the source directory.
   *
   * @param mainFileKey  — lowercased `fsPath` of the preview's main file.
   * @param content      — current editor-buffer content (with unsaved edits).
   * @param sourceDir    — directory of the main file.
   */
  private _updateDependencyTracking(
    mainFileKey: string,
    content: string,
    sourceDir: string,
  ): void {
    this._buildDependencyGraph(mainFileKey, content, sourceDir);
    this._setupDepWatcher(sourceDir);
  }

  // ─── Fallback HTML ─────────────────────────────────────────────────────────

  /**
   * Inline fallback HTML used when `media/preview.html` is not yet present.
   *
   * This produces a minimal self-contained preview page that handles the
   * same `render` / `error` message protocol as the real template.
   */
  private _getFallbackHtml(): string {
    return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="Content-Security-Policy"
        content="default-src 'none'; img-src 'self' {{cspSource}} data: https:; style-src 'unsafe-inline'; script-src 'nonce-{{nonce}}';">
  <title>PML Preview</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      padding: 16px;
      margin: 0;
      background: var(--vscode-editor-background, #1e1e1e);
      color: var(--vscode-editor-foreground, #d4d4d4);
    }
    .loading {
      text-align: center;
      padding: 48px 16px;
      color: var(--vscode-descriptionForeground, #888);
    }
    .error {
      border: 1px solid var(--vscode-inputValidation-errorBorder, #e06c75);
      background: var(--vscode-inputValidation-errorBackground, #3a1a1a);
      padding: 12px 16px;
      border-radius: 4px;
      margin-bottom: 16px;
    }
    .error h3 {
      margin: 0 0 8px 0;
      font-size: 1.1em;
      color: var(--vscode-errorForeground, #e06c75);
    }
    .error p {
      margin: 0 0 4px 0;
      word-break: break-word;
    }
    .error .hint {
      color: var(--vscode-testing-iconPassed, #98c379);
      font-size: 0.9em;
      margin-top: 8px;
      padding-top: 8px;
      border-top: 1px solid var(--vscode-inputValidation-errorBorder, #555);
    }
    .image-container {
      margin-bottom: 20px;
    }
    .image-container img {
      max-width: 100%;
      height: auto;
      display: block;
      border: 1px solid var(--vscode-widget-border, #333);
      border-radius: 4px;
    }
    .image-container .label {
      font-size: 0.85em;
      color: var(--vscode-descriptionForeground, #888);
      margin-top: 6px;
    }
  </style>
</head>
<body>
  <div id="app"><div class="loading">Loading PML preview…</div></div>
  <script nonce="{{nonce}}">
    (function () {
      var api = acquireVsCodeApi();

      window.addEventListener('message', function (event) {
        var msg = event.data;
        var app = document.getElementById('app');
        if (!app) return;

        switch (msg.command) {
          case 'render':
            if (!msg.files || msg.files.length === 0) {
              app.innerHTML = '<p>No output produced.</p>';
              break;
            }
            var html = '';
            for (var i = 0; i < msg.files.length; i++) {
              var file = msg.files[i];
              html += '<div class="image-container">';
              if (file.isMain) {
                html += '<strong>Output:</strong><br>';
              }
              html += '<img src="' + encodeURI(file.uri) + '" alt="' + escapeHtml(file.name) + '">';
              html += '<div class="label">' + escapeHtml(file.name) + '</div>';
              html += '</div>';
            }
            app.innerHTML = html;
            break;

          case 'error':
            var hintHtml = msg.hint
              ? '<div class="hint">Tip: ' + escapeHtml(msg.hint) + '</div>'
              : '';
            app.innerHTML = '<div class="error">' +
              '<h3>' + escapeHtml(msg.type || 'Error') + '</h3>' +
              '<p>' + escapeHtml(msg.message) + '</p>' +
              hintHtml +
              '</div>';
            break;
        }
      });

      api.postMessage({ command: 'ready' });

      function escapeHtml(str) {
        if (typeof str !== 'string') return String(str);
        return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
      }
    })();
  </script>
</body>
</html>`;
  }
}
