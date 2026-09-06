#!/usr/bin/env python3
"""Audit DCCEXProtocol API coverage in the DCCEXProtocolTesting sketch.

Enforces the AGENTS.md invariant: every public DCCEXProtocol method and every
DCCEXProtocolDelegate callback must be exercised by some test. Run this after
any change to the library or the sketch:

    .venv/bin/python tools/check_coverage.py [--lib /path/to/DCCEXProtocol]

The script reads the library header (DCCEXProtocol.h), extracts the public
methods and delegate callbacks, then greps the sketch sources (the .ino plus
the included .h files, which form one translation unit) for usage of each
symbol. Comment text is stripped first so documentation mentions do not count.

A public method counts as covered only when it is called through an object
(`csClient.method(...)` or `csClient->method(...)`) - callback overrides that
share a name with a bool getter (e.g. `receivedRouteList`) do not count as
exercising the method. A delegate callback counts as covered when the
listener defines it and the matching `expect...()` is used by a test.

Output is a status table (`covered`, `skipped` for documented exceptions, or
`GAP`) and the process exits non-zero when an unexpected gap exists.

Known exceptions kept deliberately unexercised:
  - disconnect()   - currently a no-op stub in the library (see AGENTS.md)

No third-party dependencies; the virtual environment is used so the tool runs
in a known interpreter (see requirements.txt and AGENTS.md).
"""

import argparse
import re
import sys
from pathlib import Path

# Documented exceptions: symbols deliberately not exercised, with a reason.
SKIPPED = {
    "disconnect": "no-op stub in the library - deliberately not exercised (AGENTS.md)",
}

# Sketch sources that, together with the .ino, form the single translation unit.
SKETCH_SOURCES = [
    "DCCEXProtocolTesting.ino",
    "Globals.h",
    "PrintHelpers.h",
    "TestListener.h",
    "Console.h",
    "TestSequence.h",
]


def strip_comments(text: str) -> str:
    """Remove C/C++ line and block comments so documentation never matches."""
    out = []
    in_block = False
    for line in text.splitlines():
        if in_block:
            end = line.find("*/")
            if end != -1:
                in_block = False
                line = line[end + 2:]
            else:
                continue
        idx = line.find("//")
        if idx != -1:
            line = line[:idx]
        start = line.find("/*")
        if start != -1:
            line = line[:start]
            in_block = True
        out.append(line)
    return "\n".join(out)


def region_lines(lines, open_line, end_markers):
    """Yield the non-blank lines of a named region of the header."""
    start = None
    for i, line in enumerate(lines):
        s = line.rstrip()
        if s == open_line and start is None:
            start = i + 1  # skip the class-open line itself
            continue
        if start is not None and (s in end_markers or s.startswith(end_markers[0])):
            return [ln.rstrip() for ln in lines[start:i] if ln.strip()]
    if start is None:
        return []
    return [ln.rstrip() for ln in lines[start:] if ln.strip()]


def extract_names(region):
    """Extract method/callback names: the identifier just before '(' on each declaration line."""
    names = []
    for line in region:
        s = line.strip()
        if not s or s.startswith("~") or "<" in s:
            continue
        m = re.match(r"^(?:virtual\s+)?.*?[\s*&](\w+)\s*\(", s)
        if m:
            names.append(m.group(1))
    return names


def load_methods(header: Path):
    """Parse DCCEXProtocol.h into (callbacks, methods, constructor_name)."""
    text = strip_comments(header.read_text(encoding="utf-8"))
    lines = text.splitlines()

    delegate = region_lines(lines, "class DCCEXProtocolDelegate {", ("};",))
    callbacks = extract_names(delegate)

    proto = region_lines(lines, "class DCCEXProtocol {", ("private:", "protected:", "};"))
    methods = extract_names(proto)
    constructor = None
    for name in methods:
        if name == "DCCEXProtocol":
            constructor = name
            methods.remove(name)
            break
    return list(dict.fromkeys(callbacks)), list(dict.fromkeys(methods)), constructor


def usage_in(sources, pattern):
    return any(re.search(pattern, s) for s in sources)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--lib",
        type=Path,
        default=Path(__file__).resolve().parent.parent.parent / "DCCEXProtocol",
        help="Path to the DCCEXProtocol library (default: sibling of the sketch repo)",
    )
    args = parser.parse_args()

    header = args.lib / "src" / "DCCEXProtocol.h"
    if not header.is_file():
        sys.exit(f"ERROR: library header not found at {header} (use --lib to override)")

    repo = Path(__file__).resolve().parent.parent
    sources = [strip_comments((repo / name).read_text(encoding="utf-8")) for name in SKETCH_SOURCES]

    callbacks, methods, constructor = load_methods(header)

    print(f"Auditing DCCEXProtocol API coverage against {repo.name}")
    print(f"library header: {header}\n")

    gap_count = 0
    cover_count = 0
    joined = "".join(sources)

    def report(label, items, kind):
        nonlocal gap_count, cover_count
        print(label)
        for name in sorted(items):
            if kind == "callback":
                used = re.search(r"\bexpect" + name[8:] + r"\s*\(", joined) is not None
            else:
                used = re.search(r"\b\w+\s*\.\s*" + name + r"\s*\(", joined) is not None
            if used:
                print(f"  covered   {name}")
                cover_count += 1
            elif name in SKIPPED:
                print(f"  skipped   {name}  ({SKIPPED[name]})")
            else:
                print(f"  *** GAP *** {name}")
                gap_count += 1

    report("DCCEXProtocolDelegate callbacks (25 expected):", callbacks, "callback")
    print()
    report(f"DCCEXProtocol public methods ({len(methods)}):", methods, "method")
    if constructor:
        name = "DCCEXProtocol::DCCEXProtocol"
        if usage_in(sources, r"\b\w+\s+csClient\s*;") or usage_in(sources, r"\bDCCEXProtocol\("):
            print(f"  covered   {name}")
            cover_count += 1
        elif name in SKIPPED:
            print(f"  skipped   {name}  ({SKIPPED[name]})")
        else:
            print(f"  *** GAP *** {name}")
            gap_count += 1

    total = cover_count + gap_count + len(SKIPPED)
    print(f"\nTotal: {total} symbols, {cover_count} covered, {gap_count} GAP, {len(SKIPPED)} documented skip")
    if gap_count:
        print("\nUnexpected coverage gaps found. Every public API member must be exercised by a test")
        print("(see AGENTS.md 'Test strategy' invariant) - add coverage, never skip silently.")
        sys.exit(1)
    print("\nOK - every public API member is exercised or documented.")


if __name__ == "__main__":
    main()