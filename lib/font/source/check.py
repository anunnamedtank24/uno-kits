import sys
sys.dont_write_bytecode = True # importing must not leave __pycache__

import argparse
import hashlib
import json
import os
import re
import shlex
from pathlib import Path


"""
Func: check output/selected.h, the list of fonts compiled into the program (included by font.h).
Usage:
    python source/check.py
    python source/check.py output
Rules (errors):
    selected.h exists and has its include guard
    every #include "x.h" is a file in the output dir (not selected.h, not outside), listed once
    every included file was made by the current convert.py: same FORMAT_VERSION
    every included file defines struct font::{file name}
    the source font of an included file (recorded path + sha256) is unchanged, if it still exists
    every included file was made for the board of platformio.ini (custom_board): the hash of its
    "// board:" line
Warnings:
    the source font of an included file is gone (cannot tell if it changed)
    a .h in the output dir is not selected (fine, it is just not compiled)
selected.h lists every .h of output/ (unused ones are not compiled in): src/tools/assets.py writes it before every
build, convert.py adds what it makes; nothing to edit by hand. convert.py runs this check after every conversion.
"""


FORMAT_VERSION = 2 # 1: bitmap, 2: shapes (points, hLines, vLines, rects)
NAMESPACE = "font"
SELECTED_NAME = "selected.h"
SELECTED_GUARD = "FONT_SELECTED_H"
LIB_DIR_PATH = Path(__file__).resolve().parent.parent # lib/font

INCLUDE_PATTERN = re.compile(r'^\s*#include\s+"([^"]+)"', re.MULTILINE)
FORMAT_PATTERN = re.compile(r"^// format: (\d+)$", re.MULTILINE)
SOURCE_PATTERN = re.compile(r"^// source: (.+) sha256 ([0-9a-f]{64})$", re.MULTILINE)


def get_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def get_source_str(source_path: Path) -> str:
    # recorded relative to lib/font when possible, so the project can move
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


# ---------- names and options (also used by src/tools/assets.py, through the header lines) ----------

PROJECT_DIR_PATH = LIB_DIR_PATH.parent.parent
ASSETS_DIR_PATH = LIB_DIR_PATH/"assets"
OPTIONS_PATTERN = re.compile(r"^// options: (.*)$", re.MULTILINE)
IGNORED_OPTIONS = ("--name", "--preview", "--board") # each with a value;
# --board: not in "// options:" but in "// board:" (the project's, not the file's)


def get_file_name(source_path: Path) -> str:
    # the start of an output name, a C++ name (ASCII letters, digits, _):
    #   a file in assets/: its name                        assets/sunset.jpg -> sunset
    #   anywhere else: its folders first, relative to the project, else to the home folder, else the whole path,
    #   so two files with the same name never share an output
    #                                                       src/photos/cat.jpg -> src_photos_cat
    #                                                       ~/Music/Game/01 Title.mp3 -> music_game_01_title
    #   letters that are not ASCII (e.g. Japanese) are left out, and a short hash of the path is added instead
    path = source_path.expanduser().resolve()
    if path.is_relative_to(ASSETS_DIR_PATH):
        relative = path.relative_to(ASSETS_DIR_PATH)
    elif path.is_relative_to(PROJECT_DIR_PATH):
        relative = path.relative_to(PROJECT_DIR_PATH)
    elif path.is_relative_to(Path.home()):
        relative = path.relative_to(Path.home())
    else:
        relative = path.relative_to(path.anchor)
    text = relative.with_suffix("").as_posix()
    name = re.sub(r"_+", "_", "".join(c if c.isascii() and c.isalnum() else "_" for c in text.lower())).strip("_")
    if any(not c.isascii() for c in text):
        name = (name + "_" if name else "") + hashlib.sha256(text.encode()).hexdigest()[:6]
    if not name or name[0].isdigit():
        name = "_" + name
    return name


