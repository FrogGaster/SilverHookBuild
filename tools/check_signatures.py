#!/usr/bin/env python3
"""Check ZA_PUTINA byte signatures against an installed hoi4.exe."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


PATTERN_CALL = re.compile(
    r'(?:FindPattern\(\s*const_cast\s*<char\s*\*>\(|FIND_PATTERN\()'
    r'"((?:\\x[0-9A-Fa-f]{2})+)"\)?,\s*'
    r'(?:const_cast\s*<char\s*\*>\()?"([x?]+)"\)?\s*\)'
)


def steam_library_paths() -> list[Path]:
    try:
        import winreg

        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
            steam_path = Path(winreg.QueryValueEx(key, "SteamPath")[0])
    except (ImportError, FileNotFoundError, OSError):
        return []

    paths = [steam_path]
    library_file = steam_path / "steamapps" / "libraryfolders.vdf"
    if library_file.is_file():
        text = library_file.read_text(encoding="utf-8", errors="replace")
        for value in re.findall(r'"path"\s+"([^"]+)"', text):
            paths.append(Path(value.replace(r"\\", "\\")))
    return list(dict.fromkeys(paths))


def find_game() -> Path | None:
    for library in steam_library_paths():
        candidate = library / "steamapps" / "common" / "Hearts of Iron IV" / "hoi4.exe"
        if candidate.is_file():
            return candidate
    return None


def signature_regex(pattern: bytes, mask: str) -> re.Pattern[bytes]:
    parts = [re.escape(bytes((value,))) if marker == "x" else b"." for value, marker in zip(pattern, mask)]
    return re.compile(b"".join(parts), re.DOTALL)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_exe", nargs="?", type=Path, help="Path to hoi4.exe (auto-detected when omitted)")
    args = parser.parse_args()

    game_exe = args.game_exe or find_game()
    if not game_exe or not game_exe.is_file():
        print("hoi4.exe was not found; pass its full path explicitly.", file=sys.stderr)
        return 1

    root = Path(__file__).resolve().parents[1]
    source_path = root / "ImGui DirectX 11 Kiero Hook" / "main.cpp"
    source = source_path.read_text(encoding="utf-8-sig")
    image = game_exe.read_bytes()

    total = 0
    missing = 0
    for match in PATTERN_CALL.finditer(source):
        line_number = source.count("\n", 0, match.start()) + 1
        line_start = source.rfind("\n", 0, match.start()) + 1
        line_prefix = source[line_start:match.start()]
        if "//" in line_prefix or source.rfind("/*", 0, match.start()) > source.rfind("*/", 0, match.start()):
            continue

        total += 1
        raw_pattern, mask = match.groups()
        pattern = bytes.fromhex(raw_pattern.replace(r"\x", ""))
        assignment = re.search(r"([A-Za-z_]\w*)\s*=", line_prefix)
        name = assignment.group(1) if assignment else f"line_{line_number}"

        if len(pattern) != len(mask):
            print(f"[INVALID] {name} (line {line_number}): pattern/mask length mismatch")
            missing += 1
            continue

        found = signature_regex(pattern, mask).search(image)
        if found:
            print(f"[OK]      {name:<28} file offset 0x{found.start():X}")
        else:
            print(f"[MISSING] {name:<28} source line {line_number}")
            missing += 1

    print(f"\nChecked {total} signatures in: {game_exe}")
    print(f"Found: {total - missing}; missing/invalid: {missing}")
    return 2 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
