import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import { execFile } from 'child_process';
import * as vscode from 'vscode';

// ─── Interfaces ──────────────────────────────────────────────────────────────

/**
 * Represents an error returned by the PML CLI in its JSON output.
 * The structure mirrors the CLI's error payload and supports nested
 * `details` for multi-parse-error scenarios.
 */
export interface RenderError {
  /** Machine-readable error type, e.g. "PMLSyntaxError", "PMLRuntimeError". */
  type: string;
  /** Human-readable error description. */
  message: string;
  /** 1-based line number where the error occurred, if applicable. */
  line?: number;
  /** 1-based column number where the error occurred, if applicable. */
  column?: number;
  /** Source file the error refers to, if relevant. */
  filename?: string;
  /** A suggested fix or explanatory hint. */
  hint?: string;
  /** Sub-errors for aggregate errors (e.g. 2 parse errors reported together). */
  details?: RenderError[];
}

/**
 * Result of executing a PML file via the CLI.
 */
export interface RenderResult {
  /** Whether the PML execution completed without errors. */
  success: boolean;
  /** Paths to output files (relative to the working directory). */
  files: string[];
  /** Error information when `success` is false; null otherwise. */
  error: RenderError | null;
}

// ─── First-error toast ────────────────────────────────────────────────────────

/**
 * Module-level flag to ensure the toast notification is shown only once per
 * extension session. Reset on any successful execution.
 */
let _firstErrorShown = false;

/**
 * Shows a VSCode error toast on the first execution error in a session.
 * Subsequent errors are silent — they still appear in the diagnostics panel
 * and the WebView, but no toast is shown to avoid spam.
 *
 * Call `resetFirstErrorShown()` on successful execution to allow a fresh
 * toast on the next error.
 *
 * @param type    - Machine-readable error type (e.g. "PMLSyntaxError").
 * @param message - Human-readable error description.
 */
export function notifyErrorOnce(type: string, message: string): void {
  if (!_firstErrorShown) {
    _firstErrorShown = true;
    vscode.window.showErrorMessage(`PML: ${type} - ${message}`);
  }
  // Subsequent errors: silent. Diagnostics/panel handle the rest.
}

/**
 * Resets the first-error-shown flag so the next error triggers a toast again.
 * Call this on every successful execution to allow fresh toasts on subsequent
 * failures.
 */
