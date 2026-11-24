import sys
sys.dont_write_bytecode = True  # never leave __pycache__
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path


"""
Func: before PlatformIO compiles, converts the fonts, pictures, animations and sounds a program names by their path,
      so a program can use any file directly:
          using Title = FONT("~/Fonts/Inter.ttf", "--height 23");
          pictures.draw<PICTURE("photos/cat.jpg", "--fit 128x64 --palette 2")>(0, 0);
      the options are those of lib/<kind>/source/convert.py, and the output is the same as running it by hand:
      lib/<kind>/output/<name>.h (one file per conversion, --target stream too)
Usage:
    automatic: platformio.ini  extra_scripts = pre:src/tools/assets.py   (runs before every build: the Build button, pio run)
    by hand:   python src/tools/assets.py            (the same, prints what it does)
               python src/tools/assets.py --force    (converts the program's files again, e.g. after editing a picture)
               python src/tools/assets.py --unused   (lists the outputs no program in src/ uses, .bak ones included)
What it does:
    1. reads every .cpp / .c / .h / .hpp / .ino in src/ (not .bak), comments removed,
       and finds FONT / PICTURE / ANIMATION / MELODY ("path", "options")
    2. path: absolute, ~/..., relative to that source file, the project, or lib/<kind>/assets/
    3. looks in lib/<kind>/output/*.h for one made from that file with those options (every .h made by convert.py
       says so in its first lines: "// source: ..." and "// options: ..."); none: runs
           lib/<kind>/source/convert.py <file> output <options>        (in lib/<kind>)
       so a file is converted once, whoever converted it (this script or you by hand), and the output stays usable
       when the file is moved or deleted (the program still names the old path)
    4. writes lib/<kind>/output/registry.h: Asset<key>::Type = the struct of each macro of the program
       (key = FNV-1a 32 of path, 0x1F, options; lib/<kind>/asset.h computes the same at compile time)
    5. writes lib/<kind>/output/selected.h: every .h of output/ (what is not used is not compiled in)
    files are written only when they change: nothing is recompiled for nothing
The converters need their packages (Pillow, numpy, ...): the python that runs them is
    1. the ASSET_PYTHON environment variable
    2. ASSET_PYTHON=/path/to/python in the project's .env (this computer's own python, not in git)
    3. platformio.ini  custom_asset_python = python3
    4. python3 on the PATH
The board's numbers (flash size and attribute, clock, drawing cycles, tone and sample clock ...) are not the
converters': platformio.ini (of the env being built)
    custom_board = lib/Uno/board.json
    passed to convert.py as --board; an output made for another board, or before board.json changed (the hash of
    its "// board: ..." line), is converted again
"""


SOURCE_SUFFIXES = {".cpp", ".c", ".h", ".hpp", ".ino"}

# macro -> lib folder, namespace
KINDS = {
    "FONT": ("font", "font"),
    "PICTURE": ("image", "picture"),
    "ANIMATION": ("video", "animation"),
    "MELODY": ("audio", "melody"),
}
STRING = r'"((?:[^"\\\n]|\\.)*)"'
MACRO_PATTERN = re.compile(r"\b(FONT|PICTURE|ANIMATION|MELODY)\s*\(\s*" + STRING + r"\s*(?:,\s*" + STRING + r"\s*)?\)")
REFERENCE_PATTERN = re.compile(r"\b(font|picture|animation|melody)\s*::\s*(\w+)")
SOURCE_PATTERN = re.compile(r"^// source: (.+) sha256 ([0-9a-f]{64})$", re.MULTILINE)
OPTIONS_PATTERN = re.compile(r"^// options: (.*)$", re.MULTILINE)
BOARD_PATTERN = re.compile(r"^// board: (.+) sha256 ([0-9a-f]{64})$", re.MULTILINE)
REGISTRY_NAME = "registry.h"
SELECTED_NAME = "selected.h"
NOT_ASSETS = {REGISTRY_NAME, SELECTED_NAME}
# each with a value; not part of what an output is made from (--board: compared on its own, "// board: ...")
IGNORED_OPTIONS = ("--name", "--preview", "--board")
HEAD_BYTES = 4096  # the lines that say how a .h was made are at its start


