#!/usr/bin/env python3
"""Parity verification script — compare Python PML and C++ PML outputs.

Usage:
    python tools/parity_check.py
    python tools/parity_check.py --pml-py "cd /root/PML && uv run pml"
    python tools/parity_check.py --pml-cpp ./build2/src/pml/cli/pml
"""

from __future__ import annotations

import argparse
import ast
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from PIL import Image, ImageChops

    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    Image = None
    ImageChops = None

# ── Constants ──────────────────────────────────────────────────────────────────

PROJECT_ROOT = Path("/root/PML")
CPP_PROJECT_ROOT = Path("/root/PML/pml-cpp")
EXAMPLES_DIR = PROJECT_ROOT / "examples"
OUTPUT_DIR = Path("/tmp/pml-parity")

# Default commands (can be overridden via CLI flags)
DEFAULT_PML_PY = "cd /root/PML && uv run pml"
DEFAULT_PML_CPP = str(CPP_PROJECT_ROOT / "build2/src/pml/cli/pml")

# Known example files that render images (PNG or GIF)
IMAGE_RENDERING_EXAMPLES = {"sprite.pml", "phase4.pml", "animation.pml", "skeleton.pml"}


# ── Helpers ────────────────────────────────────────────────────────────────────


def _cleanup_tempdir() -> None:
    """Remove the temp output directory tree."""
    if OUTPUT_DIR.exists():
        shutil.rmtree(OUTPUT_DIR)


def _ensure_output_dirs() -> None:
    """Create py/ and cpp/ subdirectories under the output dir."""
    for sub in ("py", "cpp"):
        (OUTPUT_DIR / sub).mkdir(parents=True, exist_ok=True)


def _find_json_in_stdout(text: str) -> dict | None:
    """Extract the last JSON object from mixed stdout output.

    Both Python and C++ interpreters interleave println output with
    a final JSON block when ``--json`` is passed.  This function finds
    the last top-level ``{...}`` object in *text*.
    """
    # Walk backwards and find the last '}' followed by a balanced '{'
    depth = 0
    json_end = -1
    for i in range(len(text) - 1, -1, -1):
        if text[i] == "}":
            if depth == 0:
                json_end = i + 1
            depth += 1
        elif text[i] == "{":
            depth -= 1
            if depth == 0 and json_end > 0:
                candidate = text[i:json_end]
                try:
                    return json.loads(candidate)
                except json.JSONDecodeError:
                    return None
    return None