def get_options(argv: list[str], positionals: list[str]) -> list[str]:
    # the options as given, without the positional arguments and what does not change the output
    # (--name, --preview): what src/tools/assets.py compares with the options of FONT / PICTURE / ANIMATION / MELODY
    options = list(argv)
    for value in positionals:
        if value in options:
            options.remove(value)
    kept = []
    skip = False
    for option in options:
        if skip:
            skip = False
        elif option in IGNORED_OPTIONS:
            skip = True
        elif option.split("=", 1)[0] not in IGNORED_OPTIONS:
            kept.append(option)
    return kept


def get_options_str(options: list[str]) -> str:
    return f"// options: {shlex.join(options)}\n"


def get_free_name(output_dir_path: Path, name: str, source_path: Path, options_str: str) -> str:
    # name, or name_2, name_3 ... when output/name.h was made from another file or with other options (the name
    # does not hold every option, e.g. --keyframe): an output is never replaced by a different one;
    # the same file with the same options: the same name (made again in place)
    source_line = get_source_str(source_path).rsplit(" sha256 ", 1)[0] + " sha256 "
    wanted = get_normalized_options(OPTIONS_PATTERN.search(options_str).group(1))
    number = 1
    while True:
        candidate = name if number == 1 else f"{name}_{number}"
        path = Path(output_dir_path)/(candidate + ".h")
        if not path.exists():
            return candidate
        with open(path, "rb") as f:
            head = f.read(4096).decode(errors="replace")
        options_match = OPTIONS_PATTERN.search(head)
        if source_line in head and options_match and get_normalized_options(options_match.group(1)) == wanted:
            return candidate
        number += 1


def get_normalized_options(options_text: str) -> tuple:
    # the same options in any order or form: "--fit=64x64 --palette 2" is "--palette 2 --fit 64x64"
    # (src/tools/assets.py compares them the same way)
    try:
        args = shlex.split(options_text)
    except ValueError:
        return (options_text,)
    groups = []
    for arg in get_options(args, []):
        if arg.startswith("--"):
            groups.append(arg.split("=", 1))
        elif groups:
            groups[-1].append(arg)
        else:
            groups.append([arg])
    for group in groups:
        if group[0] == "--target":
            group[1:] = sorted(group[1:])
    return tuple(sorted(tuple(group) for group in groups))



# ---------- the board: every number of it the converter uses (the project's parameter, not convert.py's) ----------

INI_PATH = PROJECT_DIR_PATH/"platformio.ini"
BOARD_PATTERN = re.compile(r"^// board: (.+) sha256 ([0-9a-f]{64})$", re.MULTILINE)
# the board of this run: set once by load_board (convert.py, stream_handler.py), read by board_value ...
board = None
board_path = None


def get_project_board_path() -> Path | None:
    # platformio.ini of the project: custom_board = lib/Uno/board.json (relative to the project); None if not there
    if not INI_PATH.exists():
        return None
    match = re.search(r"^custom_board[ \t]*=[ \t]*(.*?)[ \t]*(?:[ \t];.*)?$", INI_PATH.read_text(), re.MULTILINE)
    if match is None or not match.group(1):
        return None
    path = Path(match.group(1)).expanduser()
    return path if path.is_absolute() else PROJECT_DIR_PATH/path


def get_board_values(data):
    # the values without the "note"s: what an output depends on
    if isinstance(data, dict):
        return {key: get_board_values(value) for key, value in data.items() if key != "note"}
    if isinstance(data, list):
        return [get_board_values(value) for value in data]
    return data


def get_board_hash(data: dict) -> str:
    return hashlib.sha256(json.dumps(get_board_values(data), sort_keys=True).encode()).hexdigest()


def read_board(path: Path) -> dict:
    try:
        data = json.loads(Path(path).read_text())
    except (OSError, ValueError) as e:
        raise ValueError(f"board file {path} cannot be read: {e}")
    if not isinstance(data, dict) or "name" not in data:
        raise ValueError(f"board file {path}: not a board (no \"name\")")
    return data


