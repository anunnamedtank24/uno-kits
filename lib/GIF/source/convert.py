import sys
sys.dont_write_bytecode = True # importing check.py must not leave __pycache__

import argparse
import json
import math
import re
from dataclasses import dataclass
from pathlib import Path
from PIL import Image, ImageSequence

import check


"""
Func: convert .gif to .h (c header) for GIF::FlashPlayer (--target uno) and / or stream.py (--target mac).
      also the frame encoder / decoder used by stream.py.
Usage:
    python source/convert.py assets/orbit.gif output --target uno --width 120 --scale 2 --palette 16 --colors 8
    python source/convert.py assets/orbit.gif output --target uno --fit 240x240 --scale 2 --max-bytes 8000
    python source/convert.py assets/orbit.gif output --target mac --fit 240x240 --palette 565
    python source/convert.py assets/orbit.gif output --target uno mac --width 120 --scale 2
Output (struct animation::{name}, name = {gif}_{target}_{w}x{h}_palette_{p} or --name):
    --target uno / uno mac: output/{name}.h with the frames (FlashPlayer plays it, StreamPlayer can open it)
    --target mac:           output/mac/{name}.h with the frames (never compiled in, stream.py reads it)
                            + output/{name}.h without frames: size, colors, duration, path of the frames
    every output/{name}.h is added to output/selected.h, then selected.h is checked (source/check.py)
    every setting is written at the top of the files, whatever the name says
    getPath(): where stream.py finds the frames, relative to lib/GIF
Ranges: source/config.json (per target, both targets: what both allow); hard limits stop with an error,
        recommended ranges warn
Size (all sizes are on the screen, after scale):
    nothing                 size of the gif
    --width / --height      one: keeps the aspect ratio; both: exactly that (stretched)
    --resize F | FXxFY      factor of the gif size, e.g. 0.5 or 1.5x1 (any real number)
    --fit WxH               keeps the aspect ratio, as big as fits inside W x H
    --fill WxH              keeps the aspect ratio, covers W x H, the rest is cut (centered)
    --scale N | NXxNY       the board draws every stored pixel as NX x NY (integers): less data,
                            stored size = screen size / scale (rounded); not integer: use --resize
    --resample              nearest (pixel art) / bilinear / bicubic / lanczos (default)
Colors:
    --palette 16            one palette of up to 16 colors, 1 byte per run
    --palette 256           one palette of up to 256 colors, 2 bytes per run (uno only: palette stays in flash)
    --palette 565           every pixel its own RGB565 color, 3 bytes per run
    --colors N              fewer colors in the palette (16 / 256 only), --dither for gradients
Frame rate:
    --max-fps F             frames closer than 1/F are dropped, their time goes to the frame before
    --min-fps F             lower limit for --max-bytes, and a warning if the board cannot draw that fast
    --max-bytes N           too big: lower the fps (--reduce fps) or the size (--reduce size) until it fits
Frames:
    frame 0 and every keyframe-th frame (uno only): I frame (one window = the whole animation)
    other frames: the cheapest of changed windows / one window around every change / I frame
    cost = draw time (cycles) + bytes * byteCycles (flash read for uno, serial for mac)
"""


CONFIG_PATH = Path(__file__).resolve().parent/"config.json"
CONFIG = json.loads(CONFIG_PATH.read_text())
BOARD = CONFIG["board"]
TARGETS = CONFIG["targets"]
TARGET_BITS = {"uno": 1, "mac": 2} # GIF::TARGET_UNO / TARGET_MAC
MAC_DIR_NAME = "mac" # output/mac/: frames for stream.py only

TRANSPARENT_THRESHOLD = 128 # alpha < THRESHOLD -> background color
MIN_DELAY_MS = 20 # GIF delays below this are shown as DEFAULT_DELAY_MS (same as browsers)
DEFAULT_DELAY_MS = 100
MIN_STORED_SIZE = 4 # --max-bytes does not shrink below this
REDUCE_STEP = 0.85 # --max-bytes: fps or size times this per try

PALETTES = (16, 256, 565)
COLOR_MODES = {16: 0, 256: 1, 565: 2} # GIF::COLORS_16 / COLORS_256 / COLORS_565
RUN_LIMITS = {16: 271, 256: 256, 565: 256}
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
    background: tuple[int, int, int]
    screen_width: int
    screen_height: int
    crop: tuple[float, float, float, float] | None # in gif pixels
    scale_x: int
    scale_y: int
    stored_width: int
    stored_height: int
    resample: str
    max_fps: float | None
    min_fps: float | None
    keyframe: int
    tile: int


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


# ---------- gif ----------

def get_gif_frames(gif_path: Path) -> tuple[list[Image.Image], list[int]]:
    # every frame composited (Pillow applies the disposal methods), RGBA, gif size
    frames = []
    delays = []
    with Image.open(gif_path) as gif:
        for frame in ImageSequence.Iterator(gif):
            frames.append(frame.convert("RGBA"))
            delay = frame.info.get("duration", DEFAULT_DELAY_MS) or 0
            delays.append(int(delay) if delay >= MIN_DELAY_MS else DEFAULT_DELAY_MS)
    return frames, delays