# ---------- reading the sources ----------

def get_code_str(text: str) -> str:
    # the text with // and /* */ comments replaced by spaces (strings and chars kept as they are)
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join(ch if ch == "\n" else " " for ch in text[i:j]))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def get_unescaped_str(value: str) -> str:
    # the C escapes a path or options string may use
    return re.sub(r"\\(.)", lambda m: {"n": "\n", "t": "\t"}.get(m.group(1), m.group(1)), value)


def get_source_paths(src_dir_path: Path, hidden: bool = False) -> list[Path]:
    # the programs' files; hidden: the .bak ones too
    def is_source(path: Path) -> bool:
        suffix = Path(path.stem).suffix if hidden and path.suffix == ".bak" else path.suffix
        return suffix in SOURCE_SUFFIXES
    return sorted(path for path in src_dir_path.rglob("*") if path.is_file() and is_source(path))


def get_uses(src_dir_path: Path, hidden: bool = False) -> list[dict]:
    # every macro in the sources: macro, path, options, the file and line it is in
    uses = []
    for source_path in get_source_paths(src_dir_path, hidden):
        code_str = get_code_str(source_path.read_text(errors="replace"))
        for match in MACRO_PATTERN.finditer(code_str):
            uses.append({
                "macro": match.group(1),
                "path": get_unescaped_str(match.group(2)),
                "options": get_unescaped_str(match.group(3) or ""),
                "source": source_path,
                "line": code_str.count("\n", 0, match.start()) + 1,
            })
    return uses


def get_references(src_dir_path: Path, hidden: bool = False) -> set[str]:
    # the names the programs use as font::x, picture::x, animation::x, melody::x (comments removed)
    names = set()
    for source_path in get_source_paths(src_dir_path, hidden):
        code_str = get_code_str(source_path.read_text(errors="replace"))
        names.update(match.group(2) for match in REFERENCE_PATTERN.finditer(code_str))
    return names


# ---------- keys, files, options ----------

def get_key(path: str, options: str) -> int:
    # FNV-1a 32 of path, 0x1F, options (the same as asset.h)
    key = 2166136261
    for byte in path.encode() + b"\x1f" + options.encode():
        key = ((key ^ byte) * 16777619) & 0xFFFFFFFF
    return key


def get_candidate_paths(use: dict, project_dir_path: Path, lib_dir_path: Path) -> list[Path]:
    # where the path may point, in order: absolute / ~, next to the source, the project, lib/<kind>/assets
    raw = Path(use["path"]).expanduser()
    if raw.is_absolute():
        return [raw.resolve()]
    return [(base/raw).resolve() for base in (use["source"].parent, project_dir_path, lib_dir_path/"assets")]


def get_match_options(args: list[str]) -> tuple:
    # what makes an output: the options without --name and --preview (the same as check.get_options of each lib),
    # in a fixed order
    kept = []
    skip = False
    for arg in args:
        if skip:
            skip = False
        elif arg in IGNORED_OPTIONS:
            skip = True
        elif arg.split("=", 1)[0] not in IGNORED_OPTIONS:
            kept.append(arg)
    # the order does not matter: "--fit 128x64 --palette 2" is "--palette 2 --fit 128x64"
    groups = []
    for arg in kept:
        if arg.startswith("--"):
            groups.append(arg.split("=", 1))  # "--fit=128x64" is "--fit 128x64"
        elif groups:
            groups[-1].append(arg)
        else:
            groups.append([arg])
    for group in groups:
        if group[0] == "--target":
            group[1:] = sorted(group[1:])  # "--target stream flash" is "--target flash stream"
    return tuple(sorted(tuple(group) for group in groups))


def get_given_name(args: list[str]) -> str | None:
    for i, arg in enumerate(args):
        if arg == "--name" and i + 1 < len(args):
            return args[i + 1]
        if arg.startswith("--name="):
            return arg.split("=", 1)[1]
    return None


def get_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def get_board_values(data):
    # the values without the "note"s (the same as check.get_board_values of each lib)
    if isinstance(data, dict):
        return {key: get_board_values(value) for key, value in data.items() if key != "note"}
    if isinstance(data, list):
        return [get_board_values(value) for value in data]
    return data


