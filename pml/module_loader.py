"""PML module system — Module class and ModuleLoader with caching and circular detection."""

from __future__ import annotations

import os
from pathlib import Path
from typing import Any, TYPE_CHECKING

from pml.environment import Environment
from pml.errors import ImportError_, CircularImportError, AccessError
from pml.lexer import Lexer
from pml.parser import Parser
from pml.types import Symbol

if TYPE_CHECKING:
    from pml.evaluator import evaluate


class Module:
    """A loaded PML module with its environment and export set."""

    __slots__ = ("name", "env", "exports")

    def __init__(self, name: str, env: Environment, exports: set[str]) -> None:
        self.name = name
        self.env = env
        self.exports = exports

    def get(self, symbol: str) -> Any:
        """Get an exported symbol's value. Raises AccessError if not exported."""
        if symbol not in self.exports:
            raise AccessError(
                f"'{symbol}' is not exported from module '{self.name}'. "
                f"Exported: {sorted(self.exports)}"
            )
        return self.env.lookup(symbol)

    def has(self, symbol: str) -> bool:
        """Check if a symbol is exported from this module."""
        return symbol in self.exports

    def __repr__(self) -> str:
        return f"<Module '{self.name}' exports={sorted(self.exports)}>"


class ModuleLoader:
    """Loads PML modules with path resolution, caching, and circular dependency detection.

    Path resolution order:
    1. Relative to the importing file's directory
    2. PML_PATH environment variable (colon-separated directories)
    3. stdlib/ directory (relative to pml package root)
    """

    def __init__(self, global_env: Environment) -> None:
        self.global_env = global_env
        self.cache: dict[str, Module] = {}  # resolved absolute path -> Module
        self.loading: set[str] = set()       # currently loading (circular detection)
        self._stdlib_dir = self._find_stdlib()

    @staticmethod
    def _find_stdlib() -> str:
        """Locate the stdlib/ directory relative to the pml package root."""
        pml_dir = Path(__file__).parent
        stdlib = pml_dir.parent / "stdlib"
        if stdlib.is_dir():
            return str(stdlib)
        return ""

    def resolve_path(self, path: str, from_file: str = "") -> str:
        """Resolve a module path using the search order.

        Returns the absolute resolved path, or raises ImportError_.
        """
        # 1. Relative to importing file
        if from_file:
            base_dir = os.path.dirname(os.path.abspath(from_file))
            candidate = os.path.normpath(os.path.join(base_dir, path))
            if os.path.isfile(candidate):
                return candidate

        # 2. PML_PATH environment variable
        pml_path = os.environ.get("PML_PATH", "")
        if pml_path:
            for search_dir in pml_path.split(os.pathsep):
                search_dir = search_dir.strip()
                if search_dir:
                    candidate = os.path.normpath(os.path.join(search_dir, path))
                    if os.path.isfile(candidate):
                        return candidate

        # 3. stdlib directory
        if self._stdlib_dir:
            candidate = os.path.normpath(os.path.join(self._stdlib_dir, path))
            if os.path.isfile(candidate):
                return candidate

        # Not found
        raise ImportError_(
            f"Module not found: '{path}'"
            + (f" (imported from {from_file})" if from_file else "")
        )

    def load(self, path: str, from_file: str = "") -> Module:
        """Load a module, returning the cached version if already loaded.

        Args:
            path: The module path (e.g., "lib/math.pml" or "sprites/character.pml")
            from_file: The file doing the importing (for relative path resolution)

        Returns:
            The loaded Module

        Raises:
            ImportError_: if file not found
            CircularImportError: if circular dependency detected
        """
        resolved = self.resolve_path(path, from_file)

        # Check cache — idempotent loading
        if resolved in self.cache:
            return self.cache[resolved]

        # Circular dependency detection
        if resolved in self.loading:
            raise CircularImportError(
                f"Circular dependency detected: '{path}' is already being loaded"
            )

        self.loading.add(resolved)

        try:
            module = self._do_load(resolved, path)
            self.cache[resolved] = module
            return module
        finally:
            self.loading.discard(resolved)

    def _do_load(self, resolved_path: str, original_path: str) -> Module:
        """Internal: parse, expand macros, and evaluate a module file."""
        from pml.evaluator import evaluate as eval_expr
        from pml.expander import Expander

        # Read and parse
        with open(resolved_path, "r", encoding="utf-8") as f:
            source = f.read()

        tokens = Lexer(source, resolved_path).tokenize()
        ast = Parser(tokens, resolved_path).parse()

        # Create isolated module environment
        module_env = Environment(parent=self.global_env)

        # Macro expansion pass
        expander = Expander(module_env)
        ast = expander.expand_all(ast)

        # Evaluate all expressions in module environment
        for expr in ast:
            eval_expr(expr, module_env)

        # Extract module name from path
        name = Path(original_path).stem

        # Exports were set by (provide ...) expressions during evaluation
        module = Module(name=name, env=module_env, exports=module_env.exports)

        return module