def add_board_argument(parser: argparse.ArgumentParser):
    parser.add_argument("--board", metavar="FILE",
                        help="the board's numbers (flash size and attribute, clock, drawing cycles ...), e.g. lib/Uno/board.json "
                             "(default: custom_board of platformio.ini)")


def load_board(path: str | Path | None = None):
    # the board of this run: path, else platformio.ini; ValueError if neither
    global board, board_path
    if path is None:
        path = get_project_board_path()
        if path is None:
            raise ValueError(f"no board: give --board, or custom_board in {INI_PATH.name} (e.g. lib/Uno/board.json)")
    board_path = Path(path).expanduser().resolve()
    board = read_board(board_path)


def board_value(*keys):
    # board_value("flash", "bytes"); ValueError naming the missing key
    value = board
    for key in keys:
        if not isinstance(value, dict) or key not in value:
            raise ValueError(f"board file {board_path} has no {'.'.join(keys)}")
        value = value[key]
    return value


def get_stream_byte_cycles() -> float:
    # one byte over the serial: 10 bits on the line, in CPU cycles, + 10 % for the chunk requests
    return board_value("cpuHz")*10/board_value("serial", "baud")*1.1


def get_board_str() -> str:
    # the line that says which board the output was made for (check_header, src/tools/assets.py compare the hash)
    return f"// board: {board_value('name')} sha256 {get_board_hash(board)}\n"


def get_flash_include_str() -> str:
    header = board_value("flash", "header")
    return f"#include <{header}>\n" if header else ""


def get_array_start_str(type_: str, name: str) -> str:
    # "static const uint8_t name[] PROGMEM = "
    attribute = board_value("flash", "attribute")
    return f"static const {type_} {name}[]" + (f" {attribute}" if attribute else "") + " = "


def get_recorded_board_hash(header_str: str) -> str | None:
    match = BOARD_PATTERN.search(header_str)
    return match.group(2) if match else None


def get_new_selected_str() -> str:
    selected_str = ""
    selected_str += f"#ifndef {SELECTED_GUARD}\n"
    selected_str += f"#define {SELECTED_GUARD}\n\n"
    selected_str += "// fonts compiled into the program, included by font.h\n"
    selected_str += "// every .h of output/ (unused ones are not compiled in): src/tools/assets.py and convert.py keep it\n"
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

    recorded_hash = get_recorded_board_hash(header_str)
    project_board_path = get_project_board_path()
    if recorded_hash is None:
        errors.append(f"{header_path.name}: no board line (made by an old convert.py), regenerate it")
    elif project_board_path is not None and project_board_path.exists():
        project_hash = get_board_hash(read_board(project_board_path))
        if recorded_hash != project_hash:
            errors.append(f"{header_path.name}: made for another board (or {project_board_path.name} changed since): "
                          f"python src/clean.py --rebuild")

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
        if header_path.name not in (SELECTED_NAME, "registry.h") and header_path.name not in selected_names:
            warnings.append(f"{header_path.name}: not selected")
    return errors, warnings, selected_names


def print_check(output_dir_path: Path) -> bool:
    errors, warnings, selected_names = check_selected(output_dir_path)
    print(f"[INFO] {SELECTED_NAME}: {len(selected_names)} selected")
    for name in selected_names:
        print(" "*4 + name)
    for warning in warnings:
        print(f"[WARNING] {warning}")
    for error in errors:
        print(f"[ERROR] {error}")
    if not errors:
        print(f"[SUCCESS] {SELECTED_NAME} is fine")
    return not errors


def main():
    parser = argparse.ArgumentParser(description=f"Check {SELECTED_NAME}: the fonts compiled into the program.")
    parser.add_argument("output_dir_path", nargs="?", default=str(LIB_DIR_PATH/"output"),
                        help=f"dir with the generated .h and {SELECTED_NAME} (default: output next to source/)")
    args = parser.parse_args()
    output_dir_path = Path(args.output_dir_path).expanduser()
    if not output_dir_path.is_dir():
        parser.error(f"not a dir: {output_dir_path}")
    sys.exit(0 if print_check(output_dir_path) else 1)


if __name__ == "__main__":
    main()
