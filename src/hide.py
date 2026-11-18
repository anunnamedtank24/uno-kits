import argparse
import subprocess
from pathlib import Path


"""
Func: PlatformIO builds every .c / .cpp in src/, but only one program may have setup() / loop():
      hide the others by adding .bak, keep (or bring back) the one given.
Usage:
    python src/hide.py               every .c / .cpp in src/ (and its sub dirs) -> .bak
    python src/hide.py week_7_L298N/tests/6_car   the same, but the one file matching it stays (or loses its .bak)
Matching the kept file:
    1. "src/" and everything before it is dropped     ~/project/src/week_7_L298N/tests/6_car.cpp -> week_7_L298N/tests/6_car.cpp
    2. the first "." and everything after it is dropped  week_7_L298N/tests/6_car.cpp -> week_7_L298N/tests/6_car
    3. every .c / .cpp (and .bak) in src/ whose path inside src/ contains what is left matches
       ("" matches every file), e.g. week_8 matches week_8_TCRT5000/tests/1_line_tracking.cpp if it is the only file there
    4. "*" splits it into parts that must all appear in that order (each part a substring, anything between):
       "7*car" matches week_7_L298N/tests/6_car.cpp; without "*" it is one plain substring
       (quote it in the shell: python src/hide.py '7*car', or zsh / bash expand the "*" first)
    5. no match, or more than one: error, nothing is renamed
Renaming:
    git mv (the rename is staged, git keeps the file's history); a file git does not track yet
    (or no git) is renamed directly
"""


SRC_DIR_PATH = Path(__file__).resolve().parent
SOURCE_EXTS = {".c", ".cpp"}
HIDDEN_EXT = ".bak"
SRC_PREFIX = "src/"


def get_shown_path(path: Path) -> Path:
    # x.cpp.bak -> x.cpp
    return path.with_suffix("") if path.suffix == HIDDEN_EXT else path


def get_hidden_path(path: Path) -> Path:
    # x.cpp -> x.cpp.bak
    return path.with_name(path.name + HIDDEN_EXT)


def get_query_str(value: str) -> str:
    # "~/project/src/week_7_L298N/tests/6_car.cpp.bak" -> "week_7_L298N/tests/6_car"
    query = value.replace("\\", "/")
    src_index = query.rfind(SRC_PREFIX)
    if src_index >= 0:
        query = query[src_index + len(SRC_PREFIX):]
    return query.split(".", 1)[0]


def is_match(query: str, relative_str: str) -> bool:
    # the parts between "*" appear in order: "7*car" in "week_7_L298N/tests/6_car.cpp"
    position = 0
    for part in query.split("*"):
        index = relative_str.find(part, position)
        if index < 0:
            return False
        position = index + len(part)
    return True


def get_kept_path(parser: argparse.ArgumentParser, value: str) -> Path:
    # the one file in src/ whose path inside src/ matches the query, returned as x.cpp (not .bak)
    query = get_query_str(value)
    matches = [path for path in get_file_paths() if is_match(query, get_relative_str(path))]
    if not matches:
        parser.error(f"no .c / .cpp (or {HIDDEN_EXT}) in {SRC_DIR_PATH.name}/ contains \"{query}\" (from {value})")
    if len(matches) > 1:
        parser.error(f"\"{query}\" (from {value}) matches more than one file: "
                     + ", ".join(get_relative_str(path) for path in matches))
    return get_shown_path(matches[0])


def get_file_paths() -> list[Path]:
    # every .c / .cpp / .c.bak / .cpp.bak file under src/
    return sorted(path for path in SRC_DIR_PATH.rglob("*")
                  if path.is_file() and get_shown_path(path).suffix in SOURCE_EXTS)


def get_source_paths() -> list[Path]:
    # every .c / .cpp under src/, shown or hidden, as the shown path
    return sorted({get_shown_path(path) for path in get_file_paths()})


def get_relative_str(path: Path) -> str:
    return path.relative_to(SRC_DIR_PATH).as_posix()


def rename_path(path: Path, new_path: Path):
    # git mv; a file git does not track (or no git at all): a plain rename
    try:
        result = subprocess.run(["git", "mv", "--", str(path), str(new_path)],
                                cwd=SRC_DIR_PATH, capture_output=True, text=True)
        if result.returncode == 0:
            return
    except FileNotFoundError:
        pass
    path.rename(new_path)


def hide_sources(kept_path: Path | None):
    hidden_count = 0
    shown_count = 0
    for path in get_source_paths():
        hidden_path = get_hidden_path(path)
        if path.exists() and hidden_path.exists():
            print(f"[WARNING] Both {get_relative_str(path)} and {get_relative_str(hidden_path)} exist, left as they are")
            continue

        if path == kept_path:
            if hidden_path.exists():
                rename_path(hidden_path, path)
                shown_count += 1
                print(f"[INFO] Shown:  {get_relative_str(path)}")
            else:
                print(f"[INFO] Kept:   {get_relative_str(path)}")
        elif path.exists():
            rename_path(path, hidden_path)
            hidden_count += 1
            print(f"[INFO] Hidden: {get_relative_str(hidden_path)}")

    kept_str = get_relative_str(kept_path) if kept_path else "nothing"
    print(f"[SUCCESS] {hidden_count} hidden, {shown_count} shown, built: {kept_str}")


def main():
    parser = argparse.ArgumentParser(
        description=f"Hide every .c / .cpp in {SRC_DIR_PATH.name}/ by adding {HIDDEN_EXT}, except the one given.",
        epilog=(
            "examples:\n"
            "  python src/hide.py\n"
            "  python src/hide.py week_7_L298N/tests/6_car\n"
            "  python src/hide.py src/week_7_L298N/tests/6_car.cpp.bak   (same: up to src/ and from . are dropped)\n"
            "  python src/hide.py week_8                                  (if only one .c / .cpp contains it)\n"
            "  python src/hide.py '7*car'                               (\"*\": parts in order, quoted for the shell)"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("kept", nargs="?", help=f"part of the path of the one file to keep (or bring back from {HIDDEN_EXT}), e.g. week_7_L298N/tests/6_car; \"*\" = anything")
    args = parser.parse_args()
    kept_path = get_kept_path(parser, args.kept) if args.kept is not None else None
    hide_sources(kept_path)


if __name__ == "__main__":
    main()