def _run_command(
    cmd: str,
    *,
    cwd: str | None = None,
    timeout: int = 60,
) -> subprocess.CompletedProcess:
    """Run a shell command and return the CompletedProcess."""
    return subprocess.run(
        cmd,
        shell=True,
        cwd=cwd,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


# ── Interpreter invocation ────────────────────────────────────────────────────


def run_py(
    source: str,
    *,
    mode: str = "file",
    output_dir: str | None = None,
    json_output: bool = False,
    pml_py_cmd: str = DEFAULT_PML_PY,
) -> subprocess.CompletedProcess:
    """Run the Python PML interpreter.

    Parameters
    ----------
    source:
        If *mode* is ``"file"``: the path to a ``.pml`` file to execute.
        If *mode* is ``"eval"``: inline PML source written to a temp file.
    mode:
        ``"file"`` or ``"eval"``.
    output_dir:
        If set, passed as ``-o <output_dir>``.
    json_output:
        If true, append ``--json``.
    pml_py_cmd:
        The shell command that invokes the Python PML interpreter.
    """
    cmd_parts = [pml_py_cmd]
    if output_dir:
        cmd_parts.extend(["-o", output_dir])
    if json_output:
        cmd_parts.append("--json")

    if mode == "eval":
        # Write source to a temp file
        fd, tmp_path = tempfile.mkstemp(suffix=".pml", prefix="pml_parity_")
        with os.fdopen(fd, "w") as f:
            f.write(source)
        try:
            cmd_parts.append(tmp_path)
            return _run_command(" ".join(cmd_parts), timeout=30)
        finally:
            os.unlink(tmp_path)
    else:
        cmd_parts.append(source)
        return _run_command(" ".join(cmd_parts), timeout=60)


def run_cpp(
    source: str,
    *,
    mode: str = "file",
    output_dir: str | None = None,
    json_output: bool = False,
    pml_cpp_cmd: str = DEFAULT_PML_CPP,
) -> subprocess.CompletedProcess:
    """Run the C++ PML interpreter.

    Parameters mirror :func:`run_py`.
    """
    cmd_parts = [pml_cpp_cmd]
    if output_dir:
        cmd_parts.extend(["-o", output_dir])
    if json_output:
        cmd_parts.append("--json")

    if mode == "eval":
        fd, tmp_path = tempfile.mkstemp(suffix=".pml", prefix="pml_parity_")
        with os.fdopen(fd, "w") as f:
            f.write(source)
        try:
            cmd_parts.append(tmp_path)
            return _run_command(" ".join(cmd_parts), timeout=30)
        finally:
            os.unlink(tmp_path)
    else:
        cmd_parts.append(source)
        return _run_command(" ".join(cmd_parts), timeout=60)


# ── Comparison helpers ────────────────────────────────────────────────────────


def compare_png(py_path: str, cpp_path: str) -> tuple[bool, str]:
    """Compare two PNG files pixel-by-pixel.

    Returns ``(ok, message)``.
    """
    if not HAS_PIL:
        return False, "Pillow not installed — cannot compare PNGs"

    if not os.path.exists(py_path):
        return False, f"Python output not found: {py_path}"
    if not os.path.exists(cpp_path):
        return False, f"C++ output not found: {cpp_path}"

    try:
        img_py = Image.open(py_path)
        img_cpp = Image.open(cpp_path)

        # Normalise modes for comparison
        if img_py.mode != img_cpp.mode:
            # Convert both to RGBA for fair comparison
            img_py = img_py.convert("RGBA")
            img_cpp = img_cpp.convert("RGBA")

        if img_py.size != img_cpp.size:
            return (
                False,
                f"size mismatch: Python {img_py.size} vs C++ {img_cpp.size}",
            )

        diff = ImageChops.difference(img_py.convert("RGB"), img_cpp.convert("RGB"))
        bbox = diff.getbbox()
        if bbox is None:
            return True, ""

        # Find the first differing pixel
        extrema = diff.getextrema()
        for y in range(diff.height):
            for x in range(diff.width):
                px = diff.getpixel((x, y))
                if any(c != 0 for c in px):
                    return False, f"mismatch at pixel ({x},{y}) (RGB diff={px})"

        return False, "unknown difference"
    except Exception as e:
        return False, f"PNG comparison error: {e}"


def compare_gif(py_path: str, cpp_path: str) -> tuple[bool, str]:
    """Compare two GIF files frame-by-frame."""
    if not HAS_PIL:
        return False, "Pillow not installed — cannot compare GIFs"

    if not os.path.exists(py_path):
        return False, f"Python output not found: {py_path}"
    if not os.path.exists(cpp_path):
        return False, f"C++ output not found: {cpp_path}"

    try:
        gif_py = Image.open(py_path)
        gif_cpp = Image.open(cpp_path)

        # Count frames
        frames_py = 0
        while True:
            try:
                gif_py.seek(frames_py)
                frames_py += 1
            except EOFError:
                break

        frames_cpp = 0
        while True:
            try:
                gif_cpp.seek(frames_cpp)
                frames_cpp += 1
            except EOFError:
                break

        if frames_py != frames_cpp:
            return (
                False,
                f"frame count mismatch: Python {frames_py} vs C++ {frames_cpp}",
            )

        # Compare each frame
        for i in range(frames_py):
            gif_py.seek(i)
            gif_cpp.seek(i)
            frame_py = gif_py.convert("RGBA")
            frame_cpp = gif_cpp.convert("RGBA")

            if frame_py.size != frame_cpp.size:
                return (
                    False,
                    f"frame {i} size mismatch: Python {frame_py.size} vs C++ {frame_cpp.size}",
                )

            diff = ImageChops.difference(
                frame_py.convert("RGB"), frame_cpp.convert("RGB")
            )
            bbox = diff.getbbox()
            if bbox is not None:
                for y in range(diff.height):
                    for x in range(diff.width):
                        px = diff.getpixel((x, y))
                        if any(c != 0 for c in px):
                            return (
                                False,
                                f"frame {i} mismatch at pixel ({x},{y}) (RGB diff={px})",
                            )

        return True, ""
    except Exception as e:
        return False, f"GIF comparison error: {e}"


def _normalise_stdout(text: str) -> str:
    """Normalise stdout text for comparison.

    Strips trailing whitespace, normalises float formatting, etc.
    """
    # Strip leading/trailing whitespace
    text = text.strip()
    # Normalise multiple blank lines
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text


def _normalise_float_literals(text: str) -> str:
    """Normalise float representation to handle precision differences.

    Python may print 3.14159265358979 while C++ prints 3.141593.
    We truncate all floats to 6 decimal places for comparison.
    """
    def _truncate(m: re.Match) -> str:
        val = float(m.group())
        return f"{val:.6f}"

    return re.sub(r"\b\d+\.\d+\b", _truncate, text)


def _parse_py_repr(repr_str: str):
    """Parse a Python ``repr()`` string back into a Python value.

    The Python ``--json`` output stores result values via ``repr()``,
    so ``3`` becomes ``"3"`` and ``"hello"`` becomes ``"\"hello\""``.
    This function uses ``ast.literal_eval`` to recover the original value.
    """
    if not isinstance(repr_str, str):
        return repr_str
    try:
        return ast.literal_eval(repr_str)
    except (ValueError, SyntaxError, MemoryError):
        # Not parseable; return as-is
        return repr_str


def _compare_values(py_val, cpp_val) -> tuple[bool, str]:
    """Compare two values with appropriate type coercion and tolerance.

    Python stores results as repr-strings; C++ stores native JSON types.
    This function normalises and compares them fairly.
    """
    # Parse Python repr-string back to native value
    py_parsed = _parse_py_repr(py_val)

    # If both are numeric, compare with tolerance
    if isinstance(py_parsed, (int, float)) and isinstance(cpp_val, (int, float)):
        py_f = float(py_parsed)
        cpp_f = float(cpp_val)
        if abs(py_f - cpp_f) < 1e-9:
            return True, ""
        return False, f"Python={py_parsed!r} vs C++={cpp_val!r}"

    # String comparison
    if isinstance(py_parsed, str) and isinstance(cpp_val, str):
        if py_parsed == cpp_val:
            return True, ""
        return False, f"Python={py_parsed!r} vs C++={cpp_val!r}"

    # Direct comparison
    if py_parsed == cpp_val:
        return True, ""

    # Last resort: string repr comparison
    if repr(py_parsed) == repr(cpp_val):
        return True, ""

    return False, f"Python={py_parsed!r} vs C++={cpp_val!r}"


# ── Test cases ─────────────────────────────────────────────────────────────────


def test_example_file(
    filename: str,
    pml_py_cmd: str,
    pml_cpp_cmd: str,
) -> tuple[bool, str]:
    """Run an example ``.pml`` file through both interpreters and compare.

    Returns ``(passed, detail_message)``.
    """
    filepath = EXAMPLES_DIR / filename
    if not filepath.exists():
        return False, f"file not found: {filepath}"

    # Create isolated output directories for this file
    safe_name = filename.replace(".", "_")
    py_out = str(OUTPUT_DIR / "py" / safe_name)
    cpp_out = str(OUTPUT_DIR / "cpp" / safe_name)
    os.makedirs(py_out, exist_ok=True)
    os.makedirs(cpp_out, exist_ok=True)

    # Run both interpreters
    proc_py = run_py(
        str(filepath),
        mode="file",
        output_dir=py_out,
        json_output=False,
        pml_py_cmd=pml_py_cmd,
    )
    proc_cpp = run_cpp(
        str(filepath),
        mode="file",
        output_dir=cpp_out,
        json_output=False,
        pml_cpp_cmd=pml_cpp_cmd,
    )

    # Check exit codes
    py_ok = proc_py.returncode == 0
    cpp_ok = proc_cpp.returncode == 0

    if py_ok != cpp_ok:
        status = ""
        if py_ok:
            status = "Python OK, C++ FAILED"
        else:
            status = "Python FAILED, C++ OK"
        detail = (
            f"exit code mismatch: {status}\n"
            f"  Python stderr: {proc_py.stderr.strip() or '(none)'}\n"
            f"  C++    stderr: {proc_cpp.stderr.strip() or '(none)'}"
        )
        return False, detail

    # Compare stdout
    py_stdout = _normalise_stdout(proc_py.stdout)
    cpp_stdout = _normalise_stdout(proc_cpp.stdout)

    if py_stdout != cpp_stdout:
        # Try with float normalisation
        py_norm = _normalise_float_literals(py_stdout)
        cpp_norm = _normalise_float_literals(cpp_stdout)
        if py_norm != cpp_norm:
            return (
                False,
                f"stdout mismatch\n"
                f"  Python:\n{_indent(py_stdout[:2000], '    ')}\n"
                f"  C++:\n{_indent(cpp_stdout[:2000], '    ')}",
            )

    # Compare images if applicable (only image files, skip .meta.json etc.)
    if filename in IMAGE_RENDERING_EXAMPLES:
        py_files = sorted(Path(py_out).glob("*.*"))
        cpp_files = sorted(Path(cpp_out).glob("*.*"))

        # Only compare image files
        IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif"}
        py_images = {p.name for p in py_files if p.suffix.lower() in IMAGE_EXTS}
        cpp_images = {p.name for p in cpp_files if p.suffix.lower() in IMAGE_EXTS}

        all_images = py_images | cpp_images
        for name in sorted(all_images):
            if name not in py_images:
                return False, f"Python missing image: {name}"
            if name not in cpp_images:
                return False, f"C++ missing image: {name}"

            py_path = Path(py_out) / name
            cpp_path = Path(cpp_out) / name

            if name.lower().endswith(".gif"):
                ok, msg = compare_gif(str(py_path), str(cpp_path))
            else:
                ok, msg = compare_png(str(py_path), str(cpp_path))

            if not ok:
                return False, f"image mismatch ({name}): {msg}"

    return True, ""


def _indent(text: str, prefix: str) -> str:
    """Indent each line of *text* with *prefix*."""
    return "\n".join(f"{prefix}{line}" for line in text.splitlines())


def test_arithmetic(
    expr: str,
    expected_value,
    pml_py_cmd: str,
    pml_cpp_cmd: str,
) -> tuple[bool, str]:
    """Evaluate an arithmetic expression in both interpreters.

    Uses ``--json`` mode and compares the ``value`` field.
    """
    proc_py = run_py(
        expr,
        mode="eval",
        json_output=True,
        pml_py_cmd=pml_py_cmd,
    )
    proc_cpp = run_cpp(
        expr,
        mode="eval",
        json_output=True,
        pml_cpp_cmd=pml_cpp_cmd,
    )

    # Python: extract JSON from mixed stdout
    py_json = _find_json_in_stdout(proc_py.stdout)
    cpp_json = _find_json_in_stdout(proc_cpp.stdout)

    if py_json is None:
        return False, f"Python produced no valid JSON (stdout={proc_py.stdout!r})"
    if cpp_json is None:
        return False, f"C++ produced no valid JSON (stdout={proc_cpp.stdout!r})"

    # Both should succeed
    py_success = py_json.get("success", False)
    cpp_success = cpp_json.get("success", False)

    if not py_success:
        err = py_json.get("error", {})
        return (
            False,
            f"Python execution failed: {err.get('type', '?')}: {err.get('message', '?')}",
        )
    if not cpp_success:
        err = cpp_json.get("error", {})
        return (
            False,
            f"C++ execution failed: {err.get('type', '?')}: {err.get('message', '?')}",
        )

    # Compare values (Python stores as repr-string, C++ stores native JSON)
    py_value = py_json.get("result")
    cpp_value = cpp_json.get("value")

    ok, detail = _compare_values(py_value, cpp_value)
    if not ok:
        return False, f"value mismatch: Python={py_value!r} vs C++={cpp_value!r}"

    # Check against expected value
    py_parsed = _parse_py_repr(py_value)
    if expected_value is not None:
        ok_py, _ = _compare_values(py_parsed, expected_value)
        if not ok_py:
            return (
                False,
                f"unexpected Python value: got {py_parsed!r}, expected {expected_value!r}",
            )
        ok_cpp, _ = _compare_values(cpp_value, expected_value)
        if not ok_cpp:
            return (
                False,
                f"unexpected C++ value: got {cpp_value!r}, expected {expected_value!r}",
            )

    return True, ""


def test_error(
    source: str,
    description: str,
    pml_py_cmd: str,
    pml_cpp_cmd: str,
) -> tuple[bool, str]:
    """Verify that both interpreters produce an error for invalid source.

    Returns ``(passed, detail)``.
    """
    proc_py = run_py(
        source,
        mode="eval",
        json_output=True,
        pml_py_cmd=pml_py_cmd,
    )
    proc_cpp = run_cpp(
        source,
        mode="eval",
        json_output=True,
        pml_cpp_cmd=pml_cpp_cmd,
    )

    py_json = _find_json_in_stdout(proc_py.stdout)
    cpp_json = _find_json_in_stdout(proc_cpp.stdout)

    # Python: some errors crash without producing JSON (e.g. bare TypeError).
    # Treat missing JSON as "error produced" too.
    py_errored = (py_json is None) or (py_json.get("success", True) is False)
    cpp_errored = (cpp_json is None) or (cpp_json.get("success", True) is False)

    if not py_errored:
        return False, "Python did NOT produce an error (it succeeded)"
    if not cpp_errored:
        return False, "C++ did NOT produce an error (it succeeded)"

    # Both errored — success
    return True, ""


# ── Main ───────────────────────────────────────────────────────────────────────


def build_test_suite(pml_py_cmd: str, pml_cpp_cmd: str) -> list[tuple]:
    """Build the list of test cases.

    Each test is a ``(name, callable)`` tuple.  The callable
    returns ``(bool, str)``.
    """

    def _make_example_test(filename: str):
        return (
            f"example:{filename}",
            lambda f=filename: test_example_file(f, pml_py_cmd, pml_cpp_cmd),
        )

    def _make_arithmetic_test(expr: str, expected, label: str | None = None):
        name = label or f"arithmetic:{expr}"
        return (
            name,
            lambda e=expr, ev=expected: test_arithmetic(e, ev, pml_py_cmd, pml_cpp_cmd),
        )

    def _make_error_test(source: str, desc: str):
        return (
            f"error:{desc}",
            lambda s=source, d=desc: test_error(s, d, pml_py_cmd, pml_cpp_cmd),
        )

    tests = []

    # 1. Example files
    example_files = sorted(
        f.name for f in sorted(EXAMPLES_DIR.iterdir()) if f.suffix == ".pml"
    )
    for fname in example_files:
        tests.append(_make_example_test(fname))

    # 2. Arithmetic
    tests.append(_make_arithmetic_test("(+ 1 2)", 3))
    tests.append(_make_arithmetic_test("(* 6 7)", 42))
    tests.append(_make_arithmetic_test("(- 10 4)", 6))
    tests.append(_make_arithmetic_test("(/ 15 3)", 5))
    tests.append(_make_arithmetic_test("(+ 1 (+ 2 3))", 6, "arithmetic:nested (+ 1 (+ 2 3))"))

    # 3. Error handling
    tests.append(_make_error_test("(+ 1", "syntax unmatched paren"))
    tests.append(
        _make_error_test('(+ 1 "hello")', "type error number+string")
    )
    tests.append(_make_error_test("foobar", "unbound variable"))

    return tests


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Parity verification script — compare Python PML and C++ PML outputs.",
    )
    parser.add_argument(
        "--pml-py",
        default=DEFAULT_PML_PY,
        help=f"Command to run the Python PML interpreter (default: {DEFAULT_PML_PY})",
    )
    parser.add_argument(
        "--pml-cpp",
        default=DEFAULT_PML_CPP,
        help=f"Path to the C++ PML binary (default: {DEFAULT_PML_CPP})",
    )
    parser.add_argument(
        "--no-png",
        action="store_true",
        help="Skip PNG/GIF comparison (useful when Pillow is unavailable)",
    )
    parser.add_argument(
        "--no-cleanup",
        action="store_true",
        help="Keep output files in /tmp/pml-parity/ after running",
    )
    args = parser.parse_args()

    pml_py_cmd = args.pml_py
    pml_cpp_cmd = args.pml_cpp

    # Pre-flight checks
    if not HAS_PIL and not args.no_png:
        print(
            "Warning: Pillow is not installed. PNG/GIF comparison will be skipped.",
            file=sys.stderr,
        )
        print(
            "  Install with: pip install Pillow",
            file=sys.stderr,
        )

    # Ensure temp directories
    _ensure_output_dirs()

    # Build and run tests
    tests = build_test_suite(pml_py_cmd, pml_cpp_cmd)

    passed = 0
    failed = 0
    errors = 0

    for name, test_fn in tests:
        # Check if we should skip PNG tests
        if args.no_png and name.startswith("example:") and "sprite" in name:
            # Still run but without image comparison — handled inside test
            pass

        try:
            ok, detail = test_fn()
        except Exception as e:
            print(f"  [ERROR] {name}: exception: {e}")
            errors += 1
            continue

        if ok:
            print(f"  [PASS] {name}")
            passed += 1
        else:
            print(f"  [FAIL] {name}")
            if detail:
                for line in detail.split("\n"):
                    print(f"         {line}")
            failed += 1

    # Summary
    total = passed + failed + errors
    print(f"\nResults: {passed} passed, {failed} failed, {errors} errors")
    print(f"         ({total} total)")

    if not args.no_cleanup:
        _cleanup_tempdir()

    return 0 if failed == 0 and errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