def get_fps(delays: list[int]) -> float:
    return len(delays)/(sum(delays)/1000)


def get_limited_frames(frames: list, delays: list[int], max_fps: float | None) -> tuple[list, list[int]]:
    # frames closer than 1 / max_fps are dropped, their time goes to the frame shown before
    if max_fps is None:
        return list(frames), list(delays)
    min_delay = 1000/max_fps
    kept_frames = []
    kept_delays = []
    for frame, delay in zip(frames, delays):
        if kept_delays and kept_delays[-1] < min_delay:
            kept_delays[-1] += delay
        else:
            kept_frames.append(frame)
            kept_delays.append(delay)
    # the last one may still be too short: it joins the one before
    if len(kept_delays) > 1 and kept_delays[-1] < min_delay:
        kept_delays[-2] += kept_delays.pop()
        kept_frames.pop()
    return kept_frames, kept_delays


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


def get_screen_size_and_crop(gif_size: tuple[int, int], args: argparse.Namespace) -> tuple[int, int, tuple | None]:
    # size on the screen (after scale) and the part of the gif used (None = all)
    gif_width, gif_height = gif_size
    if args.fill is not None:
        box_width, box_height = args.fill
        ratio = max(box_width/gif_width, box_height/gif_height)
        crop_width = box_width/ratio
        crop_height = box_height/ratio
        left = (gif_width - crop_width)/2
        top = (gif_height - crop_height)/2
        return box_width, box_height, (left, top, left + crop_width, top + crop_height)
    if args.fit is not None:
        box_width, box_height = args.fit
        ratio = min(box_width/gif_width, box_height/gif_height)
        return max(1, round(gif_width*ratio)), max(1, round(gif_height*ratio)), None
    if args.resize is not None:
        factor_x, factor_y = args.resize
        return max(1, round(gif_width*factor_x)), max(1, round(gif_height*factor_y)), None
    if args.width is not None and args.height is not None:
        return args.width, args.height, None
    if args.width is not None:
        return args.width, max(1, round(gif_height*args.width/gif_width)), None
    if args.height is not None:
        return max(1, round(gif_width*args.height/gif_height)), args.height, None
    return gif_width, gif_height, None


def get_stored_size(screen_width: int, screen_height: int, scale_x: int, scale_y: int) -> tuple[int, int]:
    return max(1, round(screen_width/scale_x)), max(1, round(screen_height/scale_y))


def get_rendered_frames(frames: list[Image.Image], settings: Settings) -> list[Image.Image]:
    size = (settings.stored_width, settings.stored_height)
    rendered = []
    for frame in frames:
        if settings.crop is not None:
            frame = frame.crop(tuple(round(value) for value in settings.crop))
        rgba = frame.resize(size, RESAMPLES[settings.resample])
        canvas = Image.new("RGB", size, settings.background)
        mask = rgba.getchannel("A").point(lambda alpha: 255 if alpha >= TRANSPARENT_THRESHOLD else 0)
        canvas.paste(rgba.convert("RGB"), mask=mask)
        rendered.append(canvas)
    return rendered


# ---------- colors ----------

