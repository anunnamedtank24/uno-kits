import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


"""
Func: deletes what the tools made and can make again: every __pycache__, PlatformIO's build folder (.pio/build),
      and the converted fonts, pictures, animations and sounds (lib/<kind>/output/*.h; the README.md stays)
Usage:
    python src/clean.py                  deletes them
    python src/clean.py --dry-run        only lists what it would delete
    python src/clean.py --rebuild        deletes, then converts every output again from its first lines
                                         ("// source: ..." and "// options: ...", the same file name):
                                         a fresh copy of every output; one whose source file is gone is kept
After a plain clean:
    the next build converts again only the FONT / PICTURE / ANIMATION / MELODY files the program names
    (tools/assets.py); a program that names an output directly (font::menlo_..., picture::sunset_...) does not
    compile until it is converted again: use --rebuild, or git restore lib (the outputs are in git)
The converters' python: platformio.ini custom_asset_python, else ASSET_PYTHON, else python3
"""


PROJECT_DIR_PATH = Path(__file__).resolve().parent.parent
ASSET_KINDS = ("font", "image", "video", "audio")
KEPT_NAMES = {"README.md"}
SOURCE_PATTERN = re.compile(r"^// source: (.+) sha256 [0-9a-f]{64}$", re.MULTILINE)
OPTIONS_PATTERN = re.compile(r"^// options: (.*)$", re.MULTILINE)
# made by tools/assets.py on every build, not from one source file
WRITTEN_NAMES = {"selected.h", "registry.h"}


def get_cache_paths() -> list[Path]:
    paths = [path for path in PROJECT_DIR_PATH.rglob("__pycache__") if ".pio" not in path.parts]
    build_path = PROJECT_DIR_PATH/".pio"/"build"
    if build_path.exists():
        paths.append(build_path)
    return sorted(paths)


def get_output_paths() -> list[Path]:
    paths = []
    for kind in ASSET_KINDS:
        output_dir_path = PROJECT_DIR_PATH/"lib"/kind/"output"
        paths += [path for path in output_dir_path.glob("*") if path.is_file() and path.name not in KEPT_NAMES]
    return sorted(paths)


def get_recipe(path: Path) -> tuple[str, list[str]] | None:
    # (source as written in the file, options) of an output made by convert.py; None for the others
    if path.suffix != ".h" or path.name in WRITTEN_NAMES:
        return None
    head = path.read_text(errors="replace")[:2000]
    source = SOURCE_PATTERN.search(head)
    options = OPTIONS_PATTERN.search(head)
    if source is None or options is None:
        return None
    return source.group(1), shlex.split(options.group(1))


def get_source_path(lib_dir_path: Path, source_str: str) -> Path:
    # written relative to lib/<kind> (assets/x.png), or absolute
    path = Path(source_str).expanduser()
    return path if path.is_absolute() else lib_dir_path/path


def get_python() -> str:
    ini_path = PROJECT_DIR_PATH/"platformio.ini"
    match = re.search(r"^custom_asset_python\s*=\s*(\S+)", ini_path.read_text(), re.MULTILINE) if ini_path.exists() else None
    for candidate in (match.group(1) if match else None, os.environ.get("ASSET_PYTHON"), shutil.which("python3"),
                      sys.executable):
        if candidate:
            return str(Path(candidate).expanduser())
    return "python3"


def delete(path: Path):
    if path.is_dir():
        shutil.rmtree(path)
    else:
        path.unlink()


def convert(python: str, output_path: Path, source_path: Path, options: list[str]) -> bool:
    # the same command as by hand, with the old file name
    lib_dir_path = output_path.parent.parent
    command = [python, str(lib_dir_path/"source"/"convert.py"), str(source_path), "output", *options,
               "--name", output_path.stem]
    result = subprocess.run(command, cwd=lib_dir_path, capture_output=True, text=True)
    if result.returncode != 0 or not output_path.exists():
        print(f"[ERROR] {output_path.relative_to(PROJECT_DIR_PATH)}: convert.py failed")
        for line in (result.stdout + result.stderr).strip().splitlines()[-5:]:
            print(f"    {line}")
        return False
    print(f"[SUCCESS] {output_path.relative_to(PROJECT_DIR_PATH)}")
    return True


def main():
    parser = argparse.ArgumentParser(description="Delete the caches and the converted outputs (lib/<kind>/output).")
    parser.add_argument("--dry-run", action="store_true", help="only list what would be deleted")
    parser.add_argument("--rebuild", action="store_true", help="convert every output again after deleting it")
    args = parser.parse_args()

    recipes = {}
    kept = []
    outputs = get_output_paths()
    if args.rebuild:
        for path in outputs:
            recipe = get_recipe(path)
            if recipe is None:
                continue
            source_path = get_source_path(path.parent.parent, recipe[0])
            if source_path.exists():
                recipes[path] = (source_path, recipe[1])
            else:
                kept.append(path)
                print(f"[WARNING] {path.relative_to(PROJECT_DIR_PATH)}: kept, its source is gone ({recipe[0]})")
    paths = get_cache_paths() + [path for path in outputs if path not in kept]

    print(f"[INFO] {'Would delete' if args.dry_run else 'Deleting'} {len(paths)} files / folders")
    for path in paths:
        print(f"    {path.relative_to(PROJECT_DIR_PATH)}")
        if not args.dry_run:
            delete(path)
    if args.dry_run:
        return

    if args.rebuild and recipes:
        python = get_python()
        failed = 0
        for output_path, (source_path, options) in recipes.items():
            print(f"---------- {output_path.name} ----------")
            failed += not convert(python, output_path, source_path, options)
        if failed:
            print(f"[ERROR] {failed} of {len(recipes)} could not be converted again (git restore lib brings them back)")
            sys.exit(1)
        print(f"[SUCCESS] {len(recipes)} outputs converted again")
    elif not args.rebuild:
        print("[INFO] The next build converts the files the program names (FONT, PICTURE ...); programs that name an\n"
              "    output directly need: python src/clean.py --rebuild, or git restore lib")


if __name__ == "__main__":
    main()