export function resetFirstErrorShown(): void {
  _firstErrorShown = false;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

/**
 * Returns a temporary directory path (`<TEMP>/pml-preview/`) and ensures it
 * exists on disk by creating it recursively.
 *
 * Uses `os.tmpdir()` for cross-platform compatibility.
 */
export function getTempDir(): string {
  const dir = path.join(os.tmpdir(), 'pml-preview');
  fs.mkdirSync(dir, { recursive: true });
  return dir;
}

/**
 * Selects the "main" output file from a list of rendered files.
 *
 * Heuristic: prefers the file without `@2x` or `@4x` suffix (i.e. the
 * non-scaled output from render-set). Falls back to the last file.
 *
 * @param files - Array of output file paths (relative or absolute).
 * @returns The selected file path, or `null` if the array is empty.
 */
export function selectMainFile(files: string[]): string | null {
  if (files.length === 0) {
    return null;
  }
  // Prefer first file without @2x/@4x scaling suffix.
  for (const f of files) {
    const basename = path.basename(f);
    if (!/@[2-9]x/.test(basename)) {
      return f;
    }
  }
  return files[files.length - 1];
}

/**
 * Resolves the absolute path to the bundled `pml.exe` (Windows) or `pml`
 * (Linux/macOS) binary using the VSCode extension context.
 *
 * The binary is expected at `pml-bin/pml(.exe)` relative to the extension root.
 *
 * @param context - The VSCode extension context (provides `asAbsolutePath`).
 * @returns The absolute path to the PML binary.
 */
export function resolveBinaryPath(context: vscode.ExtensionContext): string {
  const binaryName = process.platform === 'win32' ? 'pml.exe' : 'pml';
  return context.asAbsolutePath(path.join('pml-bin', binaryName));
}

/**
 * Parses a `RenderError` from an unknown JSON value, performing basic shape
 * validation. Returns `null` if the value does not look like an error object.
 */
function parseError(raw: unknown): RenderError | null {
  if (raw === null || typeof raw !== 'object') {
    return null;
  }
  const obj = raw as Record<string, unknown>;
  if (typeof obj.type !== 'string' || typeof obj.message !== 'string') {
    return null;
  }
  const error: RenderError = {
    type: obj.type,
    message: obj.message,
  };
  if (typeof obj.line === 'number') {
    error.line = obj.line;
  }
  if (typeof obj.column === 'number') {
    error.column = obj.column;
  }
  if (typeof obj.filename === 'string') {
    error.filename = obj.filename;
  }
  if (typeof obj.hint === 'string') {
    error.hint = obj.hint;
  }
  if (Array.isArray(obj.details)) {
    error.details = obj.details
      .map((d: unknown) => parseError(d))
      .filter((d: RenderError | null): d is RenderError => d !== null);
  }
  return error;
}

/**
 * Parses the raw stdout string from the PML CLI into a `RenderResult`.
 *
 * Handles three output shapes:
 *  - Success `{"success": true, "files": ["..."]}`
 *  - Single error `{"success": false, "error": {...}}`
 *  - Multi-parse error with `error.details`
 *
 * @param stdout - Raw stdout text (expected to be valid JSON).
 * @returns A `RenderResult` derived from the JSON payload.
 * @throws {Error} If the stdout is not valid JSON or the top-level shape is unrecognised.
 */
function parseCLIOutput(stdout: string): RenderResult {
  let parsed: unknown;
  try {
    parsed = JSON.parse(stdout);
  } catch {
    throw new Error(
      `Failed to parse PML CLI output as JSON.\nRaw output:\n${stdout.slice(0, 2000)}`,
    );
  }

  if (parsed === null || typeof parsed !== 'object') {
    throw new Error(
      `Unexpected PML CLI output type: expected a JSON object, got ${typeof parsed}`,
    );
  }

  const obj = parsed as Record<string, unknown>;

  // ── Validate top-level fields ──────────────────────────────────────────
  const success = typeof obj.success === 'boolean' ? obj.success : false;
  const filesRaw = obj.files;
  const files: string[] = Array.isArray(filesRaw)
    ? filesRaw.filter((f): f is string => typeof f === 'string')
    : [];

  const errorRaw = obj.error;
  const error: RenderError | null =
    errorRaw !== null && errorRaw !== undefined
      ? parseError(errorRaw)
      : null;

  return { success, files, error };
}

// ─── Core ────────────────────────────────────────────────────────────────────

/**
 * Executes a PML file through the `pml.exe` CLI and returns the parsed result.
 *
 * Spawns the binary with the `--json` flag so the output is machine-readable
 * JSON. The binary is invoked via `child_process.execFile()` — no shell
 * involvement, safe for cross-platform use.
 *
 * @param sourceFile - Absolute path to the `.pml` source file to execute.
 * @param cwd        - Working directory for the child process (typically a
 *                     temp directory where output files will be written).
 * @param binaryPath - Absolute path to the `pml.exe` binary.
 * @returns A promise that resolves to a `RenderResult`.
 * @throws {Error} If the binary is missing, the process times out (30 s),
 *                 exits with a non-zero code, or produces unparseable output.
 */
export function executePML(
  sourceFile: string,
  cwd: string,
  binaryPath: string,
  outputDir?: string,
): Promise<RenderResult> {
  return new Promise<RenderResult>((resolve, reject) => {
    // ── Verify binary exists early ───────────────────────────────────────
    if (!fs.existsSync(binaryPath)) {
      reject(
        new Error(
          `PML binary not found at "${binaryPath}". ` +
            `Ensure pml.exe is bundled at pml-bin/pml.exe.`,
        ),
      );
      return;
    }

    const args: string[] = [sourceFile, '--json'];
    if (outputDir) {
      args.push('-o', outputDir);
    }
    const timeoutMs = 30_000;
    const maxBufferBytes = 10 * 1024 * 1024; // 10 MB

    execFile(
      binaryPath,
      args,
      {
        cwd,
        timeout: timeoutMs,
        maxBuffer: maxBufferBytes,
        windowsHide: true,
      },
      (error, stdout, stderr) => {
        // ── Timeout ──────────────────────────────────────────────────────
        if (error && error.killed) {
          reject(
            new Error(
              `PML execution timed out after ${timeoutMs / 1000} s. ` +
                `File: "${sourceFile}".`,
            ),
          );
          return;
        }

        // ── Non-zero exit (but stdout may still contain useful JSON) ────
        if (error && !stdout) {
          const detail = stderr
            ? `\nStderr:\n${stderr.slice(0, 2000)}`
            : '';
          reject(
            new Error(
              `PML process exited with error for "${sourceFile}".` +
                detail,
            ),
          );
          return;
        }

        // ── Parse stdout ─────────────────────────────────────────────────
        try {
          const result = parseCLIOutput(stdout);
          resolve(result);
        } catch (parseError) {
          const stderrHint = stderr
            ? `\nStderr:\n${stderr.slice(0, 2000)}`
            : '';
          reject(
            new Error(
              `Failed to interpret PML CLI output for "${sourceFile}".` +
                stderrHint +
                `\n--- stdout (first 2000 chars) ---\n${stdout.slice(0, 2000)}`,
            ),
          );
        }
      },
    );
  });
}
