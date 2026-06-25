#!/usr/bin/env python3
"""
extract_stdlib.py — Extract stdlib .pml files from the embedded C++ header.

This reverses the embedding done by embed_stdlib.py, recovering the original
.pml source files from src/pml/core/embedded_stdlib.h.

Usage:
    python tools/extract_stdlib.py
    (writes files to stdlib/ using the paths recorded in the header)
"""

import os
import re
import sys
from pathlib import Path


HEADER_PATH = Path("src/pml/core/embedded_stdlib.h")
STDLIB_DIR = Path("stdlib")


def unescape_hex(data: str) -> bytes:
    """Decode a C++ hex-escaped string literal like "\\x3b\\x3b" into bytes."""
    # Remove surrounding quotes
    data = data.strip().strip('"')
    # Parse hex escapes
    result = bytearray()
    i = 0
    while i < len(data):
        if data[i : i + 2] == "\\x" and i + 4 <= len(data):
            result.append(int(data[i + 2 : i + 4], 16))
            i += 4
        else:
            i += 1
    return bytes(result)


def extract_all() -> int:
    """Extract all .pml files from embedded_stdlib.h. Returns count of files written."""
    content = HEADER_PATH.read_text(encoding="utf-8")

    # Pattern: "// some/path.pml (N bytes)" followed by "constexpr const char var[N] = ..."
    file_pattern = re.compile(
        r'//\s+(\S+\.pml)\s+\((\d+)\s+bytes\)\s*\n'
        r'constexpr\s+const\s+char\s+\w+\[\d+\]\s*=\s*((?:"[^"]*"\s*)+)',
        re.MULTILINE,
    )

    count = 0
    for match in file_pattern.finditer(content):
        rel_path = match.group(1)
        size = int(match.group(2))
        hex_literals = match.group(3)

        # Join all string literals and unescape
        data = unescape_hex(hex_literals)

        # Trim null terminator if present
        if len(data) == size + 1 and data[-1] == 0:
            data = data[:size]

        out_path = STDLIB_DIR / rel_path
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_bytes(data)
        count += 1
        print(f"  {rel_path} ({len(data)} bytes)")

    return count


def main() -> None:
    if not HEADER_PATH.is_file():
        print(f"Error: {HEADER_PATH} not found. Run from repo root.", file=sys.stderr)
        sys.exit(1)

    STDLIB_DIR.mkdir(parents=True, exist_ok=True)
    count = extract_all()
    print(f"\nExtracted {count} files to {STDLIB_DIR}/")


if __name__ == "__main__":
    main()