def get_board_hash(board_path: Path) -> str:
    data = json.loads(board_path.read_text())
    return hashlib.sha256(json.dumps(get_board_values(data), sort_keys=True).encode()).hexdigest()


def get_ini_board_path(project_dir_path: Path) -> Path | None:
    # platformio.ini custom_board (the first one), relative to the project; None if not there
    ini_path = project_dir_path/"platformio.ini"
    if not ini_path.exists():
        return None
    match = re.search(r"^custom_board[ \t]*=[ \t]*(.*?)[ \t]*(?:[ \t];.*)?$", ini_path.read_text(), re.MULTILINE)
    return get_board_path(project_dir_path, match.group(1)) if match and match.group(1) else None


def get_ini_python(project_dir_path: Path) -> str | None:
    # platformio.ini custom_asset_python (the first one); None if not there
    ini_path = project_dir_path/"platformio.ini"
    if not ini_path.exists():
        return None
    match = re.search(r"^custom_asset_python[ \t]*=[ \t]*(.*?)[ \t]*(?:[ \t];.*)?$", ini_path.read_text(), re.MULTILINE)
    return match.group(1) if match and match.group(1) else None


def get_board_path(project_dir_path: Path, value: str) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else project_dir_path/path


def get_index(lib_dir_path: Path) -> dict:
    # (source file, options) -> (name, sha256 of the source when converted, board hash), from the first lines of output/*.h
    index = {}
    for header_path in sorted((lib_dir_path/"output").glob("*.h")):
        if header_path.name in NOT_ASSETS:
            continue
        with open(header_path, "rb") as f:
            head = f.read(HEAD_BYTES).decode(errors="replace")
        source_match = SOURCE_PATTERN.search(head)
        options_match = OPTIONS_PATTERN.search(head)
        if not source_match or not options_match:
            continue  # not made by convert.py: still usable by its name, not found by a path
        source_path = Path(source_match.group(1))
        if not source_path.is_absolute():
            source_path = lib_dir_path/source_path  # recorded relative to lib/<kind>
        try:
            options = get_match_options(shlex.split(options_match.group(1)))
        except ValueError:
            continue
        board_match = BOARD_PATTERN.search(head)
        board_hash = board_match.group(2) if board_match else None
        index[(str(source_path.resolve()), options)] = (header_path.stem, source_match.group(2), board_hash)
    return index


def find(use: dict, index: dict, project_dir_path: Path, lib_dir_path: Path) -> tuple:
    # the file the macro names (None: not there), and the output made from it (name, sha256, board hash) if there is one
    args = shlex.split(use["options"])
    options = get_match_options(args)
    given_name = get_given_name(args)
    candidates = get_candidate_paths(use, project_dir_path, lib_dir_path)
    file_path = next((path for path in candidates if path.is_file()), None)
    for path in [file_path] if file_path else candidates:
        entry = index.get((str(path), options))
        if entry and (given_name is None or entry[0] == given_name):
            return file_path, entry[0], entry[1], entry[2]
    return file_path, None, None, None


def write_if_changed(path: Path, text: str) -> bool:
    if path.exists() and path.read_text() == text:
        return False
    path.write_text(text)
    return True


# ---------- converting ----------

def get_dotenv(project_dir_path: Path) -> dict[str, str]:
    # KEY=VALUE lines of the project's .env (this computer's settings, not in git); # comments, quotes removed
    values = {}
    path = project_dir_path/".env"
    if not path.exists():
        return values
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.removeprefix("export ").split("=", 1)
        values[key.strip()] = value.strip().strip("'\"")
    return values


def get_python(project_dir_path: Path, given: str | None = None, ini_option: str | None = None) -> str:
    # --python, else ASSET_PYTHON (environment, then .env), else custom_asset_python, else python3
    for candidate in (given, os.environ.get("ASSET_PYTHON"), get_dotenv(project_dir_path).get("ASSET_PYTHON"),
                      ini_option, shutil.which("python3"), sys.executable):
        if candidate:
            return str(Path(candidate).expanduser())
    return "python3"


