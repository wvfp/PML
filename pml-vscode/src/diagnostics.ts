import * as vscode from 'vscode';
import { type RenderError } from './executor';

// ═══════════════════════════════════════════════════════════════════════════════
// Module-level collection reference
// ═══════════════════════════════════════════════════════════════════════════════

let _collection: vscode.DiagnosticCollection | undefined;

// ═══════════════════════════════════════════════════════════════════════════════
// initDiagnostics
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Initialises the PML diagnostics module with a VSCode diagnostic collection.
 *
 * Call this once during extension activation with the collection created by
 * `vscode.languages.createDiagnosticCollection('pml')`.
 *
 * @param collection - The PML diagnostic collection to write into.
 */
export function initDiagnostics(collection: vscode.DiagnosticCollection): void {
  _collection = collection;
}

// ═══════════════════════════════════════════════════════════════════════════════
// setDiagnostics
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Sets inline diagnostics for a given URI from an array of PML render errors.
 *
 * Each `RenderError` is mapped to one or more `vscode.Diagnostic` values:
 *
 *  - Errors with a `details` array produce one Diagnostic per sub-error
 *    (recursively), giving the user separate squiggles for each parse error.
 *  - Errors without `details` produce a single Diagnostic.
 *
 * Line/column values are converted from 1-based (PML convention) to 0-based
 * (VSCode convention). If line/column are absent, the diagnostic is placed at
 * position (0, 0).
 *
 * Severity is derived from the error type string:
 *  - Contains "Warning" → `DiagnosticSeverity.Warning`
 *  - Contains "Error" (or anything else) → `DiagnosticSeverity.Error`
 *
 * @param uri    - The document URI to attach diagnostics to.
 * @param errors - The PML render errors to display.
 */
export function setDiagnostics(uri: vscode.Uri, errors: RenderError[]): void {
  if (!_collection) {
    return;
  }

  // Clear existing diagnostics for this URI first.
  _collection.delete(uri);

  const diagnostics: vscode.Diagnostic[] = [];

  for (const error of errors) {
    flattenDiagnostics(error, diagnostics);
  }

  _collection.set(uri, diagnostics);
}

// ═══════════════════════════════════════════════════════════════════════════════
// clearDiagnostics
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Clears all PML diagnostics for the given URI.
 *
 * @param uri - The document URI to clear diagnostics for.
 */
export function clearDiagnostics(uri: vscode.Uri): void {
  if (!_collection) {
    return;
  }
  _collection.delete(uri);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Recursively converts a `RenderError` (and its `details`, if any) into
 * `vscode.Diagnostic` values, appending them to the supplied array.
 *
 * If the error has a non-empty `details` array, the parent error is **not**
 * emitted as its own Diagnostic; only the leaf sub-errors are shown. This
 * avoids duplicate squiggles for aggregate errors (e.g. a syntax error with
 * two sub-parse failures).
 *
 * @param error       - The error (or sub-error) to convert.
 * @param diagnostics - Accumulator array receiving the diagnostic entries.
 */
function flattenDiagnostics(
  error: RenderError,
  diagnostics: vscode.Diagnostic[],
): void {
  // ── If the error has sub-errors, recurse into them instead. ─────────────
  if (error.details && error.details.length > 0) {
    for (const detail of error.details) {
      flattenDiagnostics(detail, diagnostics);
    }
    return;
  }

  // ── Determine the range (0-based) ───────────────────────────────────────
  const range =
    error.line !== undefined && error.column !== undefined
      ? new vscode.Range(error.line - 1, error.column - 1, error.line - 1, error.column - 1)
      : new vscode.Range(0, 0, 0, 0);

  // ── Determine severity from the error type ──────────────────────────────
  let severity = vscode.DiagnosticSeverity.Error;
  if (error.type.includes('Warning')) {
    severity = vscode.DiagnosticSeverity.Warning;
  }

  // ── Build the diagnostic ────────────────────────────────────────────────
  const diagnostic = new vscode.Diagnostic(range, error.message, severity);
  diagnostic.source = 'pml';
  diagnostic.code = error.type;

  diagnostics.push(diagnostic);
}
