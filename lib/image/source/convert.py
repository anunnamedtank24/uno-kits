import sys
sys.dont_write_bytecode = True # importing check.py must not leave __pycache__

import argparse
import json
import math
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from PIL import Image, ImageOps

import check


"""
Func: convert a picture (.jpg / .jpeg / .png / .webp / .heic / .heif) to .h (c header)
      for image::FlashImage (--target uno) and / or stream.py (--target mac).
      also the pixel decoder used by stream.py.
Usage:
    python source/convert.py assets/sunset.jpg output --target uno --fit 120x120 --scale 2 --palette 16
    python source/convert.py assets/sunset.heic output --target mac --fill 240x240 --palette 565
    python source/convert.py assets/logo.png output --target uno --fit 128x64 --palette 2 --dither
Output (struct picture::{name}, name = {file}_{target}_{w}x{h}_palette_{p} or --name):
    --target uno / uno mac: output/{name}.h with the pixels (FlashImage draws it, StreamImage can ask for it)
    --target mac:           output/mac/{name}.h with the pixels (never compiled in, stream.py reads it)
                            + output/{name}.h without pixels: size, colors, path of the pixels
    every output/{name}.h is added to output/selected.h, then selected.h is checked (source/check.py)
    every setting is written at the top of the files, whatever the name says
    getPath(): where stream.py finds the pixels, relative to lib/image
Files:
    .jpg / .jpeg / .png / .webp: Pillow
    .heic / .heif: pillow-heif if installed (pip install pillow-heif), otherwise macOS sips makes a .png first
    the camera's rotation (EXIF orientation) is applied, transparent parts are put on --background
Ranges: source/config.json (per target, both targets: what both allow); hard limits stop with an error,
        recommended ranges warn
Size (all sizes are on the screen, after scale):
    nothing                 size of the picture (must fit 240 x 240 stored)
    --width / --height      one: keeps the aspect ratio; both: exactly that (stretched)
    --resize F | FXxFY      factor of the picture size, e.g. 0.1 or 0.2x0.1 (any real number)
    --fit WxH               keeps the aspect ratio, as big as fits inside W x H
    --fill WxH              keeps the aspect ratio, covers W x H, the rest is cut (centered)
    --scale N | NXxNY       the board draws every stored pixel as NX x NY (integers): less data,
                            stored size = screen size / scale (rounded) (the "resolution")
    --resample              nearest (pixel art) / bilinear / bicubic / lanczos (default)
Colors:
    --palette 2             black and white, 1 bit (OLED, line art); --threshold, --invert
    --palette 16            one palette of up to 16 colors, 4 bits
    --palette 256           one palette of up to 256 colors, 1 byte (uno only: the palette stays in flash)
    --palette 565           every pixel its own RGB565 color, 2 bytes (photos)
    --colors N              fewer colors in the palette (16 / 256 only), --dither for gradients
Encoding:
    --encoding auto         raw or runs, whichever is smaller (default)
    raw                     every pixel as it is: photos, dithered pictures
    runs                    runs of one color: flat colors, drawings
    --max-bytes N           too big: smaller (x 0.85 per try) until the data fits
"""


CONFIG_PATH = Path(__file__).resolve().parent/"config.json"
CONFIG = json.loads(CONFIG_PATH.read_text())
BOARD = CONFIG["board"]
TARGETS = CONFIG["targets"]
TARGET_BITS = {"uno": 1, "mac": 2} # image::TARGET_UNO / TARGET_MAC
MAC_DIR_NAME = "mac" # output/mac/: pixels for stream.py only

SUFFIXES = (".jpg", ".jpeg", ".png", ".webp", ".heic", ".heif")
HEIF_SUFFIXES = (".heic", ".heif")
SEGMENT = 32 # image::Decoder's default SEGMENT: pixels per writeWindow
MIN_STORED_SIZE = 4 # --max-bytes does not shrink below this
REDUCE_STEP = 0.85 # --max-bytes: size times this per try

PALETTES = (2, 16, 256, 565)
COLOR_MODES = {2: 0, 16: 1, 256: 2, 565: 3} # image::COLORS_2 / COLORS_16 / COLORS_256 / COLORS_565
ENCODINGS = {"raw": 0, "runs": 1} # image::ENCODING_RAW / ENCODING_RUNS
RUN_LIMITS = {2: 128, 16: 271, 256: 256, 565: 256}
BLACK_WHITE = [0x0000, 0xFFFF] # --palette 2
RESAMPLES = {
    "nearest": Image.Resampling.NEAREST,
    "bilinear": Image.Resampling.BILINEAR,
    "bicubic": Image.Resampling.BICUBIC,
    "lanczos": Image.Resampling.LANCZOS,
}


