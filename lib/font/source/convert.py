import sys
sys.dont_write_bytecode = True # importing check.py must not leave __pycache__

import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

import check


"""
Func: convert .ttf / .otf to .h (c header), proportional width, every char stored as shapes.
Usage:
    python source/convert.py assets/menlo.ttf output --height 23
    python source/convert.py assets/menlo.ttf output --width 10 --first 48 --last 57 --default "?"
    python source/convert.py assets/menlo.ttf output --size 16
Output:
    output/{font}_{arg1_name}_{arg1_value}_{arg2_name}_{arg2_value}....h, struct font::{same name}
    the new .h is added to output/selected.h (only that one, the others stay as they are),
    then selected.h is checked (source/check.py); font.h includes output/selected.h
Format:
    gridHeight is fixed, every char has its own width.
    every char is a list of shapes covering exactly its pixels (shapes may overlap, never leave the pixels):
        4 counts: points, hLines, vLines, rects
        points (x, y), hLines (x, y, w), vLines (x, y, h), rects (x, y, w, h)
        x / y relative to the top-left of the char, all uint8_t
    widths[i] / offsets[i]: width and first byte in shapes[] of char (first + i),
    the default char is appended at the end when it is outside first ~ last.
Shapes:
    every shape is one fillWindow() on the screen, so the draw time of a char is
        sum of (WINDOW_CYCLES + PIXEL_CYCLES * pixels + FLASH_BYTE_CYCLES * bytes) over its shapes
    the shapes are the cheapest cover found:
        all maximal rects (cannot grow in any direction) -> branch and bound for the cheapest set
        -> shrink every rect to what only it covers -> point / hLine / vLine / rect by its size
Sequence:
    char -> img -> pixels -> maximal rects -> cover -> shapes -> bytes
    chars -> all shapes -> c_list_str -> c_const_str
    c_str_start + c_const_str + c_struct_str + c_str_end -> c_str
"""


SIZE_SEARCH_LIMIT = 1000
THRESHOLD = 128 # grayscale (0 ~ 255) >= THRESHOLD -> bit 1, gives more solid strokes than drawing in 1-bit mode

# ---------- draw time model ----------
# counted from the avr-gcc output of Window::fillWindow() on UNO (16 MHz CPU, 8 MHz SPI)
# the same for every shape type: point / hLine / vLine / rect are all one fillWindow()
# SPI writes do not wait for the byte to be sent (see lib/Uno/Uno_ST7789.h), so a byte costs ~18, not ~23
WINDOW_CYCLES = 480 # call + clip + CASET / RASET / RAMWR (11 SPI bytes)
PIXEL_CYCLES = 44 # 2 SPI bytes, the low byte is sent during the loop
FLASH_BYTE_CYCLES = 6 # reading one byte of a shape from flash

SHAPE_TYPES = ("points", "hLines", "vLines", "rects")
SHAPE_BYTES = {"points": 2, "hLines": 3, "vLines": 3, "rects": 4}
SEARCH_NODE_LIMIT = 200000 # per char, the best cover found so far is used when reached


def indent(str_: str, tab_count: int = 1) -> str:
    added = " "*4*tab_count
    return "\n".join(added + line if line else line for line in str_.split("\n"))


def get_char_width(char: chr, font: ImageFont.FreeTypeFont) -> int:
    return int(round(font.getlength(char)))


def get_grid_height(font: ImageFont.FreeTypeFont) -> int:
    ascent, descent = font.getmetrics()
    return ascent + descent


def get_img_from_char(char: chr, font: ImageFont.FreeTypeFont, char_width: int, grid_height: int) -> Image.Image:
    # top of the cell = ascent line, so every char shares one baseline
    # anything outside char_width x grid_height is cut
    img = Image.new("L", (max(char_width, 1), grid_height), color=0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), char, font=font, fill=255)
    return img.point(lambda value: 1 if value >= THRESHOLD else 0, mode="1")


