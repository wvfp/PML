import * as vscode from 'vscode';

interface PMLBuiltin {
  label: string;
  detail: string;
  documentation: string;
  kind: vscode.CompletionItemKind;
}

const BUILTINS: PMLBuiltin[] = [
  // ── Shapes ──
  { label: 'circle', detail: '(circle cx cy r [:fill] [:stroke] [:stroke-width])', documentation: 'Create a circle GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'rect', detail: '(rect x y w h [:rx] [:fill] [:stroke])', documentation: 'Create a rectangle GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'ellipse', detail: '(ellipse cx cy rx ry [:fill] [:stroke])', documentation: 'Create an ellipse GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'line', detail: '(line x1 y1 x2 y2 [:stroke] [:stroke-width])', documentation: 'Create a line GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'polygon', detail: '(polygon points [:fill] [:stroke] [:edge-noise])', documentation: 'Create a polygon GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'text', detail: '(text x y content [:fill] [:font-size])', documentation: 'Create a text GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'path', detail: '(path [:d svg-string] [:fill] [:stroke])', documentation: 'Create a path GraphicObject', kind: vscode.CompletionItemKind.Function },
  { label: 'image', detail: '(image x y w h path [:opacity])', documentation: 'Load and display an image', kind: vscode.CompletionItemKind.Function },
  { label: 'group', detail: '(group obj1 obj2 ...)', documentation: 'Group graphic objects', kind: vscode.CompletionItemKind.Function },
  // ── Canvas ──
  { label: 'canvas', detail: '(canvas w h [:bg color])', documentation: 'Create a canvas', kind: vscode.CompletionItemKind.Function },
  { label: 'add', detail: '(add graphic-object)', documentation: 'Add a graphic object to the current canvas', kind: vscode.CompletionItemKind.Function },
  { label: 'render', detail: '(render "filename.png")', documentation: 'Render output to file', kind: vscode.CompletionItemKind.Function },
  { label: 'set-backend!', detail: '(set-backend! "skia")', documentation: 'Set the render backend', kind: vscode.CompletionItemKind.Function },
  // ── Style ──
  { label: 'with-transform', detail: '(with-transform obj :transform t)', documentation: 'Apply a transform', kind: vscode.CompletionItemKind.Function },
  { label: 'apply-shader!', detail: '(apply-shader! handle)', documentation: 'Apply a shader', kind: vscode.CompletionItemKind.Function },
  // ── Programming ──
  { label: 'define', detail: '(define name value)', documentation: 'Define a variable', kind: vscode.CompletionItemKind.Keyword },
  { label: 'let', detail: '(let ((var val) ...) body)', documentation: 'Sequential binding', kind: vscode.CompletionItemKind.Keyword },
  { label: 'let-par', detail: '(let-par ((var val) ...) body)', documentation: 'Parallel binding', kind: vscode.CompletionItemKind.Keyword },
  { label: 'lambda', detail: '(lambda (params) body)', documentation: 'Create an anonymous function', kind: vscode.CompletionItemKind.Keyword },
  { label: 'if', detail: '(if cond then-expr else-expr)', documentation: 'Conditional branch', kind: vscode.CompletionItemKind.Keyword },
  { label: 'import', detail: '(import "filename.pml")', documentation: 'Import a module', kind: vscode.CompletionItemKind.Keyword },
  { label: 'defmacro', detail: '(defmacro name (params) body)', documentation: 'Define a macro', kind: vscode.CompletionItemKind.Keyword },
  // ── Lisp operations ──
  { label: 'first', detail: '(first lst)', documentation: 'Get the first element of a list', kind: vscode.CompletionItemKind.Function },
  { label: 'rest', detail: '(rest lst)', documentation: 'Get the rest of a list', kind: vscode.CompletionItemKind.Function },
  { label: 'cons', detail: '(cons val lst)', documentation: 'Construct a new list', kind: vscode.CompletionItemKind.Function },
  { label: 'list', detail: '(list ...)', documentation: 'Create a list', kind: vscode.CompletionItemKind.Function },
  { label: 'map', detail: '(map fn lst)', documentation: 'Map a function over a list', kind: vscode.CompletionItemKind.Function },
  { label: 'filter', detail: '(filter pred lst)', documentation: 'Filter a list', kind: vscode.CompletionItemKind.Function },
  { label: 'apply', detail: '(apply fn args)', documentation: 'Apply a function to arguments', kind: vscode.CompletionItemKind.Function },
  { label: 'remove', detail: '(remove val lst)', documentation: 'Remove elements from a list', kind: vscode.CompletionItemKind.Function },
  { label: 'for-each', detail: '(for-each fn lst)', documentation: 'Iterate over a list', kind: vscode.CompletionItemKind.Function },
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

function getCompletions(document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
  const lineText = document.lineAt(position).text;
  const textBeforeCursor = lineText.substring(0, position.character);

  // If typing after a colon inside an S-expression, offer keyword args
  if (textBeforeCursor.includes(':') && /\([^)]*$/.test(textBeforeCursor)) {
    return KEYWORD_ARGS;
  }

  // Default: offer builtins
  return BUILTINS.map(b => {
    const item = new vscode.CompletionItem(b.label, b.kind);
    item.detail = b.detail;
    item.documentation = b.documentation;
    return item;
  });
}

export function registerCompletionProvider(context: vscode.ExtensionContext): void {
  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider('pml', {
      provideCompletionItems(document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
        return getCompletions(document, position);
      },
    }, ':', ...'abcdefghijklmnopqrstuvwxyz'.split('')),
  );
}
