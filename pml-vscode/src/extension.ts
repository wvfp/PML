import * as vscode from 'vscode';
import { PreviewPanel } from './preview';
import { LspClient } from './lsp-client';

let previewPanel: PreviewPanel | undefined;
let lspClient: LspClient | undefined;

export function activate(context: vscode.ExtensionContext) {
  console.log('PML extension activating...');

  // ── Preview command ─────────────────────────────────────────
  const previewDisposable = vscode.commands.registerCommand('pml.preview', () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== 'pml') {
      vscode.window.showWarningMessage('Open a .pml file first.');
      return;
    }

    if (!previewPanel) {
      previewPanel = new PreviewPanel(context.extensionUri);
      previewPanel.onDispose(() => { previewPanel = undefined; });
    }

    previewPanel.show(editor.document);
  });

  // ── Save preview image ──────────────────────────────────────
  const saveDisposable = vscode.commands.registerCommand('pml.preview.save', async () => {
    if (!previewPanel) {
      vscode.window.showWarningMessage('No active PML preview.');
      return;
    }
    await previewPanel.saveAs();
  });

  // ── Editor change → auto-refresh preview ────────────────────
  const changeDisposable = vscode.workspace.onDidChangeTextDocument((e) => {
    if (previewPanel && e.document.languageId === 'pml') {
      const config = vscode.workspace.getConfiguration('pml.preview');
      if (config.get<boolean>('autoRefresh', true)) {
        previewPanel.render(e.document);
      }
    }
  });

  // ── Save → force refresh preview ────────────────────────────
  const saveDocDisposable = vscode.workspace.onDidSaveTextDocument((doc) => {
    if (previewPanel && doc.languageId === 'pml') {
      previewPanel.render(doc);
    }
  });

  // ── LSP client ─────────────────────────────────────────────
  const lspConfig = vscode.workspace.getConfiguration('pml.lsp');
  if (lspConfig.get<boolean>('enabled', true)) {
    lspClient = new LspClient();
    lspClient.start();
  }

  // ── Register completions (fallback if LSP is off) ──────────
  const completionDisposable = vscode.languages.registerCompletionItemProvider(
    { language: 'pml' },
    {
      provideCompletionItems(document, position) {
        const items: vscode.CompletionItem[] = [];

        const specialForms = [
          'define', 'lambda', 'if', 'cond', 'let', 'let*', 'begin',
          'set!', 'quote', 'quasiquote', 'defmacro', 'import', 'provide',
          'defskeleton', 'animate', 'play', 'parallel', 'sequence',
        ];
        for (const sf of specialForms) {
          items.push(new vscode.CompletionItem(sf, vscode.CompletionItemKind.Keyword));
        }

        const builtins = [
          '+', '-', '*', '/', 'car', 'cdr', 'cons', 'list', 'length',
          'map', 'filter', 'reduce', 'println', 'print', 'display',
          'number?', 'string?', 'boolean?', 'symbol?', 'list?', 'eq?',
        ];
        for (const b of builtins) {
          items.push(new vscode.CompletionItem(b, vscode.CompletionItemKind.Function));
        }

        const components = [
          'rect', 'circle', 'ellipse', 'line', 'polygon', 'path', 'text', 'image',
          'canvas', 'sprite-canvas', 'add', 'render', 'render-spritesheet',
          'translate', 'scale', 'rotate', 'compose',
          'character', 'body', 'head', 'anime-eyes', 'mouth', 'hair',
          'outfit', 'weapon', 'potion', 'chest', 'generic-item',
          'button', 'panel', 'health-bar', 'icon', 'tile', 'decoration', 'background',
          'define-style', 'use-style', 'define-palette', 'use-palette',
          'instantiate-skeleton', 'ik-solve', 'bind-skin', 'joint-position',
        ];
        for (const c of components) {
          items.push(new vscode.CompletionItem(c, vscode.CompletionItemKind.Class));
        }

        return items;
      },
    },
    '(' // trigger character
  );

  context.subscriptions.push(
    previewDisposable,
    saveDisposable,
    changeDisposable,
    saveDocDisposable,
    completionDisposable,
  );

  // Auto-open preview if a pml file is active on startup
  if (vscode.window.activeTextEditor?.document.languageId === 'pml') {
    vscode.commands.executeCommand('pml.preview');
  }
}

export function deactivate() {
  if (lspClient) {
    lspClient.stop();
  }
}