def get_pixels_from_img(img: Image.Image, char_width: int, grid_height: int) -> set[tuple[int, int]]:
    pixels = img.load()
    return {(x, y) for y in range(grid_height) for x in range(char_width) if pixels[x, y]}


# ---------- shapes ----------
# rect = (x, y, w, h)

def get_rect_pixels(rect: tuple[int, int, int, int]) -> set[tuple[int, int]]:
    x, y, w, h = rect
    return {(px, py) for py in range(y, y + h) for px in range(x, x + w)}


def get_shape_type(rect: tuple[int, int, int, int]) -> str:
    _, _, w, h = rect
    if w == 1 and h == 1:
        return "points"
    if h == 1:
        return "hLines"
    if w == 1:
        return "vLines"
    return "rects"


def get_shape_cost(rect: tuple[int, int, int, int]) -> int:
    _, _, w, h = rect
    return WINDOW_CYCLES + PIXEL_CYCLES*w*h + FLASH_BYTE_CYCLES*SHAPE_BYTES[get_shape_type(rect)]


def get_shapes_cost(rects: list[tuple[int, int, int, int]]) -> int:
    return sum(get_shape_cost(rect) for rect in rects)


def get_maximal_rects(pixels: set[tuple[int, int]]) -> list[tuple[int, int, int, int]]:
    # every rect of pixels that cannot grow left, right, up or down
    # the cheapest cover can always be made of them (then shrunk), so they are the only candidates
    def is_row_filled(y: int, x0: int, x1: int) -> bool:
        return all((x, y) in pixels for x in range(x0, x1 + 1))

    def is_col_filled(x: int, y0: int, y1: int) -> bool:
        return all((x, y) in pixels for y in range(y0, y1 + 1))

    rects = []
    for x0, y0 in sorted(pixels, key=lambda pixel: (pixel[1], pixel[0])):
        x1 = x0
        while (x1, y0) in pixels:
            # x0 ~ x1 from row y0, as tall as possible
            y1 = y0
            while is_row_filled(y1 + 1, x0, x1):
                y1 += 1
            can_grow = is_row_filled(y0 - 1, x0, x1) or is_col_filled(x0 - 1, y0, y1) or is_col_filled(x1 + 1, y0, y1)
            if not can_grow:
                rects.append((x0, y0, x1 - x0 + 1, y1 - y0 + 1))
            x1 += 1
    return rects


def get_best_cover(pixels: set[tuple[int, int]], candidates: list[tuple[int, int, int, int]]) -> tuple[list[tuple[int, int, int, int]], bool]:
    # cheapest set of candidates covering every pixel (weighted set cover), branch and bound
    # returns (rects, proven optimal)
    pixel_list = sorted(pixels)
    pixel_indexes = {pixel: index for index, pixel in enumerate(pixel_list)}
    pixel_bits = {pixel: 1 << index for index, pixel in enumerate(pixel_list)}
    masks = [sum(pixel_bits[pixel] for pixel in get_rect_pixels(rect)) for rect in candidates]
    costs = [get_shape_cost(rect) for rect in candidates]

    # per pixel: candidates covering it, their cheapest cost, every pixel sharing a candidate with it
    covering = [[] for _ in pixel_list]
    for candidate_index, rect in enumerate(candidates):
        for pixel in get_rect_pixels(rect):
            covering[pixel_indexes[pixel]].append(candidate_index)
    cheapest = [min(costs[c] for c in cover) for cover in covering]
    neighbors = [0]*len(pixel_list)
    for index, cover in enumerate(covering):
        for c in cover:
            neighbors[index] |= masks[c]

    def get_bits(mask: int):
        while mask:
            bit = mask & -mask
            yield bit.bit_length() - 1
            mask ^= bit

    def get_lower_bound(uncovered: int) -> int:
        # pixels that share no candidate need one shape each
        bound = 0
        rest = uncovered
        while rest:
            index = (rest & -rest).bit_length() - 1
            bound += cheapest[index]
            rest &= ~neighbors[index]
        return bound

    # greedy first: cheapest cost per new pixel, gives the starting best
    best = []
    uncovered = (1 << len(pixel_list)) - 1
    while uncovered:
        candidate_index = min(
            (c for c in range(len(candidates)) if masks[c] & uncovered),
            key=lambda c: costs[c]/bin(masks[c] & uncovered).count("1"),
        )
        best.append(candidate_index)
        uncovered &= ~masks[candidate_index]
    best_cost = sum(costs[c] for c in best)

    nodes = 0
    chosen = []

    def search(uncovered: int, cost: int):
        nonlocal best, best_cost, nodes
        if nodes >= SEARCH_NODE_LIMIT:
            return
        nodes += 1
        if not uncovered:
            if cost < best_cost:
                best, best_cost = list(chosen), cost
            return
        if cost + get_lower_bound(uncovered) >= best_cost:
            return
        # the pixel with the fewest ways to be covered, every cover must use one of them
        index = min(get_bits(uncovered), key=lambda i: len(covering[i]))
        options = sorted(covering[index], key=lambda c: costs[c]/bin(masks[c] & uncovered).count("1"))
        for c in options:
            chosen.append(c)
            search(uncovered & ~masks[c], cost + costs[c])
            chosen.pop()

    search((1 << len(pixel_list)) - 1, 0)
    return [candidates[c] for c in best], nodes < SEARCH_NODE_LIMIT