@dataclass
class Settings:
    # everything one conversion needs, after the arguments are resolved
    targets: list[str] # "uno", "mac" or both
    byte_cycles: float
    palette: int
    colors: int | None
    dither: bool
    threshold: int
    invert: bool
    background: tuple[int, int, int]
    encoding: str # "auto", "raw" or "runs"
    screen_width: int
    screen_height: int
    crop: tuple[float, float, float, float] | None # in picture pixels
    scale_x: int
    scale_y: int
    stored_width: int
    stored_height: int
    resample: str


def indent(str_: str, tab_count: int = 1) -> str:
    added = " "*4*tab_count
    return "\n".join(added + line if line else line for line in str_.split("\n"))


def get_rgb565(red: int, green: int, blue: int) -> int:
    return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)


def get_pixels(img: Image.Image) -> list:
    # getdata() is deprecated since Pillow 12
    if hasattr(img, "get_flattened_data"):
        return list(img.get_flattened_data())
    return list(img.getdata())


# ---------- reading ----------

def open_heif(path: Path) -> Image.Image:
    # pillow-heif if installed, otherwise macOS sips makes a .png
    try:
        import pillow_heif
        pillow_heif.register_heif_opener()
        with Image.open(path) as img:
            img.load()
            return img.copy()
    except ImportError:
        pass
    if shutil.which("sips") is None:
        raise OSError("reading .heic / .heif needs pillow-heif (pip install pillow-heif) or macOS sips")
    with tempfile.TemporaryDirectory() as temp_dir:
        png_path = Path(temp_dir)/"picture.png"
        result = subprocess.run(["sips", "-s", "format", "png", str(path), "--out", str(png_path)],
                                capture_output=True, text=True)
        if result.returncode != 0 or not png_path.exists():
            raise OSError(f"sips could not read {path.name}: {result.stderr.strip() or result.stdout.strip()}")
        with Image.open(png_path) as img:
            img.load()
            return img.copy()


def get_picture(path: Path) -> Image.Image:
    # RGBA, turned the way the camera says (EXIF orientation)
    if path.suffix.lower() in HEIF_SUFFIXES:
        img = open_heif(path)
    else:
        with Image.open(path) as opened:
            opened.load()
            img = opened.copy()
    img = ImageOps.exif_transpose(img)
    return img.convert("RGBA")


# ---------- size ----------

def get_pair(value: str, kind) -> tuple:
    # "2" -> (2, 2), "2x3" -> (2, 3)
    parts = value.lower().split("x")
    if len(parts) == 1:
        parts = parts*2
    if len(parts) != 2:
        raise argparse.ArgumentTypeError(f"must be N or NxM, got {value!r}")
    try:
        pair = tuple(kind(part) for part in parts)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be N or NxM ({kind.__name__}), got {value!r}")
    if any(not number > 0 or math.isinf(number) for number in pair):
        raise argparse.ArgumentTypeError(f"must be > 0, got {value!r}")
    return pair


def get_int_pair(value: str) -> tuple[int, int]:
    return get_pair(value, int)


def get_float_pair(value: str) -> tuple[float, float]:
    return get_pair(value, float)


def get_box(value: str) -> tuple[int, int]:
    if "x" not in value.lower():
        raise argparse.ArgumentTypeError(f"must be WxH, got {value!r}")
    return get_pair(value, int)


def get_screen_size_and_crop(picture_size: tuple[int, int], args: argparse.Namespace) -> tuple[int, int, tuple | None]:
    # size on the screen (after scale) and the part of the picture used (None = all)
    picture_width, picture_height = picture_size
    if args.fill is not None:
        box_width, box_height = args.fill
        ratio = max(box_width/picture_width, box_height/picture_height)
        crop_width = box_width/ratio
        crop_height = box_height/ratio
        left = (picture_width - crop_width)/2
        top = (picture_height - crop_height)/2
        return box_width, box_height, (left, top, left + crop_width, top + crop_height)
    if args.fit is not None:
        box_width, box_height = args.fit
        ratio = min(box_width/picture_width, box_height/picture_height)
        return max(1, round(picture_width*ratio)), max(1, round(picture_height*ratio)), None
    if args.resize is not None:
        factor_x, factor_y = args.resize
        return max(1, round(picture_width*factor_x)), max(1, round(picture_height*factor_y)), None
    if args.width is not None and args.height is not None:
        return args.width, args.height, None
    if args.width is not None:
        return args.width, max(1, round(picture_height*args.width/picture_width)), None
    if args.height is not None:
        return max(1, round(picture_width*args.height/picture_height)), args.height, None
    return picture_width, picture_height, None