def convert(python: str, lib_dir_path: Path, file_path: Path, args: list[str], board_path: Path | None) -> str:
    # runs convert.py exactly like by hand, with the project's board; its log (the result is looked up in output/
    # afterwards)
    board_args = ["--board", str(board_path.resolve())] if board_path is not None else []  # convert.py runs in lib/<kind>
    command = [python, str(lib_dir_path/"source"/"convert.py"), str(file_path), "output", *args, *board_args]
    result = subprocess.run(command, cwd=lib_dir_path, capture_output=True, text=True)
    return (result.stdout + result.stderr).strip()


# ---------- the files this script writes ----------

def get_registry_str(lib: str, namespace: str, macro: str, entries: dict) -> str:
    # only the macros of the program being built
    guard = f"{lib.upper()}_REGISTRY_H"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "// written by src/tools/assets.py before every build (do not edit): each",
        f"// {macro}(path, options) of the program in src/ -> {namespace}::Asset<key>::Type, the struct it was converted to",
        "",
    ]
    for key, (use, name) in sorted(entries.items()):
        lines.append(f"// {macro}(\"{use['path']}\", \"{use['options']}\")")
        lines.append(f"#include \"{name}.h\"")
        lines.append(f"namespace {namespace} {{ template <> struct Asset<0x{key:08X}UL> {{ using Type = {name}; }}; }}")
    lines += ["", "#endif", ""]
    return "\n".join(lines)


def get_selected_str(output_dir_path: Path, guard: str, namespace: str) -> str:
    names = sorted(path.name for path in output_dir_path.glob("*.h") if path.name not in NOT_ASSETS)
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "// written by src/tools/assets.py before every build and by convert.py (do not edit): every .h of output/,",
        f"// so a program can use any of them as {namespace}::name; what a program does not use is not compiled in",
        "",
        *[f"#include \"{name}\"" for name in names],
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def update_selected(output_dir_path: Path, namespace: str) -> bool:
    selected_path = output_dir_path/SELECTED_NAME
    old = selected_path.read_text() if selected_path.exists() else ""
    guard_match = re.search(r"#ifndef (\w+)", old)
    guard = guard_match.group(1) if guard_match else f"{namespace.upper()}_SELECTED_H"
    return write_if_changed(selected_path, get_selected_str(output_dir_path, guard, namespace))


# ---------- entry points ----------

def run(project_dir_path: Path, python: str, force: bool = False, board_path: Path | None = None) -> bool:
    # True if every macro has its output; board_path: the project's board.json (None: convert.py finds none, an error)
    project_dir_path = project_dir_path.resolve()
    if board_path is not None and not board_path.exists():
        print(f"[ERROR] src/tools/assets.py: the board file {board_path} (custom_board of platformio.ini) does not exist")
        return False
    board_hash = get_board_hash(board_path) if board_path is not None else None
    uses = get_uses(project_dir_path/"src")
    ok = True
    for macro, (lib, namespace) in KINDS.items():
        lib_dir_path = project_dir_path/"lib"/lib
        if not lib_dir_path.is_dir():
            continue
        output_dir_path = lib_dir_path/"output"
        output_dir_path.mkdir(exist_ok=True)
        index = get_index(lib_dir_path)
        entries = {}
        for use in (use for use in uses if use["macro"] == macro):
            where = f"{use['source'].relative_to(project_dir_path)}:{use['line']}"
            label = f"{macro}(\"{use['path']}\", \"{use['options']}\")"
            try:
                args = shlex.split(use["options"])
            except ValueError as e:
                print(f"[ERROR] {where}: {label}: the options cannot be read ({e})")
                ok = False
                continue
            file_path, name, sha256, made_board = find(use, index, project_dir_path, lib_dir_path)
            if name is not None and not force and board_hash is not None and made_board != board_hash:
                if file_path is None:
                    print(f"[ERROR] {where}: {label}: lib/{lib}/output/{name}.h was made for another board "
                          f"(or before {board_path.name} changed), and the file is gone: it cannot be converted again")
                    ok = False
                    continue
                print(f"[INFO] {where}: {label}: lib/{lib}/output/{name}.h was made for another board "
                      f"(or before {board_path.name} changed), converting it again")
                name = None
            if name is not None and not force:
                if file_path is None:
                    print(f"[INFO] {where}: {label}: the file is gone, using lib/{lib}/output/{name}.h made from it")
                elif get_sha256(file_path) != sha256:
                    print(f"[WARNING] {where}: {label}: the file changed since lib/{lib}/output/{name}.h was made from it; "
                          f"python src/tools/assets.py --force converts it again")
                entries[get_key(use["path"], use["options"])] = (use, name)
                continue
            if file_path is None:
                print(f"[ERROR] {where}: {label}: file not found "
                      f"(tried: absolute / ~, next to the source, the project, lib/{lib}/assets)")
                ok = False
                continue
            print(f"---------- {label} ----------")
            print(f"[INFO] {where}: {label}: converting into lib/{lib}/output/")
            log = convert(python, lib_dir_path, file_path, args, board_path)
            index = get_index(lib_dir_path)
            _, name, _, made_board = find(use, index, project_dir_path, lib_dir_path)
            # an old output still there is no success: it must be made for this board now
            if name is None or (board_hash is not None and made_board != board_hash):
                print(f"[ERROR] {where}: convert.py failed:\n" + "\n".join(" "*4 + line for line in log.splitlines()))
                ok = False
                continue
            print(" "*4 + f"-> {namespace}::{name}")
            for line in log.splitlines():
                if "[WARNING]" in line or "[ERROR]" in line:
                    print(" "*4 + line.strip())
            entries[get_key(use["path"], use["options"])] = (use, name)
        write_if_changed(output_dir_path/REGISTRY_NAME, get_registry_str(lib, namespace, macro, entries))
        update_selected(output_dir_path, namespace)
    return ok