def get_shrunk_rects(rects: list[tuple[int, int, int, int]]) -> list[tuple[int, int, int, int]]:
    # overlaps cost time: shrink every rect to the bounding box of what only it covers, drop it if nothing
    rects = sorted(rects, key=lambda rect: rect[2]*rect[3])
    changed = True
    while changed:
        changed = False
        for index, rect in enumerate(rects):
            others = set()
            for other_index, other in enumerate(rects):
                if other_index != index:
                    others |= get_rect_pixels(other)
            only = get_rect_pixels(rect) - others
            if not only:
                rects.pop(index)
                changed = True
                break
            xs = [x for x, _ in only]
            ys = [y for _, y in only]
            shrunk = (min(xs), min(ys), max(xs) - min(xs) + 1, max(ys) - min(ys) + 1)
            if shrunk != rect:
                rects[index] = shrunk
                changed = True
    return rects


def get_shapes_from_pixels(pixels: set[tuple[int, int]]) -> tuple[dict[str, list[tuple[int, ...]]], int, bool]:
    # returns (shapes by type, draw cost in cycles, proven optimal)
    shapes = {shape_type: [] for shape_type in SHAPE_TYPES}
    if not pixels:
        return shapes, 0, True

    cover, is_optimal = get_best_cover(pixels, get_maximal_rects(pixels))
    rects = get_shrunk_rects(cover)

    covered = set()
    for rect in rects:
        covered |= get_rect_pixels(rect)
    if covered != pixels:
        raise RuntimeError("[Bug] shapes do not cover exactly the pixels of a char.")

    for rect in sorted(rects, key=lambda rect: (rect[1], rect[0])):
        x, y, w, h = rect
        shape_type = get_shape_type(rect)
        if shape_type == "points":
            shapes[shape_type].append((x, y))
        elif shape_type == "hLines":
            shapes[shape_type].append((x, y, w))
        elif shape_type == "vLines":
            shapes[shape_type].append((x, y, h))
        else:
            shapes[shape_type].append((x, y, w, h))
    return shapes, get_shapes_cost(rects), is_optimal


def get_bytes_from_shapes(shapes: dict[str, list[tuple[int, ...]]]) -> list[int]:
    bytes_ = []
    for shape_type in SHAPE_TYPES:
        if len(shapes[shape_type]) > 255:
            raise ValueError(f"A char has {len(shapes[shape_type])} {shape_type}, counts must fit in uint8_t (255).")
        bytes_.append(len(shapes[shape_type]))
    for shape_type in SHAPE_TYPES:
        for shape in shapes[shape_type]:
            bytes_.extend(shape)
    return bytes_


# ---------- c code ----------