def get_stored_size(screen_width: int, screen_height: int, scale_x: int, scale_y: int) -> tuple[int, int]:
    return max(1, round(screen_width/scale_x)), max(1, round(screen_height/scale_y))


def get_rendered(picture: Image.Image, settings: Settings) -> Image.Image:
    # cut, resized to the stored size, on the background
    if settings.crop is not None:
        picture = picture.crop(tuple(round(value) for value in settings.crop))
    size = (settings.stored_width, settings.stored_height)
    rgba = picture.resize(size, RESAMPLES[settings.resample])
    background = Image.new("RGBA", size, settings.background + (255,))
    return Image.alpha_composite(background, rgba).convert("RGB")


# ---------- colors ----------

def get_palette_and_values(rendered: Image.Image, settings: Settings) -> tuple[list[int], list[int]]:
    # 565: every pixel its RGB565 color, no palette; 2: black / white; 16 / 256: a palette, values = indexes
    if settings.palette == 565:
        return [], [get_rgb565(*rgb) for rgb in get_pixels(rendered)]

    if settings.palette == 2:
        gray = rendered.convert("L")
        if settings.invert:
            gray = ImageOps.invert(gray)
        if settings.dither:
            values = [1 if value else 0 for value in get_pixels(gray.convert("1", dither=Image.Dither.FLOYDSTEINBERG))]
        else:
            values = [1 if value >= settings.threshold else 0 for value in get_pixels(gray)]
        return list(BLACK_WHITE), values

    color_count = min(settings.colors or settings.palette, settings.palette)
    dither = Image.Dither.FLOYDSTEINBERG if settings.dither else Image.Dither.NONE
    quantized = rendered.quantize(colors=color_count, method=Image.Quantize.MEDIANCUT, dither=dither)
    indexes = get_pixels(quantized)
    all_rgb = quantized.getpalette()
    # only the colors used, renumbered 0 ~ n - 1
    used = sorted(set(indexes))
    renumber = {old: new for new, old in enumerate(used)}
    palette = [get_rgb565(*all_rgb[3*index:3*index + 3]) for index in used]
    return palette, [renumber[index] for index in indexes]


# ---------- encoding (also used by stream.py) ----------

def get_raw_bytes(values: list[int], palette: int) -> list[int]:
    if palette == 565:
        return [byte for value in values for byte in (value & 0xFF, value >> 8)]
    if palette == 256:
        return list(values)
    # 1 or 4 bits, first pixel in the high bits, running on across rows
    bit_count = 1 if palette == 2 else 4
    per_byte = 8//bit_count
    bytes_ = []
    for start in range(0, len(values), per_byte):
        byte = 0
        for i, value in enumerate(values[start:start + per_byte]):
            byte |= value << (8 - bit_count*(i + 1))
        bytes_.append(byte)
    return bytes_


def get_run_bytes(values: list[int], palette: int) -> list[int]:
    bytes_ = []
    position = 0
    while position < len(values):
        value = values[position]
        count = 1
        while position + count < len(values) and values[position + count] == value and count < RUN_LIMITS[palette]:
            count += 1
        if palette == 2:
            bytes_.append((value << 7) | (count - 1))
        elif palette == 16:
            if count < 16:
                bytes_.append((count << 4) | value)
            else:
                bytes_.extend([value, count - 16])
        elif palette == 256:
            bytes_.extend([count - 1, value])
        else:
            bytes_.extend([count - 1, value & 0xFF, value >> 8])
        position += count
    return bytes_


def get_encoded(values: list[int], palette: int, encoding: str) -> tuple[str, list[int]]:
    # "auto": the smaller one (raw on a tie: no run bookkeeping)
    options = {}
    if encoding in ("auto", "raw"):
        options["raw"] = get_raw_bytes(values, palette)
    if encoding in ("auto", "runs"):
        options["runs"] = get_run_bytes(values, palette)
    name = min(options, key=lambda key: (len(options[key]), key != "raw"))
    return name, options[name]


def get_decoded_values(data: list[int], count: int, palette: int, encoding: str) -> list[int]:
    # the inverse of get_encoded (stream.py checks files with it)
    values = []
    if encoding == "raw":
        if palette == 565:
            return [data[i] | (data[i + 1] << 8) for i in range(0, 2*count, 2)]
        if palette == 256:
            return list(data[:count])
        bit_count = 1 if palette == 2 else 4
        mask = (1 << bit_count) - 1
        for byte in data:
            for shift in range(8 - bit_count, -1, -bit_count):
                values.append((byte >> shift) & mask)
        return values[:count]
    position = 0
    while len(values) < count:
        if palette == 2:
            run = data[position]
            values += [run >> 7]*((run & 0x7F) + 1)
            position += 1
        elif palette == 16:
            run = data[position]
            run_count = run >> 4
            position += 1
            if run_count == 0:
                run_count = data[position] + 16
                position += 1
            values += [run & 0x0F]*run_count
        elif palette == 256:
            values += [data[position + 1]]*(data[position] + 1)
            position += 2
        else:
            values += [data[position + 1] | (data[position + 2] << 8)]*(data[position] + 1)
            position += 3
    return values[:count]