def get_palette_and_value_frames(frames: list[Image.Image], settings: Settings) -> tuple[list[int], list[list[int]]]:
    # 565: every pixel its RGB565 color, no palette; 16 / 256: one palette for every frame, values = indexes
    if settings.palette == 565:
        value_frames = []
        for frame in frames:
            value_frames.append([get_rgb565(*rgb) for rgb in get_pixels(frame)])
        return [], value_frames

    # quantize every frame stacked together; fast octree keeps small distinct colors
    color_count = min(settings.colors or settings.palette, settings.palette)
    width, height = frames[0].size
    stacked = Image.new("RGB", (width, height*len(frames)))
    for i, frame in enumerate(frames):
        stacked.paste(frame, (0, height*i))
    palette_img = stacked.quantize(colors=color_count, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    all_rgb = palette_img.getpalette()
    used_indexes = sorted(index for _, index in palette_img.getcolors(maxcolors=256))
    rgb_palette = [tuple(all_rgb[3*index:3*index + 3]) for index in used_indexes]
    palette = [get_rgb565(*rgb) for rgb in rgb_palette]

    if settings.dither:
        # Pillow dithers to a palette image; padding entries repeat color 0, so they map back to 0
        dither_img = Image.new("P", (1, 1))
        flat = [value for rgb in rgb_palette for value in rgb]
        dither_img.putpalette(flat + list(rgb_palette[0])*(256 - len(rgb_palette)))
        value_frames = []
        for frame in frames:
            indexes = get_pixels(frame.quantize(palette=dither_img, dither=Image.Dither.FLOYDSTEINBERG))
            value_frames.append([index if index < len(rgb_palette) else 0 for index in indexes])
        return palette, value_frames

    # nearest palette color, done here: Pillow may pick unused (padding) entries of its 256-color palette
    nearest = {}

    def get_nearest(rgb: tuple[int, int, int]) -> int:
        if rgb not in nearest:
            nearest[rgb] = min(range(len(rgb_palette)), key=lambda i: sum((a - b)**2 for a, b in zip(rgb, rgb_palette[i])))
        return nearest[rgb]

    return palette, [[get_nearest(rgb) for rgb in get_pixels(frame)] for frame in frames]


# ---------- encoding (also used by stream.py) ----------
# rect = (x, y, w, h)

def get_runs(value_frame: list[int], width: int, rect: tuple[int, int, int, int], palette: int) -> list[int]:
    x, y, w, h = rect
    pixels = [value_frame[(y + row)*width + x + col] for row in range(h) for col in range(w)]
    bytes_ = []
    position = 0
    while position < len(pixels):
        value = pixels[position]
        count = 1
        while position + count < len(pixels) and pixels[position + count] == value and count < RUN_LIMITS[palette]:
            count += 1
        if palette == 16:
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


def get_run_count(window_bytes: list[int], palette: int) -> int:
    run_bytes = window_bytes[4:]
    if palette == 256:
        return len(run_bytes)//2
    if palette == 565:
        return len(run_bytes)//3
    # 16: a run is 1 byte, or 2 when its high nibble is 0
    count = 0
    position = 0
    while position < len(run_bytes):
        position += 1 if run_bytes[position] >> 4 else 2
        count += 1
    return count


def get_windows_cost(windows_bytes: list[list[int]], rects: list[tuple[int, int, int, int]],
                     scale_x: int, scale_y: int, palette: int, byte_cycles: float) -> float:
    cost = 0
    for window_bytes, (_, _, w, h) in zip(windows_bytes, rects):
        cost += BOARD["windowCycles"] + BOARD["pixelCycles"]*scale_x*scale_y*w*h
        cost += BOARD["runCycles"]*get_run_count(window_bytes, palette) + byte_cycles*len(window_bytes)
    return cost


def get_changed_rects(previous: list[int], current: list[int], width: int, height: int, tile: int) -> list[tuple[int, int, int, int]]:
    # changed tiles -> spans in every tile row -> same spans in following rows merged -> shrunk to the changes
    changed = [previous[i] != current[i] for i in range(width*height)]
    tile_cols = (width + tile - 1)//tile
    tile_rows = (height + tile - 1)//tile

    def is_tile_changed(tile_col: int, tile_row: int) -> bool:
        for y in range(tile_row*tile, min((tile_row + 1)*tile, height)):
            for x in range(tile_col*tile, min((tile_col + 1)*tile, width)):
                if changed[y*width + x]:
                    return True
        return False

    tile_rects = []
    open_rects = {} # (first col, last col) -> [first row, last row]
    for tile_row in range(tile_rows):
        spans = []
        tile_col = 0
        while tile_col < tile_cols:
            if is_tile_changed(tile_col, tile_row):
                first_col = tile_col
                while tile_col + 1 < tile_cols and is_tile_changed(tile_col + 1, tile_row):
                    tile_col += 1
                spans.append((first_col, tile_col))
            tile_col += 1
        next_open = {}
        for span in spans:
            if span in open_rects:
                next_open[span] = [open_rects.pop(span)[0], tile_row]
            else:
                next_open[span] = [tile_row, tile_row]
        for span, rows in open_rects.items():
            tile_rects.append((span, rows))
        open_rects = next_open
    for span, rows in open_rects.items():
        tile_rects.append((span, rows))

    rects = []
    for (first_col, last_col), (first_row, last_row) in tile_rects:
        x0 = first_col*tile
        y0 = first_row*tile
        x1 = min((last_col + 1)*tile, width) - 1
        y1 = min((last_row + 1)*tile, height) - 1
        xs = [x for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if changed[y*width + x]]
        ys = [y for y in range(y0, y1 + 1) for x in range(x0, x1 + 1) if changed[y*width + x]]
        rects.append((min(xs), min(ys), max(xs) - min(xs) + 1, max(ys) - min(ys) + 1))
    return rects


def get_bounding_rect(rects: list[tuple[int, int, int, int]]) -> tuple[int, int, int, int]:
    x0 = min(x for x, _, _, _ in rects)
    y0 = min(y for _, y, _, _ in rects)
    x1 = max(x + w for x, _, w, _ in rects)
    y1 = max(y + h for _, y, _, h in rects)
    return x0, y0, x1 - x0, y1 - y0


def get_frame_bytes(previous: list[int] | None, current: list[int], width: int, height: int, delay: int,
                    scale_x: int, scale_y: int, palette: int, tile: int, byte_cycles: float) -> tuple[list[int], int, float]:
    # previous None: I frame; returns (bytes, type, cost in cycles)
    full_rects = [(0, 0, width, height)]
    options = [full_rects]
    if previous is not None:
        changed_rects = get_changed_rects(previous, current, width, height, tile)
        if not changed_rects:
            options = [[]]
        else:
            options.append(changed_rects)
            options.append([get_bounding_rect(changed_rects)])

    best = None
    for rects in options:
        if len(rects) > 255:
            continue
        windows_bytes = [list(rect) + get_runs(current, width, rect, palette) for rect in rects]
        cost = get_windows_cost(windows_bytes, rects, scale_x, scale_y, palette, byte_cycles) + byte_cycles*4
        # one window covering everything is an I frame
        frame_type = 1 if rects == full_rects else 0
        frame_bytes = [delay & 0xFF, delay >> 8, frame_type, len(rects)] + [byte for window_bytes in windows_bytes for byte in window_bytes]
        if best is None or cost < best[2]:
            best = (frame_bytes, frame_type, cost)
    return best


def get_all_frame_bytes(value_frames: list[list[int]], delays: list[int], settings: Settings) -> list[tuple[list[int], int, float]]:
    byte_cycles = settings.byte_cycles
    all_frame_bytes = []
    for i, current in enumerate(value_frames):
        is_keyframe = i == 0 or (settings.keyframe > 0 and i % settings.keyframe == 0)
        previous = None if is_keyframe else value_frames[i - 1]
        all_frame_bytes.append(get_frame_bytes(previous, current, settings.stored_width, settings.stored_height, delays[i],
                                               settings.scale_x, settings.scale_y, settings.palette, settings.tile, byte_cycles))
    return all_frame_bytes


def get_decoded_frame(frame_bytes: list[int], previous: list[int] | None, width: int, height: int, palette: int) -> list[int]:
    # the inverse of get_frame_bytes: values of the whole frame (stream.py reads .h files with it)
    values = list(previous) if previous is not None else [0]*(width*height)
    position = 4
    for _ in range(frame_bytes[3]):
        x, y, w, h = frame_bytes[position:position + 4]
        position += 4
        pixels = []
        while len(pixels) < w*h:
            if palette == 16:
                run = frame_bytes[position]
                count = run >> 4
                position += 1
                if count == 0:
                    count = frame_bytes[position] + 16
                    position += 1
                pixels += [run & 0x0F]*count
            elif palette == 256:
                pixels += [frame_bytes[position + 1]]*(frame_bytes[position] + 1)
                position += 2
            else:
                color = frame_bytes[position + 1] | (frame_bytes[position + 2] << 8)
                pixels += [color]*(frame_bytes[position] + 1)
                position += 3
        for row in range(h):
            values[(y + row)*width + x:(y + row)*width + x + w] = pixels[row*w:(row + 1)*w]
    return values


# ---------- estimates ----------

def get_frame_ms(frame_cost: float) -> float:
    return frame_cost*1000/BOARD["cpuHz"]


def get_reachable_fps(all_frame_bytes: list[tuple[list[int], int, float]], delays: list[int]) -> float:
    # every frame takes max(its delay, its draw / send time)
    total_ms = sum(max(delay, get_frame_ms(cost)) for (_, _, cost), delay in zip(all_frame_bytes, delays))
    return len(delays)/(total_ms/1000)


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


def get_frames_str(all_frame_bytes: list[tuple[list[int], int, float]], values_per_line: int = 16) -> str:
    frames_str = ""
    for i, (frame_bytes, frame_type, cost) in enumerate(all_frame_bytes):
        delay = frame_bytes[0] | (frame_bytes[1] << 8)
        frames_str += f"// frame {i}: {'I' if frame_type else 'P'}, {delay} ms, {frame_bytes[3]} windows, "
        frames_str += f"{len(frame_bytes)} bytes, ~{get_frame_ms(cost):.1f} ms to draw\n"
        for start_index in range(0, len(frame_bytes), values_per_line):
            frames_str += ", ".join(str(value) for value in frame_bytes[start_index:start_index + values_per_line]) + ",\n"
    return frames_str


def get_settings_str(settings: Settings, gif_info: str, delays: list[int], palette: list[int],
                     data_bytes: int, all_frame_bytes: list, reachable_fps: float) -> str:
    # every setting, whatever the file name says
    draw_ms = [get_frame_ms(cost) for _, _, cost in all_frame_bytes]
    crop = "none" if settings.crop is None else "x {:.1f} y {:.1f} w {:.1f} h {:.1f} (gif px)".format(
        settings.crop[0], settings.crop[1], settings.crop[2] - settings.crop[0], settings.crop[3] - settings.crop[1])
    color_count = "no palette" if settings.palette == 565 else f"{len(palette)} colors used"
    time_name = "draw" if "uno" in settings.targets else "draw + send"
    target_lines = [f"target      {target}: {TARGETS[target]['note']}" for target in settings.targets]
    lines = target_lines + [
        f"gif         {gif_info}",
        f"crop        {crop}",
        f"screen      {settings.stored_width*settings.scale_x} x {settings.stored_height*settings.scale_y} px "
        f"= stored {settings.stored_width} x {settings.stored_height} x scale {settings.scale_x} x {settings.scale_y}",
        f"resample    {settings.resample}",
        f"colors      palette {settings.palette}, {color_count}, dither {'on' if settings.dither else 'off'}, "
        f"background {'%02X%02X%02X' % settings.background}",
        f"frames      {len(delays)} frames, {sum(delays)} ms, {get_fps(delays):.1f} fps "
        f"(max-fps {'none' if settings.max_fps is None else f'{settings.max_fps:g}' if settings.max_fps == round(settings.max_fps, 1) else f'{settings.max_fps:.2f}'}, "
        f"min-fps {'none' if settings.min_fps is None else f'{settings.min_fps:g}'})",
        f"keyframe    {settings.keyframe or 'frame 0 only'}, tile {settings.tile}",
        f"data        {data_bytes} bytes",
        f"{time_name:<11} {min(draw_ms):.1f} ~ {max(draw_ms):.1f} ms per frame (estimated), reaches {reachable_fps:.1f} fps",
    ]
    return "// settings:\n" + "".join(f"//   {line}\n" for line in lines)


def get_target_bits(targets: list[str]) -> int:
    return sum(TARGET_BITS[target] for target in targets)


def get_target_label(targets: list[str]) -> str:
    return "_".join(targets)


def get_offsets(all_frame_bytes: list[tuple[list[int], int, float]]) -> tuple[list[int], int]:
    offsets = []
    offset = 0
    for frame_bytes, _, _ in all_frame_bytes:
        offsets.append(offset)
        offset += len(frame_bytes)
    return offsets, offset


def get_c_start_str(name: str, command: str, gif_path: Path, settings_str: str) -> str:
    c_str = ""
    c_str += f"#ifndef ANIMATION_{name.upper()}_H\n"
    c_str += f"#define ANIMATION_{name.upper()}_H\n\n"
    c_str += "// generated by convert.py, do not edit\n"
    c_str += f"// {command}\n"
    c_str += check.get_format_str()
    c_str += check.get_source_str(gif_path)
    c_str += settings_str + "\n"
    c_str += "#include <stdint.h>\n"
    c_str += "#if defined(__AVR__)\n"
    c_str += "#include <avr/pgmspace.h>\n"
    c_str += "#endif\n"
    c_str += "#ifndef PROGMEM\n"
    c_str += "#define PROGMEM\n"
    c_str += "#endif\n\n"
    c_str += "namespace animation {\n"
    return c_str


def get_c_struct_str(name: str, usage: str, c_const_str: str) -> str:
    c_struct_str = ""
    c_struct_str += "".join(f"// {line}\n" for line in usage.split("\n"))
    c_struct_str += f"struct {name} {{\n"
    c_struct_str += indent(c_const_str.rstrip("\n")) + "\n"
    c_struct_str += "};\n"
    return indent(c_struct_str.rstrip("\n")) + "\n}\n\n#endif\n"


def get_c_info_str(settings: Settings, palette: list[int], all_frame_bytes: list, delays: list[int], path: str) -> str:
    # what every output/{name}.h has, with or without the frames
    color_count = 0 if settings.palette == 565 else len(palette)
    c_info_str = ""
    c_info_str += f"static constexpr uint8_t target = {get_target_bits(settings.targets)}; // "
    c_info_str += " | ".join(f"GIF::TARGET_{target.upper()}" for target in settings.targets) + "\n"
    c_info_str += f"static constexpr uint8_t width = {settings.stored_width};\n"
    c_info_str += f"static constexpr uint8_t height = {settings.stored_height};\n"
    c_info_str += f"static constexpr uint8_t scaleX = {settings.scale_x};\n"
    c_info_str += f"static constexpr uint8_t scaleY = {settings.scale_y};\n"
    c_info_str += f"static constexpr uint8_t colorMode = {COLOR_MODES[settings.palette]}; // GIF::COLORS_{settings.palette}\n"
    c_info_str += f"static constexpr uint16_t colorCount = {color_count};\n"
    c_info_str += f"static constexpr uint16_t frameCount = {len(all_frame_bytes)};\n"
    c_info_str += f"static constexpr uint32_t durationMs = {sum(delays)};\n"
    c_info_str += "\n"
    c_info_str += get_c_getter_str("where stream.py finds the frames, relative to lib/GIF (GIF::StreamPlayer sends it)",
                                   "char", "getPath", f'static const char path[] PROGMEM = "{path}";\n', "path")
    return c_info_str


def get_c_data_str(name: str, command: str, gif_path: Path, settings: Settings, settings_str: str,
                   palette: list[int], all_frame_bytes: list, delays: list[int], path: str) -> str:
    # the whole animation: info + palette + frames
    offsets, offset = get_offsets(all_frame_bytes)
    # uint16_t for FlashPlayer; a frames file for stream.py only may be bigger (it never goes into flash)
    offset_type = "uint16_t"
    if offset > 0xFFFF:
        if "uno" in settings.targets:
            raise ValueError(f"Frame data is {offset} bytes, offsets must fit in uint16_t (65535): "
                             "use --max-bytes, a smaller size or fewer frames.")
        offset_type = "uint32_t"

    c_const_str = get_c_info_str(settings, palette, all_frame_bytes, delays, path)
    c_const_str += "\n"
    c_const_str += get_c_getter_str("RGB565 colors (none for palette 565)", "uint16_t", "getPalette",
                                    get_c_table_str("uint16_t", "palette", [f"0x{color:04X}" for color in palette], values_per_line=8), "palette")
    c_const_str += "\n"
    c_const_str += get_c_getter_str("first byte in getFrames() of frame i", offset_type, "getOffsets",
                                    get_c_table_str(offset_type, "offsets", offsets, values_per_line=12), "offsets")
    c_const_str += "\n"
    c_const_str += get_c_getter_str("every frame, format: lib/GIF/decoder.h", "uint8_t", "getFrames",
                                    get_c_list_str("uint8_t", "frames", get_frames_str(all_frame_bytes)), "frames")

    usages = []
    if "uno" in settings.targets:
        usages.append(f"GIF::FlashPlayer::start<animation::{name}>(x, y)")
    if "mac" in settings.targets:
        usages.append(f"GIF::StreamPlayer::open<animation::{name}>(x, y), with stream.py running")
    if settings.targets == ["mac"]:
        usages = [f"frames for stream.py only, never included: GIF::StreamPlayer::open<animation::{name}>(x, y)"]
    return get_c_start_str(name, command, gif_path, settings_str) + get_c_struct_str(name, "\n".join(usages), c_const_str)


def get_c_descriptor_str(name: str, command: str, gif_path: Path, settings: Settings, settings_str: str,
                         palette: list[int], all_frame_bytes: list, delays: list[int], path: str) -> str:
    # --target mac: no frames, only what the board needs to open it
    c_const_str = get_c_info_str(settings, palette, all_frame_bytes, delays, path)
    usage = f"GIF::StreamPlayer::open<animation::{name}>(x, y), with stream.py running\nthe frames are in {path}"
    return get_c_start_str(name, command, gif_path, settings_str) + get_c_struct_str(name, usage, c_const_str)


def get_name(gif_path: Path, settings: Settings) -> str:
    gif_name = "".join(c if c.isalnum() else "_" for c in gif_path.stem.lower())
    if gif_name[0].isdigit():
        gif_name = "_" + gif_name
    screen_width = settings.stored_width*settings.scale_x
    screen_height = settings.stored_height*settings.scale_y
    return f"{gif_name}_{get_target_label(settings.targets)}_{screen_width}x{screen_height}_palette_{settings.palette}"


def get_lib_relative_str(path: Path) -> str:
    # path as stream.py looks for it: relative to lib/GIF
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


def get_non_negative_int(value: str) -> int:
    try:
        number = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be an integer, got {value!r}")
    if number < 0:
        raise argparse.ArgumentTypeError(f"must be >= 0, got {number}")
    return number


def get_positive_float(value: str) -> float:
    try:
        number = float(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"must be a number, got {value!r}")
    if not number > 0 or math.isinf(number):
        raise argparse.ArgumentTypeError(f"must be > 0, got {value!r}")
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
        description="Convert .gif to a c header for GIF::FlashPlayer (--target uno) and / or GIF::StreamPlayer (--target mac).",
        epilog=(
            "examples:\n"
            "  python source/convert.py assets/orbit.gif output --target uno --width 120 --scale 2 --palette 16 --colors 8\n"
            "  python source/convert.py assets/orbit.gif output --target uno --fit 240x240 --scale 2 --max-bytes 8000\n"
            "  python source/convert.py assets/orbit.gif output --target mac --fit 240x240 --palette 565\n"
            "  python source/convert.py assets/orbit.gif output --target uno mac --width 120 --scale 2\n"
            "  python source/check.py    (checks output/selected.h)\n"
            "ranges and defaults per target: source/config.json"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("gif_path", help="animation, .gif")
    parser.add_argument("output_dir_path", help="dir for the generated .h (made if missing)")
    parser.add_argument("--target", choices=sorted(TARGETS), nargs="+", required=True,
                        help="uno: frames in flash (FlashPlayer), mac: frames on the computer (StreamPlayer + stream.py), "
                             "both: within the ranges of both")
    parser.add_argument("--name", type=get_identifier, help="file / struct name instead of the default (every setting is in the file anyway)")

    size_group = parser.add_argument_group("size on the screen (one way; --width and --height may go together)")
    size_group.add_argument("--width", type=get_positive_int, help="px; alone: keeps the aspect ratio")
    size_group.add_argument("--height", type=get_positive_int, help="px; alone: keeps the aspect ratio")
    size_group.add_argument("--resize", type=get_float_pair, help="F or FXxFY: factor of the gif size, any real number")
    size_group.add_argument("--fit", type=get_box, help="WxH: keeps the aspect ratio, inside the box")
    size_group.add_argument("--fill", type=get_box, help="WxH: keeps the aspect ratio, covers the box, the rest is cut")
    size_group.add_argument("--scale", type=get_int_pair, default=(1, 1), help="N or NXxNY: the board repeats every stored pixel (default 1)")
    size_group.add_argument("--resample", choices=sorted(RESAMPLES), default="lanczos", help="nearest for pixel art (default lanczos)")

    color_group = parser.add_argument_group("colors")
    color_group.add_argument("--palette", type=int, choices=PALETTES, help="16 / 256 / 565 (default per target, config.json)")
    color_group.add_argument("--colors", type=get_positive_int, help="at most this many colors in the palette (16 / 256 only)")
    color_group.add_argument("--dither", action="store_true", help="Floyd-Steinberg dithering (16 / 256): smoother gradients, more data")
    color_group.add_argument("--background", type=get_color, default=(0, 0, 0), help="color behind transparent pixels (default 000000)")

    frame_group = parser.add_argument_group("frames")
    frame_group.add_argument("--max-fps", type=get_positive_float, help="drop frames closer than 1/F")
    frame_group.add_argument("--min-fps", type=get_positive_float, help="lower limit for --max-bytes; warns if the board cannot reach it")
    frame_group.add_argument("--max-bytes", type=get_positive_int, help="lower the fps or the size until the data fits")
    frame_group.add_argument("--reduce", choices=("fps", "size"), default="size", help="what --max-bytes lowers first (default size)")
    frame_group.add_argument("--keyframe", type=get_non_negative_int, default=0, help="uno: I frame every N frames, to catch up when late (default 0)")
    frame_group.add_argument("--tile", type=get_positive_int, default=8, help="tile size in px for finding changed areas (default 8)")
    return parser


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

    gif_path = Path(args.gif_path).expanduser()
    if not gif_path.is_file():
        parser.error(f"gif_path not found: {gif_path}")
    if gif_path.suffix.lower() != ".gif":
        parser.error(f"gif_path must be .gif, got {gif_path.name}")
    try:
        with Image.open(gif_path) as gif:
            if gif.format != "GIF":
                parser.error(f"gif_path is not a GIF file: {gif_path}")
    except OSError as e:
        parser.error(f"not able to read the gif: {e}")

    if args.palette is None:
        args.palette = target["defaultPalette"]
    if args.palette not in target["palettes"]:
        parser.error(f"--palette {args.palette} is not possible for --target {target_str} (config.json: {target['palettes']})")
    if args.palette not in target["recommendedPalettes"]:
        warnings.append(f"--palette {args.palette} with --target {target_str} makes a lot of data")
    if args.colors is not None:
        if args.palette == 565:
            warnings.append("--colors is ignored with --palette 565")
        elif not 2 <= args.colors <= args.palette:
            parser.error(f"--colors must be 2 ~ {args.palette} with --palette {args.palette}, got {args.colors}")
    if args.dither and args.palette == 565:
        warnings.append("--dither is ignored with --palette 565")

    if max(args.scale) > CONFIG["maxScale"]:
        parser.error(f"--scale must be at most {CONFIG['maxScale']} (config.json), got {args.scale}")
    if args.min_fps is not None and args.max_fps is not None and args.min_fps > args.max_fps:
        parser.error(f"--min-fps {args.min_fps} is above --max-fps {args.max_fps}")
    if args.max_fps is not None and args.max_fps > CONFIG["recommendedMaxFps"]:
        warnings.append(f"--max-fps {args.max_fps} is above {CONFIG['recommendedMaxFps']}: the board rarely gets there")
    if args.keyframe and not target["useKeyframe"]:
        warnings.append(f"--keyframe is ignored with --target {target_str} (the computer sends the difference to the frame due)")
        args.keyframe = 0
    return warnings


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
        "useKeyframe": uno["useKeyframe"],
    }


def get_settings(args: argparse.Namespace, gif_size: tuple[int, int], size_factor: float, max_fps: float | None) -> Settings:
    screen_width, screen_height, crop = get_screen_size_and_crop(gif_size, args)
    screen_width = max(1, round(screen_width*size_factor))
    screen_height = max(1, round(screen_height*size_factor))
    scale_x, scale_y = args.scale
    stored_width, stored_height = get_stored_size(screen_width, screen_height, scale_x, scale_y)
    return Settings(
        targets=args.target, byte_cycles=get_target_config(args.target)["byteCycles"],
        palette=args.palette, colors=args.colors, dither=args.dither, background=args.background,
        screen_width=screen_width, screen_height=screen_height, crop=crop, scale_x=scale_x, scale_y=scale_y,
        stored_width=stored_width, stored_height=stored_height, resample=args.resample,
        max_fps=max_fps, min_fps=args.min_fps, keyframe=args.keyframe, tile=args.tile,
    )


def check_settings(settings: Settings) -> tuple[list[str], list[str]]:
    errors = []
    warnings = []
    width = settings.stored_width*settings.scale_x
    height = settings.stored_height*settings.scale_y
    if max(settings.stored_width, settings.stored_height) > CONFIG["maxStoredSize"]:
        errors.append(f"stored {settings.stored_width} x {settings.stored_height} px: each must be <= {CONFIG['maxStoredSize']} (uint8_t)")
    if (settings.scale_x, settings.scale_y) != (1, 1) and settings.stored_width > CONFIG["maxScaledWidth"]:
        errors.append(f"stored width {settings.stored_width} must be <= {CONFIG['maxScaledWidth']} when scaled (row buffer of GIF::Decoder)")
    if width > CONFIG["screen"]["width"] or height > CONFIG["screen"]["height"]:
        errors.append(f"{width} x {height} px on the screen: bigger than {CONFIG['screen']['width']} x {CONFIG['screen']['height']}, "
                      "the players refuse it")
    if (width, height) != (settings.screen_width, settings.screen_height):
        warnings.append(f"{settings.screen_width} x {settings.screen_height} is not a multiple of --scale "
                        f"{settings.scale_x}x{settings.scale_y}: drawn as {width} x {height}")
    return errors, warnings


def convert_gif_to_h(args: argparse.Namespace, command: str, warnings: list[str]) -> bool:
    # returns True if selected.h passes the check
    gif_path = Path(args.gif_path).expanduser().resolve()
    output_dir_path = Path(args.output_dir_path).expanduser().resolve()
    if output_dir_path.exists() and not output_dir_path.is_dir():
        raise NotADirectoryError(f"Not a dir: {output_dir_path}")
    output_dir_path.mkdir(parents=True, exist_ok=True)

    gif_frames, gif_delays = get_gif_frames(gif_path)
    gif_size = gif_frames[0].size
    gif_info = f"{gif_size[0]} x {gif_size[1]} px, {len(gif_delays)} frames, {sum(gif_delays)} ms, {get_fps(gif_delays):.1f} fps"
    target = get_target_config(args.target)

    # --max-bytes: try, lower the fps or the size, try again
    size_factor = 1.0
    max_fps = args.max_fps
    reduce_order = [args.reduce, "size" if args.reduce == "fps" else "fps"]
    while True:
        frames, delays = get_limited_frames(gif_frames, gif_delays, max_fps)
        settings = get_settings(args, gif_size, size_factor, max_fps)
        errors, size_warnings = check_settings(settings)
        if errors:
            raise ValueError("; ".join(errors))
        palette, value_frames = get_palette_and_value_frames(get_rendered_frames(frames, settings), settings)
        all_frame_bytes = get_all_frame_bytes(value_frames, delays, settings)
        data_bytes = sum(len(frame_bytes) for frame_bytes, _, _ in all_frame_bytes) + 2*len(all_frame_bytes) + 2*len(palette)
        if args.max_bytes is None or data_bytes <= args.max_bytes:
            break

        print(f"[Info] {data_bytes} bytes > --max-bytes {args.max_bytes} "
              f"({settings.stored_width} x {settings.stored_height} px, {get_fps(delays):.1f} fps), lowering")
        reduced = False
        for way in reduce_order:
            if way == "fps":
                lower_fps = get_fps(delays)*REDUCE_STEP
                if len(delays) > 1 and (args.min_fps is None or lower_fps >= args.min_fps):
                    max_fps = lower_fps
                    reduced = True
            elif min(settings.stored_width, settings.stored_height)*REDUCE_STEP >= MIN_STORED_SIZE:
                size_factor *= REDUCE_STEP
                reduced = True
            if reduced:
                break
        if not reduced:
            raise ValueError(f"cannot fit --max-bytes {args.max_bytes}: smallest try is {data_bytes} bytes "
                             f"(--min-fps {args.min_fps} and size {MIN_STORED_SIZE} px reached)")
    warnings += size_warnings

    reachable_fps = get_reachable_fps(all_frame_bytes, delays)
    fps = get_fps(delays)
    if reachable_fps < fps*0.95:
        way = "FlashPlayer jumps to I frames or slows down" if "uno" in args.target else "stream.py skips frames"
        warnings.append(f"the board reaches ~{reachable_fps:.1f} fps of {fps:.1f} ({way})")
    if args.min_fps is not None and reachable_fps < args.min_fps:
        warnings.append(f"the board reaches ~{reachable_fps:.1f} fps, below --min-fps {args.min_fps}")
    if target["recommendedMaxBytes"] is not None and data_bytes > target["recommendedMaxBytes"]:
        warnings.append(f"{data_bytes} bytes is above {target['recommendedMaxBytes']} (config.json) for --target {' '.join(args.target)}")

    name = args.name or get_name(gif_path, settings)
    output_file_path = output_dir_path/(name + ".h")
    settings_str = get_settings_str(settings, gif_info, delays, palette, data_bytes, all_frame_bytes, reachable_fps)
    if args.target == ["mac"]:
        # frames in output/mac/ (never included), the board gets a small file with the path
        data_file_path = output_dir_path/MAC_DIR_NAME/(name + ".h")
        path = get_lib_relative_str(data_file_path)
        files = [
            (data_file_path, get_c_data_str(name, command, gif_path, settings, settings_str, palette, all_frame_bytes, delays, path)),
            (output_file_path, get_c_descriptor_str(name, command, gif_path, settings, settings_str, palette, all_frame_bytes, delays, path)),
        ]
    else:
        path = get_lib_relative_str(output_file_path)
        files = [(output_file_path, get_c_data_str(name, command, gif_path, settings, settings_str, palette, all_frame_bytes, delays, path))]
    if path.startswith("/"):
        warnings.append(f"{path} is outside lib/GIF: stream.py only opens files inside lib/GIF")

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
        print(f"[Warning] GIF.h includes {check.LIB_DIR_PATH.name}/output/{check.SELECTED_NAME}, not this dir")
    return check.print_check(output_dir_path)


def main():
    parser = get_parser()
    args = parser.parse_args()
    warnings = check_args(parser, args)
    command = " ".join([Path(args.gif_path).name] + sys.argv[2:])
    try:
        is_fine = convert_gif_to_h(args, command, warnings)
    except (ValueError, OSError) as e:
        # same form as the argument errors: usage + message, no traceback
        parser.error(str(e))
    if not is_fine:
        sys.exit(1)


if __name__ == "__main__":
    main()
