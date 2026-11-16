import sys
sys.dont_write_bytecode = True # importing must not leave __pycache__

import argparse
import hashlib
import os
import re
from pathlib import Path


"""
Func: check output/selected.h, the list of melodies compiled into the program (included by audio.h).
Usage:
    python source/check.py
    python source/check.py output
Rules (errors):
    selected.h exists and has its include guard
    every #include "x.h" is a file in the output dir (not selected.h, not outside), listed once
    every included file was made by the current convert.py: same FORMAT_VERSION
    every included file defines struct melody::{file name}, getPath() and, unless made for the mac only,
    its data: getNotes() (notes, rhythm) or getSamples() (pcm)
    the source audio of an included file (recorded path + sha256) is unchanged, if it still exists
Warnings:
    the source audio of an included file is gone (cannot tell if it changed)
    a .h in the output dir is not selected (fine, it just cannot be used)
selected.h is edited by hand (remove what you do not use), convert.py adds what it makes.
"""


FORMAT_VERSION = 2 # 2: target, kind, encoding, count, durationMs, rate, rhythmHz, dataSize, getPath(), getNotes() / getSamples() (lib/audio/format.h)
NAMESPACE = "melody"
SELECTED_NAME = "selected.h"
SELECTED_GUARD = "MELODY_SELECTED_H"
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/audio

INCLUDE_PATTERN = re.compile(r'^\s*#include\s+"([^"]+)"', re.MULTILINE)
FORMAT_PATTERN = re.compile(r"^// format: (\d+)$", re.MULTILINE)
SOURCE_PATTERN = re.compile(r"^// source: (.+) sha256 ([0-9a-f]{64})$", re.MULTILINE)


def get_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def get_source_str(source_path: Path) -> str:
    # recorded relative to lib/audio when possible, so the project can move
    source_path = source_path.resolve()
    try:
        shown = Path(os.path.relpath(source_path, LIB_DIR_PATH)).as_posix()
    except ValueError:
        shown = source_path.as_posix()
    if shown.startswith(".."):
        shown = source_path.as_posix()
    return f"// source: {shown} sha256 {get_sha256(source_path)}\n"


def get_format_str() -> str:
    return f"// format: {FORMAT_VERSION}\n"


def get_new_selected_str() -> str:
    selected_str = ""
    selected_str += f"#ifndef {SELECTED_GUARD}\n"
    selected_str += f"#define {SELECTED_GUARD}\n\n"
    selected_str += "// melodies compiled into the program, included by audio.h\n"
    selected_str += "// edit by hand (remove what you do not use), convert.py adds what it makes,\n"
    selected_str += "// python source/check.py checks every include\n"
    selected_str += "\n#endif\n"
    return selected_str


def add_selected(output_dir_path: Path, header_path: Path) -> bool:
    # adds #include "header" to selected.h (made if missing), keeps everything else; False if already there
    selected_path = output_dir_path/SELECTED_NAME
    if not selected_path.exists():
        selected_path.write_text(get_new_selected_str())
    selected_str = selected_path.read_text()
    include_path = Path(os.path.relpath(header_path, output_dir_path)).as_posix()
    if include_path in INCLUDE_PATTERN.findall(selected_str):
        return False

    lines = selected_str.split("\n")
    include_indexes = [i for i, line in enumerate(lines) if INCLUDE_PATTERN.match(line)]
    if include_indexes:
        insert_index = include_indexes[-1] + 1
    else:
        # before the blank line above the last #endif
        endif_index = max(i for i, line in enumerate(lines) if line.startswith("#endif"))
        insert_index = endif_index - 1 if endif_index > 0 and lines[endif_index - 1] == "" else endif_index
    lines.insert(insert_index, f"#include \"{include_path}\"")
    selected_path.write_text("\n".join(lines))
    return True