def get_shapes_str(char: chr, char_width: int, shapes: dict[str, list[tuple[int, ...]]], shapes_per_line: int = 6) -> str:
    counts = [len(shapes[shape_type]) for shape_type in SHAPE_TYPES]
    shapes_str = ""
    shapes_str += f"// {ord(char)} '{char}' width {char_width}: "
    shapes_str += ", ".join(f"{count} {shape_type}" for count, shape_type in zip(counts, SHAPE_TYPES)) + "\n"
    shapes_str += ", ".join(str(count) for count in counts) + ",\n"
    for shape_type in SHAPE_TYPES:
        values = shapes[shape_type]
        for start_index in range(0, len(values), shapes_per_line):
            shape_strs = [", ".join(str(value) for value in shape) + "," for shape in values[start_index:start_index + shapes_per_line]]
            shapes_str += "  ".join(shape_strs) + "\n"
    return shapes_str


def get_c_list_str(type_: str, name: str, content_str: str) -> str:
    c_list_str = ""
    c_list_str += f"static const {type_} {name}[] PROGMEM = {{\n"
    c_list_str += indent(content_str.rstrip("\n")) + "\n"
    c_list_str += "};\n"
    return c_list_str


def get_c_table_str(type_: str, name: str, values: list[int], values_per_line: int = 16) -> str:
    lines = []
    for start_index in range(0, len(values), values_per_line):
        lines.append(", ".join(str(value) for value in values[start_index:start_index + values_per_line]) + ",")
    return get_c_list_str(type_, name, "\n".join(lines))


def get_c_getter_str(comment: str, type_: str, getter_name: str, c_array_str: str, array_name: str) -> str:
    # the array lives inside an inline function: one copy for the whole program, no extra namespace
    c_getter_str = ""
    c_getter_str += f"// {comment}\n"
    c_getter_str += f"static inline const {type_} *{getter_name}() {{\n"
    c_getter_str += indent(c_array_str.rstrip("\n")) + "\n"
    c_getter_str += f"    return {array_name};\n"
    c_getter_str += "}\n"
    return c_getter_str


def get_c_const_str_from_chars(chars: list[chr], font: ImageFont.FreeTypeFont, grid_height: int, first: int, last: int, default: chr) -> str:
    widths = [get_char_width(char, font) for char in chars]
    offsets = []
    offset = 0
    shapes_strs = []
    pixel_count = 0
    bitmap_bytes = 0
    total_cost = 0
    shape_count = 0
    not_optimal_chars = []
    for char, char_width in zip(chars, widths):
        img = get_img_from_char(char, font, char_width, grid_height)
        pixels = get_pixels_from_img(img, char_width, grid_height)
        shapes, cost, is_optimal = get_shapes_from_pixels(pixels)
        bytes_ = get_bytes_from_shapes(shapes)

        offsets.append(offset)
        offset += len(bytes_)
        shapes_strs.append(get_shapes_str(char, char_width, shapes))
        pixel_count += len(pixels)
        bitmap_bytes += (char_width*grid_height + 7)//8
        total_cost += cost
        shape_count += sum(len(shapes[shape_type]) for shape_type in SHAPE_TYPES)
        if not is_optimal:
            not_optimal_chars.append(char)
    if offset > 0xFFFF:
        raise ValueError(f"Font data is {offset} bytes, offsets must fit in uint16_t (65535).")

    point_cost = pixel_count*(WINDOW_CYCLES + PIXEL_CYCLES)
    print(" "*4 + f"Shapes: {shape_count} for {pixel_count} pixels")
    print(" "*4 + f"Draw time of every char once: {total_cost} cycles ({100*total_cost/max(point_cost, 1):.0f}% of a point per pixel)")
    print(" "*4 + f"Shape data: {offset} bytes (bitmap: {bitmap_bytes} bytes)")
    if not_optimal_chars:
        print(" "*4 + f"[Warning] Search limit reached, best found used: {''.join(not_optimal_chars)}")

    default_index = chars.index(default)

    c_const_str = ""
    c_const_str += f"static constexpr uint8_t gridHeight = {grid_height};\n"
    c_const_str += f"static constexpr uint8_t first = {first};\n"
    c_const_str += f"static constexpr uint8_t last = {last};\n"
    c_const_str += f"static constexpr uint8_t defaultChar = {ord(default)}; // '{default}'\n"
    c_const_str += f"static constexpr uint8_t defaultIndex = {default_index}; // index in widths / offsets\n"
    c_const_str += "\n"
    c_const_str += get_c_getter_str("width of char (first + i), the default char is appended when outside first ~ last",
                                    "uint8_t", "getWidths", get_c_table_str("uint8_t", "widths", widths), "widths")
    c_const_str += "\n"
    c_const_str += get_c_getter_str("first byte in getShapes() of char (first + i)",
                                    "uint16_t", "getOffsets", get_c_table_str("uint16_t", "offsets", offsets, values_per_line=12), "offsets")
    c_const_str += "\n"
    c_const_str += get_c_getter_str("shapes of every char: counts of points, hLines, vLines, rects, then "
                                    "points (x, y), hLines (x, y, w), vLines (x, y, h), rects (x, y, w, h)",
                                    "uint8_t", "getShapes", get_c_list_str("uint8_t", "shapes", "".join(shapes_strs)), "shapes")
    return c_const_str