# ---------- estimates ----------

def get_draw_ms(settings: Settings, data_bytes: int) -> float:
    # segments of SEGMENT screen pixels per stored row, sent once per screen row
    row_segments = math.ceil(settings.stored_width*settings.scale_x/SEGMENT)
    segments = row_segments*settings.stored_height*settings.scale_y
    pixels = settings.stored_width*settings.scale_x*settings.stored_height*settings.scale_y
    cycles = segments*BOARD["segmentCycles"] + pixels*BOARD["pixelCycles"] + data_bytes*settings.byte_cycles
    return cycles*1000/BOARD["cpuHz"]


# ---------- c code ----------

def get_c_table_str(type_: str, name: str, values: list, values_per_line: int = 16) -> str:
    lines = []
    for start_index in range(0, len(values), values_per_line):
        lines.append(", ".join(str(value) for value in values[start_index:start_index + values_per_line]) + ",")
    return get_c_list_str(type_, name, "\n".join(lines) if lines else "0, // empty")


def get_c_list_str(type_: str, name: str, content_str: str) -> str:
    c_list_str = ""
    c_list_str += f"static const {type_} {name}[] PROGMEM = {{\n"
    c_list_str += indent(content_str.rstrip("\n")) + "\n"
    c_list_str += "};\n"
    return c_list_str


def get_c_getter_str(comment: str, type_: str, getter_name: str, c_array_str: str, array_name: str) -> str:
    # the array lives inside an inline function: one copy for the whole program, no extra namespace
    c_getter_str = ""
    c_getter_str += f"// {comment}\n"
    c_getter_str += f"static inline const {type_} *{getter_name}() {{\n"
    c_getter_str += indent(c_array_str.rstrip("\n")) + "\n"
    c_getter_str += f"    return {array_name};\n"
    c_getter_str += "}\n"
    return c_getter_str


def get_settings_str(settings: Settings, picture_info: str, palette: list[int], encoding: str,
                     data_bytes: int, draw_ms: float) -> str:
    # every setting, whatever the file name says
    crop = "none" if settings.crop is None else "x {:.1f} y {:.1f} w {:.1f} h {:.1f} (picture px)".format(
        settings.crop[0], settings.crop[1], settings.crop[2] - settings.crop[0], settings.crop[3] - settings.crop[1])
    if settings.palette == 565:
        color_str = "palette 565, no palette"
    elif settings.palette == 2:
        way = "dither" if settings.dither else f"threshold {settings.threshold}"
        color_str = f"palette 2 (black / white), {way}, invert {'on' if settings.invert else 'off'}"
    else:
        color_str = f"palette {settings.palette}, {len(palette)} colors used, dither {'on' if settings.dither else 'off'}"
    time_name = "draw" if "uno" in settings.targets else "draw + send"
    target_lines = [f"target      {target}: {TARGETS[target]['note']}" for target in settings.targets]
    lines = target_lines + [
        f"picture     {picture_info}",
        f"crop        {crop}",
        f"screen      {settings.stored_width*settings.scale_x} x {settings.stored_height*settings.scale_y} px "
        f"= stored {settings.stored_width} x {settings.stored_height} x scale {settings.scale_x} x {settings.scale_y}",
        f"resample    {settings.resample}",
        f"colors      {color_str}, background {'%02X%02X%02X' % settings.background}",
        f"encoding    {encoding} (asked: {settings.encoding})",
        f"data        {data_bytes} bytes",
        f"{time_name:<11} ~{draw_ms:.0f} ms (estimated, ST7789 on an UNO)",
    ]
    return "// settings:\n" + "".join(f"//   {line}\n" for line in lines)


def get_target_bits(targets: list[str]) -> int:
    return sum(TARGET_BITS[target] for target in targets)


