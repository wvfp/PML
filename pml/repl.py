"""PML REPL and CLI entry point."""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path
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

    # Register character animation builtins (blink, idle-breath, walk-cycle)
    from pml.sprites.character_animation import register_character_animation
    register_character_animation(env)

    # Register template subsystem
    from pml.templates import register_templates
    register_templates(env)

    # Register shader subsystem
    from pml.shaders import register_shaders
    register_shaders(env)

    # Register hand-drawn / sketch subsystem
    from pml.graphics.builtins_sketch import register_sketch_builtins
    register_sketch_builtins(env)

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
# Project subcommand dispatch
# ======================================================================


def _build_subcommand_parsers() -> dict[str, argparse.ArgumentParser]:
    """Build per-subcommand parsers with proper --help."""
    parsers: dict[str, argparse.ArgumentParser] = {}

    # render
    p = argparse.ArgumentParser(description="Render all project targets from pml.toml")
    p.add_argument("--watch", "-w", action="store_true", help="Watch files and re-render on change")
    p.add_argument("--parallel", "-P", action="store_true", help="Render independent targets in parallel")
    parsers["render"] = p

    # new
    p = argparse.ArgumentParser(description="Scaffold a new PML project")
    p.add_argument("name", help="Project name")
    p.add_argument("dest", nargs="?", default=None, help="Target directory (default: <name>)")
    parsers["new"] = p

    # clean
    p = argparse.ArgumentParser(description="Remove the out/ directory and cache")
    parsers["clean"] = p

    # run
    p = argparse.ArgumentParser(description="Render all targets and open outputs with system viewer")
    parsers["run"] = p

    # check
    p = argparse.ArgumentParser(description="Validate .pml syntax without rendering")
    parsers["check"] = p

    return parsers


_SUBCOMMAND_PARSERS = _build_subcommand_parsers()
_SUBCOMMANDS = frozenset(_SUBCOMMAND_PARSERS.keys())


def _load_project() -> Any:
    """Load pml.toml from CWD, exit with error if not found."""
    from pml.project import ProjectConfig, load_project_config
    config = load_project_config(os.getcwd())
    if config is None:
        print("Error: no pml.toml found in current directory", file=sys.stderr)
        sys.exit(1)
    return config


def _print_results(
    results: list[dict[str, Any]],
    *,
    label: str = "targets",
) -> None:
    success = sum(1 for r in results if r.get("success"))
    failed = sum(1 for r in results if not r.get("success"))
    total = len(results)
    if total == 0:
        print(f"No {label} to process.")
        return
    print(f"\n{'=' * 40}")
    print(f"  {success}/{total} {label} completed successfully")
    if failed:
        for r in results:
            if not r.get("success"):
                src = r.get("src") or r.get("workspace_member", "?")
                print(f"  FAIL  {src}: {r.get('error')}", file=sys.stderr)
        print(f"{'=' * 40}")


def _subcommand_render(argv: list[str]) -> None:
    ns = _SUBCOMMAND_PARSERS["render"].parse_args(argv)
    from pml.project import PMLProject

    project = PMLProject(_load_project())
    if ns.watch:
        project.watch()
    else:
        results = project.render_all(parallel=ns.parallel)
        _print_results(results)
        if not all(r.get("success") for r in results):
            sys.exit(1)


def _subcommand_new(argv: list[str]) -> None:
    ns = _SUBCOMMAND_PARSERS["new"].parse_args(argv)
    from pml.project import scaffold_project
    scaffold_project(ns.name, ns.dest)


def _subcommand_clean(argv: list[str]) -> None:
    _SUBCOMMAND_PARSERS["clean"].parse_args(argv)
    from pml.project import PMLProject

    project = PMLProject(_load_project())
    removed = project.clean()
    project_name = Path(project.config.project_dir).name
    if removed:
        print(f"Cleaned {project_name}: {len(removed)} file(s) removed")
    else:
        print(f"{project_name}: nothing to clean")


def _subcommand_run(argv: list[str]) -> None:
    _SUBCOMMAND_PARSERS["run"].parse_args(argv)
    from pml.project import PMLProject

    PMLProject(_load_project()).run()


def _subcommand_check(argv: list[str]) -> None:
    _SUBCOMMAND_PARSERS["check"].parse_args(argv)
    from pml.project import PMLProject

    project = PMLProject(_load_project())
    results = project.check()
    _print_results(results, label="files")
    if not all(r.get("success") for r in results):
        sys.exit(1)


# ======================================================================
# CLI main
# ======================================================================


def main(argv: list[str] | None = None) -> None:
    """CLI entry point.

    Usage:
      pml [options] [file.pml]   — execute file or start REPL
      pml new <name>             — scaffold a new project
      pml render [--watch]       — render project targets
      pml clean                  — remove out/
      pml run                    — render + open outputs
      pml check                  — validate .pml files
    """
    if argv is None:
        argv = sys.argv[1:]

    # Subcommand dispatch
    if argv and argv[0] in _SUBCOMMANDS:
        cmd = argv[0]
        rest = argv[1:]
        if cmd == "render":
            _subcommand_render(rest)
        elif cmd == "new":
            _subcommand_new(rest)
        elif cmd == "clean":
            _subcommand_clean(rest)
        elif cmd == "run":
            _subcommand_run(rest)
        elif cmd == "check":
            _subcommand_check(rest)
        return

    # Legacy: file execution or REPL
    args = _parse_args(argv if argv else None)

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