def get_c_struct_str(name: str, c_const_str: str) -> str:
    c_struct_str = ""
    c_struct_str += f"// graphics::Text::setFont<font::{name}>()\n"
    c_struct_str += f"struct {name} {{\n"
    c_struct_str += indent(c_const_str.rstrip("\n")) + "\n"
    c_struct_str += "};\n"
    return c_struct_str


def get_c_str_start(name: str, command: str, font_path: Path) -> str:
    c_str_start = ""
    c_str_start += f"#ifndef FONT_{name.upper()}_H\n"
    c_str_start += f"#define FONT_{name.upper()}_H\n\n"
    c_str_start += f"// generated by convert.py, do not edit\n"
    c_str_start += f"// {command}\n"
    c_str_start += check.get_format_str()
    c_str_start += check.get_source_str(font_path) + "\n"
    c_str_start += "#include <stdint.h>\n"
    c_str_start += "#if defined(__AVR__)\n"
    c_str_start += "#include <avr/pgmspace.h>\n"
    c_str_start += "#endif\n"
    c_str_start += "#ifndef PROGMEM\n"
    c_str_start += "#define PROGMEM\n"
    c_str_start += "#endif\n\n"
    c_str_start += "namespace font {\n"
    return c_str_start


def get_c_str_end() -> str:
    c_str_end = ""
    c_str_end += "}\n\n"
    c_str_end += "#endif\n"
    return c_str_end


def get_c_str(chars: list[chr], font: ImageFont.FreeTypeFont, font_path: Path, name: str, command: str,
              grid_height: int, first: int, last: int, default: chr) -> str:
    c_const_str = get_c_const_str_from_chars(chars, font, grid_height, first, last, default)
    c_str = ""
    c_str += get_c_str_start(name, command, font_path)
    c_str += indent(get_c_struct_str(name, c_const_str).rstrip("\n")) + "\n"
    c_str += get_c_str_end()
    return c_str


# ---------- font ----------

def get_parsed_path(path: str | Path, must_exist: bool = False, make_dir: bool = False, exts: str | set[str] | None = None) -> Path:
    path = Path(path).expanduser().resolve()
    if must_exist and not path.exists():
        raise FileNotFoundError(f"Path not found: {path}")
    if make_dir:
        if path.exists() and not path.is_dir():
            raise NotADirectoryError(f"Not a dir: {path}")
        path.mkdir(parents=True, exist_ok=True)
    if exts is not None:
        if isinstance(exts, str):
            exts = {exts}
        if path.suffix.lower() not in exts:
            raise ValueError(f"Path must have one of the following extensions: {', '.join(sorted(exts))} ({path})")
    return path


