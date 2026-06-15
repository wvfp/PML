"""PML REPL and CLI entry point."""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from typing import Any

from pml.builtins import register_all
from pml.environment import Environment
from pml.errors import PMLError
from pml.evaluator import apply_function, evaluate
from pml.expander import Expander
from pml.graphics.render import set_output_dir
from pml.lexer import Lexer
from pml.parser import Parser
from pml.types import BuiltinProcedure


# ======================================================================
# Global environment setup
# ======================================================================


def create_global_env() -> Environment:
    """Create and populate the global environment with all builtins."""
    env = Environment()
    register_all(env)

    # Register transform builtins
    from pml.transform_builtins import register_transforms
    register_transforms(env)

    # Register graphics subsystem (primitives, canvas, render)
    from pml.graphics import register_graphics
    register_graphics(env)

    # Register sprite subsystem (style, palette, semantic components)
    from pml.sprites import register_sprites
    register_sprites(env)

    # Register animation subsystem (animate, timeline, GIF output)
    from pml.animation import register_animation
    register_animation(env)

    # Register skeleton subsystem (defskeleton, IK, bind-skin)
    from pml.skeleton import register_skeleton
    register_skeleton(env)

    # Register higher-order functions that need evaluator access
    def _map(fn: Any, lst: list) -> list:
        return [apply_function(fn, [item], {}) for item in lst]

    def _filter(fn: Any, lst: list) -> list:
        from pml.evaluator import is_truthy

        return [item for item in lst if is_truthy(apply_function(fn, [item], {}))]

    def _reduce(fn: Any, init: Any, lst: list) -> Any:
        acc = init
        for item in lst:
            acc = apply_function(fn, [acc, item], {})
        return acc

    env.define("map", BuiltinProcedure("map", _map))
    env.define("filter", BuiltinProcedure("filter", _filter))
    env.define("reduce", BuiltinProcedure("reduce", _reduce))

    # gensym for macro hygiene
    _gensym_counter = [0]

    def _gensym(prefix: str = "g") -> Any:
        from pml.types import Symbol

        _gensym_counter[0] += 1
        return Symbol(f"{prefix}_{_gensym_counter[0]}")

    env.define("gensym", BuiltinProcedure("gensym", _gensym))

    return env


# ======================================================================
# File execution
# ======================================================================


def run_file(filename: str, env: Environment | None = None) -> Any:
    """Read, parse, expand macros, and evaluate a .pml file."""
    if env is None:
        env = create_global_env()

    with open(filename, "r", encoding="utf-8") as f:
        source = f.read()

    tokens = Lexer(source, filename).tokenize()
    ast = Parser(tokens, filename).parse()

    # Macro expansion pass
    expander = Expander(env)
    ast = expander.expand_all(ast)

    # Store source file path for relative module imports
    env.define("__source_file__", filename)

    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result


# ======================================================================
# REPL
# ======================================================================


def repl() -> None:
    """Interactive Read-Eval-Print Loop."""
    from pml.builtins.io import _pml_repr

    env = create_global_env()
    print("PML 0.1.0 — Type (exit) or Ctrl-D to quit.")
    while True:
        try:
            line = input("pml> ")
        except (EOFError, KeyboardInterrupt):
            print()
            break

        line = line.strip()
        if not line:
            continue
        if line in ("(exit)", "exit", "quit"):
            break

        try:
            tokens = Lexer(line).tokenize()
            ast = Parser(tokens).parse()
            expander = Expander(env)
            ast = expander.expand_all(ast)
            for expr in ast:
                result = evaluate(expr, env)
                if result is not None:
                    print(_pml_repr(result))
        except PMLError as e:
            print(f"Error: {e}", file=sys.stderr)
        except Exception as e:
            print(f"Internal error: {e}", file=sys.stderr)


# ======================================================================
# CLI argument parsing
# ======================================================================


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse CLI arguments."""
    parser = argparse.ArgumentParser(
        description="PML — Picture Markup Language interpreter",
    )
    parser.add_argument(
        "file",
        nargs="?",
        help="PML source file to execute (omit for REPL)",
    )
    parser.add_argument(
        "--output-dir", "-o",
        default=None,
        help="Output directory for rendered files (default: same directory as source)",
    )
    parser.add_argument(
        "--watch", "-w",
        action="store_true",
        help="Watch the source file for changes and re-run",
    )
    parser.add_argument(
        "--gif",
        action="store_true",
        help="Force GIF animation output (equivalent to setting format in PML)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output structured JSON result to stdout",
    )
    return parser.parse_args(argv)


def _run_with_json(
    filename: str,
    args: argparse.Namespace,
) -> dict[str, Any]:
    """Run a file and return structured result dict."""
    try:
        result = run_file(filename)
        return {
            "success": True,
            "file": os.path.abspath(filename),
            "result": repr(result) if result is not None else None,
        }
    except PMLError as e:
        err: dict[str, Any] = {
            "type": type(e).__name__,
            "message": e.pml_message if hasattr(e, "pml_message") else str(e),
        }
        if e.line is not None:
            err["line"] = e.line
        if e.column is not None:
            err["column"] = e.column
        if e.filename is not None:
            err["filename"] = e.filename
        return {"success": False, "file": os.path.abspath(filename), "error": err}
    except FileNotFoundError:
        return {
            "success": False,
            "file": os.path.abspath(filename),
            "error": {"type": "FileNotFound", "message": f"file not found: {filename}"},
        }


def _watch_file(filename: str, args: argparse.Namespace) -> None:
    """Poll file for changes and re-run on modification."""
    last_mtime = os.path.getmtime(filename)
    print(f"Watching {filename} for changes... (Ctrl-C to stop)")

    try:
        while True:
            time.sleep(0.5)
            try:
                mtime = os.path.getmtime(filename)
            except OSError:
                continue
            if mtime != last_mtime:
                last_mtime = mtime
                print(f"\n--- {time.strftime('%H:%M:%S')} ---")
                try:
                    run_file(filename)
                except Exception as e:
                    print(f"Error: {e}", file=sys.stderr)
    except KeyboardInterrupt:
        print("\nStopped.")


# ======================================================================
# CLI main
# ======================================================================


def main(argv: list[str] | None = None) -> None:
    """CLI entry point: pml [options] [file.pml]

    Without a file argument, starts an interactive REPL.
    """
    args = _parse_args(argv)

    if args.output_dir:
        set_output_dir(args.output_dir)

    if args.file:
        filename = args.file

        if args.json:
            result = _run_with_json(filename, args)
            print(json.dumps(result, indent=2, default=str))
            if not result["success"]:
                sys.exit(1)
            return

        if args.watch:
            _watch_file(filename, args)
            return

        try:
            run_file(filename)
        except PMLError as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)
        except FileNotFoundError:
            print(f"Error: file not found: {filename}", file=sys.stderr)
            sys.exit(1)
    else:
        repl()


if __name__ == "__main__":
    main()
