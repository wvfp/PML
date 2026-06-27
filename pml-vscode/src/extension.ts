import * as vscode from 'vscode';
import { PMLPreviewProvider } from './previewProvider';
import { initDiagnostics } from './diagnostics';
import { registerCompletionProvider } from './completionProvider';
import { initOutputChannel, getOutputChannel } from './outputChannel';

// ═══════════════════════════════════════════════════════════════════════════════
// Diagnostics collection (module-level singleton)
// ═══════════════════════════════════════════════════════════════════════════════

let _diagnostics: vscode.DiagnosticCollection;
let statusBarItem: vscode.StatusBarItem;

/**
 * Returns the extension's singleton PML diagnostics collection.
 * Created during activate(); available after extension activation.
 */
export function getDiagnostics(): vscode.DiagnosticCollection {
  return _diagnostics;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Status bar updates
// ═══════════════════════════════════════════════════════════════════════════════

export type StatusBarState = 'idle' | 'rendering' | 'success' | 'error';

/**
 * Update the PML status bar indicator with the current render state.
 *
 * - `'idle'`:      "PML" (default)
 * - `'rendering'`: "$(loading~spin) PML"
 * - `'success'`:   "$(check) PML {detail}"  (detail = elapsed time like "150ms")
 * - `'error'`:     "$(error) PML" or "$(error) PML - {detail}"
 */
export function updateStatusBar(status: StatusBarState, detail?: string): void {
  switch (status) {
    case 'idle':
      statusBarItem.text = 'PML';
      break;
    case 'rendering':
      statusBarItem.text = '$(loading~spin) PML';
      break;
    case 'success':
      statusBarItem.text = detail
        ? `$(check) PML ${detail}`
        : '$(check) PML';
      break;
    case 'error':
      statusBarItem.text = detail
        ? `$(error) PML - ${detail}`
        : '$(error) PML';
      break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Activate
// ═══════════════════════════════════════════════════════════════════════════════

export function activate(context: vscode.ExtensionContext) {
  // ── Diagnostics ──────────────────────────────────────────────────────────────
  _diagnostics = vscode.languages.createDiagnosticCollection('pml');
  context.subscriptions.push(_diagnostics);
  initDiagnostics(_diagnostics);

  // ── Output channel ───────────────────────────────────────────────────────────
  initOutputChannel();

  // ── Status bar item ──────────────────────────────────────────────────────────
  statusBarItem = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Right,
    100,
  );
  statusBarItem.text = 'PML';
  statusBarItem.tooltip = 'Open PML Preview to the Side';
  statusBarItem.command = 'pml.previewSide';
  statusBarItem.show();
  context.subscriptions.push(statusBarItem);

  // ── Live/Manual render mode toggle ─────────────────────────────────────────
  const liveToggle = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Right, 99,
  );

  function updateLiveToggle() {
    const mode = vscode.workspace.getConfiguration('pml').get('preview.autoRender');
    if (mode === 'manual') {
      liveToggle.text = '$(debug-pause) Manual';
      liveToggle.tooltip = 'Render on save only | Click for Live mode';
    } else {
      liveToggle.text = '$(sync) Live';
      liveToggle.tooltip = 'Render on keystroke (500ms debounce) | Click for Manual mode';
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

  // ── Code completion provider ───────────────────────────────────────────────
  registerCompletionProvider(context);

  // ── Custom editor provider ───────────────────────────────────────────────────
  const provider = new PMLPreviewProvider(context);
  context.subscriptions.push(
    vscode.window.registerCustomEditorProvider('pml.previewEditor', provider),
  );

  // ── Commands ─────────────────────────────────────────────────────────────────
  context.subscriptions.push(
    vscode.commands.registerCommand(
      'pml.previewSide',
      (uri?: vscode.Uri) => {
        const target = uri ?? vscode.window.activeTextEditor?.document.uri;
        if (!target) {
          vscode.window.showErrorMessage(
            'No PML file selected. Open a .pml file first, then run "Open PML Preview to the Side".',
          );
          return;
        }
        vscode.commands.executeCommand(
          'vscode.openWith',
          target,
          'pml.previewEditor',
          { viewColumn: vscode.ViewColumn.Beside },
        );
      },
    ),
  );

  // ── Show output channel ──────────────────────────────────────────────────────
  context.subscriptions.push(
    vscode.commands.registerCommand('pml.showOutput', () => {
      const ch = getOutputChannel();
      if (ch) ch.show();
    }),
  );
}

// ═══════════════════════════════════════════════════════════════════════════════
// Deactivate
// ═══════════════════════════════════════════════════════════════════════════════

export function deactivate() {
  // Dispose diagnostics explicitly. (Also auto-disposed via context.subscriptions
  // in activate(), but explicit cleanup ensures orderly teardown.)
  if (_diagnostics) {
    _diagnostics.dispose();
  }
}