def get_parsed_font(font_path: Path, size: int) -> ImageFont.FreeTypeFont:
    try:
        return ImageFont.truetype(str(font_path), size=size)
    except OSError as e:
        raise OSError(f"Not able to load font: {e}")


def get_size_from_limit(font_path: Path, limit: int, get_value) -> int:
    # largest font size whose get_value(font) still fits in limit
    size = 0
    for next_size in range(1, SIZE_SEARCH_LIMIT + 1):
        if get_value(get_parsed_font(font_path, next_size)) > limit:
            break
        size = next_size
    if size == 0:
        raise ValueError(f"No font size fits {limit} px, use a larger value or --size instead.")
    return size


def get_size_from_height(font_path: Path, height: int) -> int:
    return get_size_from_limit(font_path, height, get_grid_height)


def get_size_from_width(font_path: Path, width: int, chars: list[chr]) -> int:
    return get_size_from_limit(font_path, width, lambda font: max(get_char_width(char, font) for char in chars))


def get_is_missing_char(char: chr, font: ImageFont.FreeTypeFont) -> bool:
    # a missing char is drawn with the .notdef glyph, same as a char no font has
    if char.isspace():
        return False
    notdef_mask = font.getmask("\U0010FFFF")
    char_mask = font.getmask(char)
    return char_mask.size == notdef_mask.size and bytes(char_mask) == bytes(notdef_mask)


def get_name(font_path: Path, size_arg_name: str, size_arg_value: int, first: int, last: int, default: chr) -> str:
    font_name = "".join(c if c.isalnum() else "_" for c in font_path.stem.lower())
    if font_name[0].isdigit():
        font_name = "_" + font_name
    return f"{font_name}_{size_arg_name}_{size_arg_value}_first_{first}_last_{last}_default_{ord(default)}"


def get_chars(start_index: int = 32, end_index: int = 126) -> list[chr]:
    chars = [chr(char_index) for char_index in range(start_index, end_index + 1)]
    return chars


# ---------- arguments ----------

def get_positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if number <= 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got {number}")
    return number