def check_header(header_path: Path) -> tuple[list[str], list[str]]:
    errors = []
    warnings = []
    name = header_path.stem
    header_str = header_path.read_text()

    format_match = FORMAT_PATTERN.search(header_str)
    if not format_match:
        errors.append(f"{header_path.name}: made by an old convert.py (no format line), regenerate it")
    elif int(format_match.group(1)) != FORMAT_VERSION:
        errors.append(f"{header_path.name}: format {format_match.group(1)}, convert.py now makes {FORMAT_VERSION}, regenerate it")

    if not re.search(rf"^namespace {NAMESPACE} \{{", header_str, re.MULTILINE):
        errors.append(f"{header_path.name}: no namespace {NAMESPACE}")
    if not re.search(rf"^\s*struct {re.escape(name)} \{{", header_str, re.MULTILINE):
        errors.append(f"{header_path.name}: no struct {NAMESPACE}::{name} (file renamed?)")

    source_match = SOURCE_PATTERN.search(header_str)
    if not source_match:
        errors.append(f"{header_path.name}: no source line, regenerate it")
    else:
        source_path = Path(source_match.group(1))
        if not source_path.is_absolute():
            source_path = LIB_DIR_PATH/source_path
        if not source_path.exists():
            warnings.append(f"{header_path.name}: source {source_match.group(1)} not found, cannot check if it changed")
        elif get_sha256(source_path) != source_match.group(2):
            errors.append(f"{header_path.name}: source {source_match.group(1)} changed since, regenerate it")

    if "getPath()" not in header_str:
        errors.append(f"{header_path.name}: no getPath(), regenerate it")
    target_match = re.search(r"static constexpr uint8_t target = (\d+);", header_str)
    kind_match = re.search(r"static constexpr uint8_t kind = (\d+);", header_str)
    if not target_match or not kind_match:
        errors.append(f"{header_path.name}: no target / kind, regenerate it")
    elif int(target_match.group(1)) & 1:
        data_name = "getSamples()" if int(kind_match.group(1)) == 2 else "getNotes()"
        if data_name not in header_str:
            errors.append(f"{header_path.name}: made for the UNO but no {data_name}, regenerate it")
    return errors, warnings


def check_selected(output_dir_path: Path) -> tuple[list[str], list[str], list[str]]:
    # returns (errors, warnings, selected file names)
    output_dir_path = output_dir_path.resolve()
    selected_path = output_dir_path/SELECTED_NAME
    if not selected_path.exists():
        return [f"{selected_path} not found, run convert.py once"], [], []

    errors = []
    warnings = []
    selected_str = selected_path.read_text()
    if f"#ifndef {SELECTED_GUARD}" not in selected_str or f"#define {SELECTED_GUARD}" not in selected_str:
        errors.append(f"{SELECTED_NAME}: include guard {SELECTED_GUARD} missing")

    selected_names = []
    for include_path in INCLUDE_PATTERN.findall(selected_str):
        header_path = (output_dir_path/include_path).resolve()
        if header_path.parent != output_dir_path:
            errors.append(f"{SELECTED_NAME}: \"{include_path}\" is not in {output_dir_path.name}/")
            continue
        if header_path.name == SELECTED_NAME:
            errors.append(f"{SELECTED_NAME}: includes itself")
            continue
        if header_path.name in selected_names:
            errors.append(f"{SELECTED_NAME}: \"{include_path}\" is listed twice")
            continue
        if not header_path.exists():
            errors.append(f"{SELECTED_NAME}: \"{include_path}\" not found")
            continue
        selected_names.append(header_path.name)
        header_errors, header_warnings = check_header(header_path)
        errors += header_errors
        warnings += header_warnings

    for header_path in sorted(output_dir_path.glob("*.h")):
        if header_path.name != SELECTED_NAME and header_path.name not in selected_names:
            warnings.append(f"{header_path.name}: not selected")
    return errors, warnings, selected_names


def print_check(output_dir_path: Path) -> bool:
    errors, warnings, selected_names = check_selected(output_dir_path)
    print(f"[Info] {SELECTED_NAME}: {len(selected_names)} selected")
    for name in selected_names:
        print(" "*4 + name)
    for warning in warnings:
        print(f"[Warning] {warning}")
    for error in errors:
        print(f"[Error] {error}")
    if not errors:
        print(f"[Success] {SELECTED_NAME} is fine")
    return not errors


def main():
    parser = argparse.ArgumentParser(description=f"Check {SELECTED_NAME}: the melodies compiled into the program.")
    parser.add_argument("output_dir_path", nargs="?", default=str(LIB_DIR_PATH/"output"),
                        help=f"dir with the generated .h and {SELECTED_NAME} (default: output next to source/)")
    args = parser.parse_args()
    output_dir_path = Path(args.output_dir_path).expanduser()
    if not output_dir_path.is_dir():
        parser.error(f"not a dir: {output_dir_path}")
    sys.exit(0 if print_check(output_dir_path) else 1)


if __name__ == "__main__":
    main()