def get_c_start_str(name: str, command: str, picture_path: Path, settings_str: str) -> str:
    c_str = ""
    c_str += f"#ifndef PICTURE_{name.upper()}_H\n"
    c_str += f"#define PICTURE_{name.upper()}_H\n\n"
    c_str += "// generated by convert.py, do not edit\n"
    c_str += f"// {command}\n"
    c_str += check.get_format_str()
    c_str += check.get_source_str(picture_path)
    c_str += settings_str + "\n"
    c_str += "#include <stdint.h>\n"
    c_str += "#if defined(__AVR__)\n"
    c_str += "#include <avr/pgmspace.h>\n"
    c_str += "#endif\n"
    c_str += "#ifndef PROGMEM\n"
    c_str += "#define PROGMEM\n"
    c_str += "#endif\n\n"
    c_str += "namespace picture {\n"
    return c_str


def get_c_struct_str(name: str, usage: str, c_const_str: str) -> str:
    c_struct_str = ""
    c_struct_str += "".join(f"// {line}\n" for line in usage.split("\n"))
    c_struct_str += f"struct {name} {{\n"
    c_struct_str += indent(c_const_str.rstrip("\n")) + "\n"
    c_struct_str += "};\n"
    return indent(c_struct_str.rstrip("\n")) + "\n}\n\n#endif\n"


def get_c_info_str(settings: Settings, palette: list[int], encoding: str, data_bytes: int, path: str) -> str:
    # what every output/{name}.h has, with or without the pixels
    c_info_str = ""
    c_info_str += f"static constexpr uint8_t target = {get_target_bits(settings.targets)}; // "
    c_info_str += " | ".join(f"image::TARGET_{target.upper()}" for target in settings.targets) + "\n"
    c_info_str += f"static constexpr uint8_t width = {settings.stored_width};\n"
    c_info_str += f"static constexpr uint8_t height = {settings.stored_height};\n"
    c_info_str += f"static constexpr uint8_t scaleX = {settings.scale_x};\n"
    c_info_str += f"static constexpr uint8_t scaleY = {settings.scale_y};\n"
    c_info_str += f"static constexpr uint8_t colorMode = {COLOR_MODES[settings.palette]}; // image::COLORS_{settings.palette}\n"
    c_info_str += f"static constexpr uint8_t encoding = {ENCODINGS[encoding]}; // image::ENCODING_{encoding.upper()}\n"
    c_info_str += f"static constexpr uint16_t colorCount = {len(palette)};\n"
    c_info_str += f"static constexpr uint32_t dataSize = {data_bytes};\n"
    c_info_str += "\n"
    c_info_str += get_c_getter_str("where stream.py finds the pixels, relative to lib/image (image::StreamImage sends it)",
                                   "char", "getPath", f'static const char path[] PROGMEM = "{path}";\n', "path")
    return c_info_str


def get_c_data_str(name: str, command: str, picture_path: Path, settings: Settings, settings_str: str,
                   palette: list[int], encoding: str, data: list[int], path: str) -> str:
    # the whole picture: info + palette + pixels
    c_const_str = get_c_info_str(settings, palette, encoding, len(data), path)
    c_const_str += "\n"
    c_const_str += get_c_getter_str("RGB565 colors (none for palette 565)", "uint16_t", "getPalette",
                                    get_c_table_str("uint16_t", "palette", [f"0x{color:04X}" for color in palette], values_per_line=8), "palette")
    c_const_str += "\n"
    c_const_str += get_c_getter_str(f"pixels, {encoding}, format: lib/image/decoder.h", "uint8_t", "getPixels",
                                    get_c_table_str("uint8_t", "pixels", data, values_per_line=24), "pixels")

    usages = []
    if "uno" in settings.targets:
        usages.append(f"image::FlashImage::draw<picture::{name}>(x, y)")
    if "mac" in settings.targets:
        usages.append(f"image::StreamImage::draw<picture::{name}>(x, y), with stream.py running")
    if settings.targets == ["mac"]:
        usages = [f"pixels for stream.py only, never included: image::StreamImage::draw<picture::{name}>(x, y)"]
    return get_c_start_str(name, command, picture_path, settings_str) + get_c_struct_str(name, "\n".join(usages), c_const_str)


def get_c_descriptor_str(name: str, command: str, picture_path: Path, settings: Settings, settings_str: str,
                         palette: list[int], encoding: str, data: list[int], path: str) -> str:
    # --target mac: no pixels, only what the board needs to ask for it
    c_const_str = get_c_info_str(settings, palette, encoding, len(data), path)
    usage = f"image::StreamImage::draw<picture::{name}>(x, y), with stream.py running\nthe pixels are in {path}"
    return get_c_start_str(name, command, picture_path, settings_str) + get_c_struct_str(name, usage, c_const_str)


