import * as vscode from 'vscode';

let _channel: vscode.OutputChannel | undefined;

export function initOutputChannel(): vscode.OutputChannel {
  _channel = vscode.window.createOutputChannel('PML Render');
  return _channel;
}

export function getOutputChannel(): vscode.OutputChannel | undefined {
  return _channel;
}

export function logRender(
  filePath: string,
  binaryPath: string,
  cwd: string,
  success: boolean,
  elapsed: number,
  files: string[],
  error?: { type: string; message: string; hint?: string } | null,
): void {
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