def get_char_code(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if not 0 <= number <= 255:
        raise argparse.ArgumentTypeError(f"must be 0 ~ 255, got {number}")
    return number


def get_default_char(value: str) -> chr:
    if len(value) != 1 or ord(value) > 255:
        raise argparse.ArgumentTypeError(f"must be one char with code 0 ~ 255, got {value!r}")
    return value


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert .ttf / .otf to a c header (proportional width, chars stored as shapes).",
        epilog=(
            "examples:\n"
            "  python source/convert.py assets/menlo.ttf output --height 23\n"
            "  python source/convert.py assets/menlo.ttf output --width 10 --first 48 --last 57 --default \"?\"\n"
            "  python source/convert.py assets/menlo.ttf output --size 16"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("font_path", help="font file, .ttf or .otf")
    parser.add_argument("output_dir_path", help="dir for the generated .h (made if missing)")
    size_group = parser.add_mutually_exclusive_group(required=True)
    size_group.add_argument("--height", type=get_positive_int, help="grid height in px (1 ~ 255), font size fitted to ascent + descent")
    size_group.add_argument("--width", type=get_positive_int, help="width of the widest char in px (1 ~ 255), font size fitted to it")
    size_group.add_argument("--size", type=get_positive_int, help="font size passed to FreeType directly")
    parser.add_argument("--first", type=get_char_code, default=32, help="first char code, 0 ~ 255 (default 32)")
    parser.add_argument("--last", type=get_char_code, default=126, help="last char code, first ~ 255 (default 126)")
    parser.add_argument("--default", type=get_default_char, default="?", help="char drawn for codes outside first ~ last (default '?')")
    return parser


def get_parsed_args(parser: argparse.ArgumentParser) -> argparse.Namespace:
    args = parser.parse_args()
    if args.first > args.last:
        parser.error(f"--first ({args.first}) must be <= --last ({args.last})")
    if args.height is not None and args.height > 255:
        parser.error(f"--height must be <= 255 (gridHeight is uint8_t), got {args.height}")
    if args.width is not None and args.width > 255:
        parser.error(f"--width must be <= 255 (widths are uint8_t), got {args.width}")
    font_path = Path(args.font_path).expanduser()
    if not font_path.is_file():
        parser.error(f"font_path not found: {font_path}")
    if font_path.suffix.lower() not in {".ttf", ".otf"}:
        parser.error(f"font_path must be .ttf or .otf, got {font_path.name}")
    return args


def convert_font_to_h(font_path: str, output_dir_path: str, height: int | None, width: int | None, size: int | None,
                      first: int, last: int, default: chr, command: str) -> bool:
    # returns True if selected.h passes the check
    font_path = get_parsed_path(font_path, must_exist=True, exts={".ttf", ".otf"})
    output_dir_path = get_parsed_path(output_dir_path, make_dir=True)

    chars = get_chars(first, last)
    if default not in chars:
        chars.append(default)

    if height is not None:
        size_arg_name, size_arg_value = "height", height
        size = get_size_from_height(font_path, height)
    elif width is not None:
        size_arg_name, size_arg_value = "width", width
        size = get_size_from_width(font_path, width, chars)
    else:
        size_arg_name, size_arg_value = "size", size

    font = get_parsed_font(font_path, size)
    grid_height = height if height is not None else get_grid_height(font)
    if grid_height > 255:
        raise ValueError(f"gridHeight {grid_height} must fit in uint8_t (255), use a smaller size.")
    too_wide_chars = [char for char in chars if get_char_width(char, font) > 255]
    if too_wide_chars:
        raise ValueError(f"Width of {''.join(too_wide_chars)} must fit in uint8_t (255), use a smaller size.")

    if get_is_missing_char(default, font):
        raise ValueError(f"Default char '{default}' is not in the font, choose another with --default.")
    missing_chars = [char for char in chars if get_is_missing_char(char, font)]

    name = get_name(font_path, size_arg_name, size_arg_value, first, last, default)
    output_file_path = output_dir_path/(name + ".h")

    widths = [get_char_width(char, font) for char in chars]
    print(f"[Info] {name}:")
    print(" "*4 + f"Font size: {size}")
    print(" "*4 + f"Ascent: {font.getmetrics()[0]}")
    print(" "*4 + f"Descent: {font.getmetrics()[1]}")
    print(" "*4 + f"Grid height: {grid_height}")
    print(" "*4 + f"Width: {min(widths)} ~ {max(widths)}")
    print(" "*4 + f"Chars: {len(chars)}")
    if missing_chars:
        print(" "*4 + f"[Warning] Not in the font (drawn as .notdef): {''.join(missing_chars)}")

    c_str = get_c_str(chars, font, font_path, name, command, grid_height, first, last, default)
    with open(output_file_path, "w") as f:
        f.write(c_str)
    print(f"[Success] Saved as: {output_file_path}")

    if check.add_selected(output_dir_path, output_file_path):
        print(f"[Success] Added to {check.SELECTED_NAME}")
    if output_dir_path != (check.LIB_DIR_PATH/"output").resolve():
        print(f"[Warning] font.h includes {check.LIB_DIR_PATH.name}/output/{check.SELECTED_NAME}, not this dir")
    return check.print_check(output_dir_path)


def main():
    parser = get_parser()
    args = get_parsed_args(parser)
    command = f"{Path(args.font_path).name} " + " ".join(
        f"--{key} {value!r}" if key == "default" else f"--{key} {value}"
        for key, value in vars(args).items()
        if key not in ("font_path", "output_dir_path") and value is not None
    )
    try:
        is_fine = convert_font_to_h(
            font_path=args.font_path,
            output_dir_path=args.output_dir_path,
            height=args.height,
            width=args.width,
            size=args.size,
            first=args.first,
            last=args.last,
            default=args.default,
            command=command,
        )
    except (ValueError, OSError) as e:
        # same form as the argument errors: usage + message, no traceback
        parser.error(str(e))
    if not is_fine:
        sys.exit(1)


if __name__ == "__main__":
    main()