def print_unused(project_dir_path: Path):
    # the outputs no program uses (.bak ones count): by name (font::x ...) or by a macro
    src_dir_path = project_dir_path/"src"
    uses = get_uses(src_dir_path, hidden=True)
    references = get_references(src_dir_path, hidden=True)
    for macro, (lib, namespace) in KINDS.items():
        lib_dir_path = project_dir_path/"lib"/lib
        index = get_index(lib_dir_path)
        used = set(references)
        for use in (use for use in uses if use["macro"] == macro):
            try:
                used.add(find(use, index, project_dir_path, lib_dir_path)[1])
            except ValueError:
                pass
        for header_path in sorted((lib_dir_path/"output").glob("*.h")):
            if header_path.name not in NOT_ASSETS and header_path.stem not in used:
                print(f"lib/{lib}/output/{header_path.name}")


def run_in_platformio():
    Import("env")  # noqa: F821 (SCons provides it)
    project_dir_path = Path(env.subst("$PROJECT_DIR"))  # noqa: F821
    try:
        project_option = env.GetProjectOption("custom_asset_python", "")  # noqa: F821
    except Exception:
        project_option = ""
    try:
        board_option = env.GetProjectOption("custom_board", "")  # noqa: F821
    except Exception:
        board_option = ""
    board_path = get_board_path(project_dir_path, board_option) if board_option else None
    if not run(project_dir_path, get_python(project_dir_path, ini_option=project_option), board_path=board_path):
        print("[ERROR] src/tools/assets.py: some files could not be converted (see above)")
        env.Exit(1)  # noqa: F821


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Convert the FONT / PICTURE / ANIMATION / MELODY files named in src/.")
    parser.add_argument("--force", action="store_true", help="convert the files of the program in src/ again")
    parser.add_argument("--unused", action="store_true",
                        help="list the outputs no program in src/ uses (.bak programs count); nothing is deleted")
    parser.add_argument("--python", help="the python that runs the converters (default: ASSET_PYTHON of the environment or .env, custom_asset_python, python3)")
    args = parser.parse_args()
    project_dir_path = Path(__file__).resolve().parents[2]
    if args.unused:
        print_unused(project_dir_path)
        return
    sys.exit(0 if run(project_dir_path, get_python(project_dir_path, args.python, get_ini_python(project_dir_path)), args.force, get_ini_board_path(project_dir_path)) else 1)


if __name__ == "__main__":
    main()
else:
    # inside SCons (extra_scripts) Import is given; a NameError inside run_in_platformio() is a real error, not this
    try:
        Import  # noqa: F821 (only inside SCons)
    except NameError:
        pass
    else:
        run_in_platformio()