def get_name(picture_path: Path, settings: Settings) -> str:
    file_name = "".join(c if c.isalnum() else "_" for c in picture_path.stem.lower())
    if file_name[0].isdigit():
        file_name = "_" + file_name
    screen_width = settings.stored_width*settings.scale_x
    screen_height = settings.stored_height*settings.scale_y
    return f"{file_name}_{'_'.join(settings.targets)}_{screen_width}x{screen_height}_palette_{settings.palette}"


def get_lib_relative_str(path: Path) -> str:
    # path as stream.py looks for it: relative to lib/image
    try:
        return path.resolve().relative_to(check.LIB_DIR_PATH).as_posix()
    except ValueError:
        return path.resolve().as_posix()


# ---------- arguments ----------

def get_positive_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if number <= 0:
        raise argparse.ArgumentTypeError(f"must be > 0, got {number}")
    return number


def get_byte(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if not 0 <= number <= 255:
        raise argparse.ArgumentTypeError(f"must be 0 ~ 255, got {number}")
    return number


def get_color(value: str) -> tuple[int, int, int]:
    text = value.lstrip("#")
    if len(text) != 6 or any(c not in "0123456789abcdefABCDEF" for c in text):
        raise argparse.ArgumentTypeError(f"must be a hex color like 000000 or #FF8800, got {value!r}")
    return int(text[0:2], 16), int(text[2:4], 16), int(text[4:6], 16)


def get_identifier(value: str) -> str:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value):
        raise argparse.ArgumentTypeError(f"must be a C++ name (letters, digits, _), got {value!r}")
    return value


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert a picture (.jpg / .jpeg / .png / .webp / .heic / .heif) to a c header "
                    "for image::FlashImage (--target uno) and / or image::StreamImage (--target mac).",
        epilog=(
            "examples:\n"
            "  python source/convert.py assets/sunset.jpg output --target uno --fit 120x120 --scale 2 --palette 16\n"
            "  python source/convert.py assets/sunset.heic output --target mac --fill 240x240 --palette 565\n"
            "  python source/convert.py assets/logo.png output --target uno --fit 128x64 --palette 2 --dither\n"
            "  python source/check.py    (checks output/selected.h)\n"
            "ranges and defaults per target: source/config.json"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("picture_path", help="picture: " + " ".join(SUFFIXES))
    parser.add_argument("output_dir_path", help="dir for the generated .h (made if missing)")
    parser.add_argument("--target", choices=sorted(TARGETS), nargs="+", required=True,
                        help="uno: pixels in flash (FlashImage), mac: pixels on the computer (StreamImage + stream.py), "
                             "both: within the ranges of both")
    parser.add_argument("--name", type=get_identifier, help="file / struct name instead of the default (every setting is in the file anyway)")

    size_group = parser.add_argument_group("size on the screen (one way; --width and --height may go together)")
    size_group.add_argument("--width", type=get_positive_int, help="px; alone: keeps the aspect ratio")
    size_group.add_argument("--height", type=get_positive_int, help="px; alone: keeps the aspect ratio")
    size_group.add_argument("--resize", type=get_float_pair, help="F or FXxFY: factor of the picture size, any real number")
    size_group.add_argument("--fit", type=get_box, help="WxH: keeps the aspect ratio, inside the box")
    size_group.add_argument("--fill", type=get_box, help="WxH: keeps the aspect ratio, covers the box, the rest is cut")
    size_group.add_argument("--scale", type=get_int_pair, default=(1, 1), help="N or NXxNY: the board repeats every stored pixel (default 1)")
    size_group.add_argument("--resample", choices=sorted(RESAMPLES), default="lanczos", help="nearest for pixel art (default lanczos)")

    color_group = parser.add_argument_group("colors")
    color_group.add_argument("--palette", type=int, choices=PALETTES, help="2 / 16 / 256 / 565 (default per target, config.json)")
    color_group.add_argument("--colors", type=get_positive_int, help="at most this many colors in the palette (16 / 256 only)")
    color_group.add_argument("--dither", action="store_true", help="Floyd-Steinberg dithering (2 / 16 / 256): smoother gradients, more data")
    color_group.add_argument("--threshold", type=get_byte, default=128, help="palette 2 without --dither: gray >= this is white (default 128)")
    color_group.add_argument("--invert", action="store_true", help="palette 2: black <-> white")
    color_group.add_argument("--background", type=get_color, default=(0, 0, 0), help="color behind transparent pixels (default 000000)")

    data_group = parser.add_argument_group("data")
    data_group.add_argument("--encoding", choices=("auto", "raw", "runs"), default="auto", help="auto: the smaller one (default)")
    data_group.add_argument("--max-bytes", type=get_positive_int, help="make it smaller until the data fits")
    return parser


def get_target_config(targets: list[str]) -> dict:
    # one target: its ranges; both: what both allow (the stricter limit, the uno costs)
    if len(targets) == 1:
        return TARGETS[targets[0]]
    uno = TARGETS["uno"]
    mac = TARGETS["mac"]
    palettes = [palette for palette in uno["palettes"] if palette in mac["palettes"]]
    default_palette = uno["defaultPalette"] if uno["defaultPalette"] in palettes else palettes[0]
    return {
        "palettes": palettes,
        "recommendedPalettes": [palette for palette in uno["recommendedPalettes"] if palette in mac["recommendedPalettes"]],
        "defaultPalette": default_palette,
        "byteCycles": uno["byteCycles"],
        "recommendedMaxBytes": uno["recommendedMaxBytes"],
    }


def check_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> list[str]:
    # errors stop here, warnings are returned
    warnings = []
    args.target = [target for target in ("uno", "mac") if target in args.target]
    target = get_target_config(args.target)
    target_str = " ".join(args.target)
    size_ways = [way for way in ("resize", "fit", "fill") if getattr(args, way) is not None]
    if args.width is not None or args.height is not None:
        size_ways.append("width / height")
    if len(size_ways) > 1:
        parser.error(f"one way to set the size, got {', '.join(size_ways)}")

    picture_path = Path(args.picture_path).expanduser()
    if not picture_path.is_file():
        parser.error(f"picture_path not found: {picture_path}")
    if picture_path.suffix.lower() not in SUFFIXES:
        parser.error(f"picture_path must be {' / '.join(SUFFIXES)}, got {picture_path.name}")

    if args.palette is None:
        args.palette = target["defaultPalette"]
    if args.palette not in target["palettes"]:
        parser.error(f"--palette {args.palette} is not possible for --target {target_str} (config.json: {target['palettes']})")
    if args.palette not in target["recommendedPalettes"]:
        warnings.append(f"--palette {args.palette} with --target {target_str} makes a lot of data")
    if args.colors is not None:
        if args.palette in (2, 565):
            warnings.append(f"--colors is ignored with --palette {args.palette}")
        elif not 2 <= args.colors <= args.palette:
            parser.error(f"--colors must be 2 ~ {args.palette} with --palette {args.palette}, got {args.colors}")
    if args.dither and args.palette == 565:
        warnings.append("--dither is ignored with --palette 565")
    if args.palette != 2 and (args.invert or args.threshold != 128):
        warnings.append("--threshold / --invert are only for --palette 2")
    if args.palette == 2 and args.dither and args.threshold != 128:
        warnings.append("--threshold is ignored with --dither")
    if max(args.scale) > CONFIG["maxScale"]:
        parser.error(f"--scale must be at most {CONFIG['maxScale']} (config.json), got {args.scale}")
    return warnings


def get_settings(args: argparse.Namespace, picture_size: tuple[int, int], size_factor: float) -> Settings:
    screen_width, screen_height, crop = get_screen_size_and_crop(picture_size, args)
    screen_width = max(1, round(screen_width*size_factor))
    screen_height = max(1, round(screen_height*size_factor))
    scale_x, scale_y = args.scale
    stored_width, stored_height = get_stored_size(screen_width, screen_height, scale_x, scale_y)
    return Settings(
        targets=args.target, byte_cycles=get_target_config(args.target)["byteCycles"],
        palette=args.palette, colors=args.colors, dither=args.dither, threshold=args.threshold, invert=args.invert,
        background=args.background, encoding=args.encoding,
        screen_width=screen_width, screen_height=screen_height, crop=crop, scale_x=scale_x, scale_y=scale_y,
        stored_width=stored_width, stored_height=stored_height, resample=args.resample,
    )


def check_settings(settings: Settings) -> tuple[list[str], list[str]]:
    errors = []
    warnings = []
    width = settings.stored_width*settings.scale_x
    height = settings.stored_height*settings.scale_y
    if max(settings.stored_width, settings.stored_height) > CONFIG["maxStoredSize"]:
        errors.append(f"stored {settings.stored_width} x {settings.stored_height} px: each must be <= {CONFIG['maxStoredSize']} "
                      "(give a size: --fit, --width, --resize ...)")
    if width > CONFIG["screen"]["width"] or height > CONFIG["screen"]["height"]:
        warnings.append(f"{width} x {height} px on the screen: bigger than {CONFIG['screen']['width']} x {CONFIG['screen']['height']}, "
                        "the part off the screen is cut (data for nothing)")
    if (width, height) != (settings.screen_width, settings.screen_height):
        warnings.append(f"{settings.screen_width} x {settings.screen_height} is not a multiple of --scale "
                        f"{settings.scale_x}x{settings.scale_y}: drawn as {width} x {height}")
    return errors, warnings


def convert_picture_to_h(args: argparse.Namespace, command: str, warnings: list[str]) -> bool:
    # returns True if selected.h passes the check
    picture_path = Path(args.picture_path).expanduser().resolve()
    output_dir_path = Path(args.output_dir_path).expanduser().resolve()
    if output_dir_path.exists() and not output_dir_path.is_dir():
        raise NotADirectoryError(f"Not a dir: {output_dir_path}")
    output_dir_path.mkdir(parents=True, exist_ok=True)

    picture = get_picture(picture_path)
    picture_info = f"{picture.size[0]} x {picture.size[1]} px, {picture_path.suffix.lower()[1:]}"
    target = get_target_config(args.target)

    # --max-bytes: try, make it smaller, try again
    size_factor = 1.0
    while True:
        settings = get_settings(args, picture.size, size_factor)
        errors, size_warnings = check_settings(settings)
        if errors:
            raise ValueError("; ".join(errors))
        palette, values = get_palette_and_values(get_rendered(picture, settings), settings)
        encoding, data = get_encoded(values, settings.palette, settings.encoding)
        data_bytes = len(data) + 2*len(palette)
        if args.max_bytes is None or data_bytes <= args.max_bytes:
            break
        if min(settings.stored_width, settings.stored_height)*REDUCE_STEP < MIN_STORED_SIZE:
            raise ValueError(f"cannot fit --max-bytes {args.max_bytes}: smallest try is {data_bytes} bytes "
                             f"(size {MIN_STORED_SIZE} px reached)")
        print(f"[Info] {data_bytes} bytes > --max-bytes {args.max_bytes} "
              f"({settings.stored_width} x {settings.stored_height} px), making it smaller")
        size_factor *= REDUCE_STEP
    warnings += size_warnings
    if get_decoded_values(data, len(values), settings.palette, encoding) != values:
        raise ValueError("internal: the encoded pixels do not decode back (please report)")
    if target["recommendedMaxBytes"] is not None and data_bytes > target["recommendedMaxBytes"]:
        warnings.append(f"{data_bytes} bytes is above {target['recommendedMaxBytes']} (config.json) for --target {' '.join(args.target)}")

    draw_ms = get_draw_ms(settings, data_bytes)
    name = args.name or get_name(picture_path, settings)
    output_file_path = output_dir_path/(name + ".h")
    settings_str = get_settings_str(settings, picture_info, palette, encoding, data_bytes, draw_ms)
    if args.target == ["mac"]:
        # pixels in output/mac/ (never included), the board gets a small file with the path
        data_file_path = output_dir_path/MAC_DIR_NAME/(name + ".h")
        path = get_lib_relative_str(data_file_path)
        files = [
            (data_file_path, get_c_data_str(name, command, picture_path, settings, settings_str, palette, encoding, data, path)),
            (output_file_path, get_c_descriptor_str(name, command, picture_path, settings, settings_str, palette, encoding, data, path)),
        ]
    else:
        path = get_lib_relative_str(output_file_path)
        files = [(output_file_path, get_c_data_str(name, command, picture_path, settings, settings_str, palette, encoding, data, path))]
    if path.startswith("/"):
        warnings.append(f"{path} is outside lib/image: stream.py only opens files inside lib/image")

    print(f"[Info] {name}:")
    print("\n".join(" "*4 + line[5:] for line in settings_str.splitlines()[1:]))
    for warning in warnings:
        print(f"[Warning] {warning}")
    for file_path, c_str in files:
        file_path.parent.mkdir(parents=True, exist_ok=True)
        if file_path.exists():
            print(f"[Info] Replaced the old {file_path.name}")
        with open(file_path, "w") as f:
            f.write(c_str)
        print(f"[Success] Saved as: {file_path}")

    if check.add_selected(output_dir_path, output_file_path):
        print(f"[Success] Added to {check.SELECTED_NAME}")
    if output_dir_path != (check.LIB_DIR_PATH/"output").resolve():
        print(f"[Warning] image.h includes {check.LIB_DIR_PATH.name}/output/{check.SELECTED_NAME}, not this dir")
    return check.print_check(output_dir_path)


def main():
    parser = get_parser()
    args = parser.parse_args()
    warnings = check_args(parser, args)
    command = " ".join([Path(args.picture_path).name] + sys.argv[2:])
    try:
        is_fine = convert_picture_to_h(args, command, warnings)
    except (ValueError, OSError) as e:
        # same form as the argument errors: usage + message, no traceback
        parser.error(str(e))
    if not is_fine:
        sys.exit(1)


if __name__ == "__main__":
    main()
