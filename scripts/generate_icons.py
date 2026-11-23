#!/usr/bin/env python3
# pyright: strict
"""
Generate the icon sets in assets/icons/<set>/: a white 90x90 PNG per icon, its shape in the alpha channel, which the
screen recolours to the theme (src/UI/Widgets/Display/Icon.cpp).

Every set has the same icons under the same names, so a set can be chosen in the settings without anything going
missing. Each icon is named for what it means where it is used (`fan.png`, `estop.png`), not for its picture, and
ICONS below says which picture each set draws for it:

  - a glyph of the set's icon font, by its name in the font (Material Symbols or Phosphor), or
  - a picture drawn here (Drawing), for what no font has: a nozzle, extruding, a heated bed, a bed mesh.

The fonts are downloaded, at the pinned versions below, into a cache folder (out/icon_fonts by default) the first time.
Their licences are copied beside the icons drawn from them (LICENSE.txt).

Usage:
  scripts/generate_icons.py               # every set
  scripts/generate_icons.py --set material
  scripts/generate_icons.py --sheet out/icons.png   # also a contact sheet of every set, to look over

Needs only Pillow (requirements.txt), built with FreeType and Raqm for variable fonts.
"""

from __future__ import annotations

import argparse
import math
import re
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from PIL import Image, ImageDraw, ImageFont, ImageOps

from common.project import ASSETS, ROOT, complete, complete_files, complete_with

ICONS_DIR = ASSETS / "icons"
DEFAULT_CACHE = ROOT / "out" / "icon_fonts"

SIZE = 90  # Pixels square, as the screen expects
SUPERSAMPLE = 8  # Drawings are drawn this many times larger, then scaled down, for smooth edges

# ---------------------------------------------------------------------------
# Fonts
# ---------------------------------------------------------------------------

# google/material-design-icons, 2026-10-23
MATERIAL_COMMIT = "10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7"
MATERIAL_URL = f"https://raw.githubusercontent.com/google/material-design-icons/{MATERIAL_COMMIT}"
# @phosphor-icons/web, 2025-03-31
PHOSPHOR_URL = "https://unpkg.com/@phosphor-icons/web@2.1.2"


@dataclass(frozen=True)
class FontSource:
    """An icon font: where to download it and its glyph names, and how to place a glyph in the icon's square."""

    key: str  # Its name in the cache folder
    font_url: str  # The font file
    names_url: str  # Its glyph names: a Material .codepoints file or a Phosphor style.css
    license_url: str  # Its licence
    license_name: str  # E.g. "Apache License 2.0"
    homepage: str  # Where it comes from, for LICENSE.txt
    # A glyph whose ink is known, and that ink as fractions of the square: it places every glyph of the font
    reference_glyph: str
    reference_box: tuple[float, float, float, float]


MATERIAL_OUTLINED = FontSource(
    key="material_symbols_outlined",
    font_url=f"{MATERIAL_URL}/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf",
    names_url=f"{MATERIAL_URL}/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.codepoints",
    license_url=f"{MATERIAL_URL}/LICENSE",
    license_name="Apache License 2.0",
    homepage="https://github.com/google/material-design-icons (Material Symbols Outlined)",
    reference_glyph="cancel",  # A circle 2 to 22 of 24
    reference_box=(2 / 24, 2 / 24, 22 / 24, 22 / 24),
)

MATERIAL_ROUNDED = FontSource(
    key="material_symbols_rounded",
    font_url=f"{MATERIAL_URL}/variablefont/MaterialSymbolsRounded%5BFILL,GRAD,opsz,wght%5D.ttf",
    names_url=f"{MATERIAL_URL}/variablefont/MaterialSymbolsRounded%5BFILL,GRAD,opsz,wght%5D.codepoints",
    license_url=f"{MATERIAL_URL}/LICENSE",
    license_name="Apache License 2.0",
    homepage="https://github.com/google/material-design-icons (Material Symbols Rounded)",
    reference_glyph="cancel",  # A circle 2 to 22 of 24, filled or not
    reference_box=(2 / 24, 2 / 24, 22 / 24, 22 / 24),
)


def phosphor(weight: str, file_suffix: str, stroke: float) -> FontSource:
    """Phosphor in `weight` (regular, bold, …), whose font file is Phosphor<file_suffix>.ttf and whose lines are
    `stroke` units wide of its 256"""
    # Its square's sides are at 40 and 216 of 256, the line centred on them
    inset = (40 - stroke / 2) / 256
    return FontSource(
        key=f"phosphor_{weight}",
        font_url=f"{PHOSPHOR_URL}/src/{weight}/Phosphor{file_suffix}.ttf",
        names_url=f"{PHOSPHOR_URL}/src/{weight}/style.css",
        license_url=f"{PHOSPHOR_URL}/LICENSE",
        license_name="MIT License",
        homepage=f"https://phosphoricons.com (Phosphor {weight}, @phosphor-icons/web 2.1.2)",
        reference_glyph="square",
        reference_box=(inset, inset, 1 - inset, 1 - inset),
    )


PHOSPHOR_REGULAR = phosphor("regular", "", stroke=16)
PHOSPHOR_BOLD = phosphor("bold", "-Bold", stroke=24)


class Font:
    """An icon font downloaded into the cache, ready to draw its glyphs"""

    def __init__(self, source: FontSource, cache: Path, variation: list[float] | None):
        """Download `source` into `cache` if it isn't there, with the variable font's axes set to `variation`"""
        self.source = source
        folder = cache / source.key
        folder.mkdir(parents=True, exist_ok=True)
        self.font_path = download(source.font_url, folder / "font.ttf")
        self.license_text = download(source.license_url, folder / "LICENSE").read_text(encoding="utf-8")
        names_text = download(source.names_url, folder / "names.txt").read_text(encoding="utf-8")
        self.codepoints = parse_names(names_text)
        self.variation = variation
        self.offset = self._place()

    def has(self, name: str) -> bool:
        """Whether the font has a glyph called `name`"""
        return name in self.codepoints

    def draw(self, name: str) -> Image.Image:
        """The glyph called `name` as an SIZE x SIZE mask"""
        mask = Image.new("L", (SIZE, SIZE), 0)
        ImageDraw.Draw(mask).text(self.offset, chr(self.codepoints[name]), font=self._font(SIZE), fill=255,
                                  anchor="ls")
        return mask

    def _font(self, size: int) -> ImageFont.FreeTypeFont:
        """The font at `size` pixels, its axes set"""
        font = ImageFont.truetype(str(self.font_path), size)
        if self.variation is not None:
            font.set_variation_by_axes(self.variation)
        return font

    def _place(self) -> tuple[float, float]:
        """Where to put a glyph's baseline so the reference glyph's ink lands on its known box"""
        # Measured from the ink drawn, with room all round: a font's own box can be its advance, not its ink
        canvas = Image.new("L", (3 * SIZE, 3 * SIZE), 0)
        ImageDraw.Draw(canvas).text((SIZE, 2 * SIZE), chr(self.codepoints[self.source.reference_glyph]),
                                    font=self._font(SIZE), fill=255, anchor="ls")
        ink = canvas.getbbox()
        assert ink is not None, "the reference glyph draws nothing"
        left, top, _, _ = ink
        box = self.source.reference_box
        return box[0] * SIZE - (left - SIZE), box[1] * SIZE - (top - 2 * SIZE)


def download(url: str, path: Path) -> Path:
    """`path`, downloading `url` to it first if it isn't there"""
    if not path.exists():
        print(f"Downloading {url}")
        with urllib.request.urlopen(url, timeout=60) as response:
            path.write_bytes(response.read())
    return path


def parse_names(text: str) -> dict[str, int]:
    """Glyph names and code points from a Material .codepoints file ("name hex" lines) or a Phosphor style.css"""
    css = re.findall(r'\.ph-([a-z0-9-]+):before\s*\{\s*content:\s*"\\([0-9a-f]+)"', text)
    if css:
        return {name: int(code, 16) for name, code in css}
    names: dict[str, int] = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 2:
            names[parts[0]] = int(parts[1], 16)
    return names


# ---------------------------------------------------------------------------
# Drawings: icons no font has, drawn on the 24 x 24 grid Material Symbols uses
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class Pen:
    """How a set draws: its line width on the 24-unit grid, and whether shapes are filled"""

    width: float  # Line width in grid units: Material's is 2
    filled: bool  # Closed shapes filled, as a filled icon set's are


class Canvas:
    """A supersampled drawing on the 24-unit icon grid, with round ends and joins"""

    def __init__(self, pen: Pen):
        """A blank drawing in `pen`'s style"""
        self.pen = pen
        self.scale = SIZE * SUPERSAMPLE / 24
        self.mask = Image.new("L", (SIZE * SUPERSAMPLE, SIZE * SUPERSAMPLE), 0)
        self.draw = ImageDraw.Draw(self.mask)

    def _xy(self, points: list[tuple[float, float]]) -> list[tuple[float, float]]:
        """`points` from grid units to pixels"""
        return [(x * self.scale, y * self.scale) for x, y in points]

    def line(self, points: list[tuple[float, float]], ink: int = 255, width: float | None = None) -> None:
        """A line through `points`, the pen's width unless `width` is given, round at its ends and corners"""
        w = (self.pen.width if width is None else width) * self.scale
        xy = self._xy(points)
        self.draw.line(xy, fill=ink, width=round(w), joint="curve")
        for x, y in (xy[0], xy[-1]):
            self.draw.ellipse((x - w / 2, y - w / 2, x + w / 2, y + w / 2), fill=ink)

    def shape(self, points: list[tuple[float, float]], always_filled: bool = False) -> None:
        """A closed shape through `points`: outlined, and filled too if the pen fills (or `always_filled`). The outline
        is centred on the points either way, so a shape is the same size in every set."""
        if self.pen.filled or always_filled:
            self.draw.polygon(self._xy(points), fill=255)
        self.line(points + [points[0]])

    def cut(self, points: list[tuple[float, float]], width: float) -> None:
        """Erase a line `width` units wide through `points`: a gap in a filled shape"""
        self.line(points, ink=0, width=width)

    def mask_image(self) -> Image.Image:
        """The drawing as an SIZE x SIZE mask"""
        return ImageOps.fit(self.mask, (SIZE, SIZE), Image.Resampling.LANCZOS)  # square to square: a plain resize


def rounded_rect(x0: float, y0: float, x1: float, y1: float, r: float, steps: int = 6) -> list[tuple[float, float]]:
    """The points of a rectangle with corners rounded to radius `r`"""
    points: list[tuple[float, float]] = []
    for cx, cy, start in ((x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0), (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)):
        for i in range(steps + 1):
            a = math.radians(start + 90 * i / steps)
            points.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return points


def draw_nozzle(c: Canvas, top: float = 2.0, height: float = 19.0) -> None:
    """A hotend from `top` down `height` units: the filament's inlet, the heater block, and the nozzle under it
    narrowing to its tip"""

    def y(units: float) -> float:
        """`units` down a hotend drawn 19 units tall"""
        return top + units * height / 19.0

    inlet = rounded_rect(9.5, y(0), 14.5, y(4), 0.6)
    block = rounded_rect(3.5, y(4), 20.5, y(11.5), 1.2)
    cone = [(7.5, y(11.5)), (16.5, y(11.5)), (13.3, y(18)), (10.7, y(18))]
    if c.pen.filled:
        for points in (inlet, block, cone):
            c.shape(points)
        # Gaps between the parts, so they read as parts and not one blob
        for row, (x0, x1) in ((y(4), (8.5, 15.5)), (y(11.5), (6.5, 17.5))):
            c.cut([(x0, row), (x1, row)], width=c.pen.width * 0.7)
    else:
        c.shape(inlet)
        c.shape(block)
        c.line([cone[0], cone[3], cone[2], cone[1]])  # Open at the top: the block's edge closes it


def draw_tools(c: Canvas) -> None:
    """The tools: a hotend"""
    draw_nozzle(c)


def draw_arrow(c: Canvas, x: float, y_from: float, y_to: float, head: float = 3.2) -> None:
    """An arrow at `x` from `y_from` to `y_to`, its head at `y_to`"""
    c.line([(x, y_from), (x, y_to)])
    sign = 1 if y_to > y_from else -1
    c.line([(x - head, y_to - sign * head), (x, y_to), (x + head, y_to - sign * head)])


def draw_extrude(c: Canvas) -> None:
    """Extruding: an arrow pushing down into a hotend"""
    draw_nozzle(c, top=9, height=13.5)
    draw_arrow(c, 12, 1.2, 6.8)


def draw_retract(c: Canvas) -> None:
    """Retracting: an arrow pulling up out of a hotend"""
    draw_nozzle(c, top=9, height=13.5)
    draw_arrow(c, 12, 6.8, 1.2)


def wave(x: float, y0: float, y1: float, amplitude: float = 1.3) -> list[tuple[float, float]]:
    """The points of a wavy vertical line at `x` from `y0` to `y1`: heat rising"""
    steps = 24
    return [(x + amplitude * math.sin(2 * math.pi * i / steps * 1.5), y0 + (y1 - y0) * i / steps)
            for i in range(steps + 1)]


def draw_bed(c: Canvas) -> None:
    """A heated bed: a plate with heat rising from it"""
    c.shape(rounded_rect(2.5, 15.5, 21.5, 20, 1))
    for x in (7, 12, 17):
        c.line(wave(x, 3, 12))


def draw_heightmap(c: Canvas) -> None:
    """A bed mesh: a grid seen in perspective, as a probed bed is"""
    back_left, back_right = (6.5, 6), (17.5, 6)
    front_left, front_right = (2, 19), (22, 19)

    def lerp(a: tuple[float, float], b: tuple[float, float], t: float) -> tuple[float, float]:
        return a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t

    c.shape([back_left, back_right, front_right, front_left])
    # Lines across, closer together towards the back, and lines from front to back
    lines: list[list[tuple[float, float]]] = []
    for t in (0.3, 0.62):
        lines.append([lerp(back_left, front_left, t), lerp(back_right, front_right, t)])
    for t in (1 / 3, 2 / 3):
        lines.append([lerp(back_left, back_right, t), lerp(front_left, front_right, t)])
    for points in lines:
        if c.pen.filled:
            c.cut(points, width=c.pen.width * 0.8)
        else:
            c.line(points)


DRAWINGS: dict[str, Callable[[Canvas], None]] = {
    "tools": draw_tools,
    "extrude": draw_extrude,
    "retract": draw_retract,
    "bed": draw_bed,
    "heightmap": draw_heightmap,
}

# ---------------------------------------------------------------------------
# The icons
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class Icon:
    """One icon: what it means, and the glyph each font family draws for it (or a drawing of its own)"""

    meaning: str  # Where it is used, for the reader
    material: str | None = None  # Its Material Symbols glyph
    phosphor: str | None = None  # Its Phosphor glyph
    drawing: str | None = None  # Or the drawing (DRAWINGS) every set uses


ICONS: dict[str, Icon] = {
    # Pages (the sidebar)
    "dashboard": Icon("The dashboard, the home page", "dashboard", "squares-four"),
    "control": Icon("The Control page", "joystick", "joystick"),
    "files": Icon("The Files page", "folder_open", "folder-open"),
    "console": Icon("The console", "terminal", "terminal-window"),
    "settings": Icon("Settings; configuring a widget", "settings", "gear-six"),
    # Control's tabs
    "move": Icon("Moving the axes", "open_with", "arrows-out-cardinal"),
    "tools": Icon("The tools: hotends, spindles", drawing="tools"),
    "heightmap": Icon("The bed mesh", drawing="heightmap"),
    "objects": Icon("The job's objects, to cancel one", "category", "shapes"),
    "fan": Icon("Fans", "mode_fan", "fan"),
    # Files' tabs
    "job": Icon("Jobs, and a job's widgets", "deployed_code", "cube"),
    "macros": Icon("Macros", "data_object", "brackets-curly"),
    # Settings' categories
    "settings_general": Icon("General settings", "tune", "sliders-horizontal"),
    "settings_display": Icon("Display settings", "desktop_windows", "monitor"),
    "settings_notifications": Icon("Notification settings", "notifications", "bell"),
    "settings_connection": Icon("Connection settings", "wifi", "wifi-high"),
    "settings_system": Icon("System settings", "memory", "cpu"),
    "settings_developer": Icon("Developer settings", "build", "wrench"),
    # Moving
    "home_axis": Icon("Homing axes", "home", "house"),
    "disable_motors": Icon("Turning the motors off", "power_settings_new", "power"),
    "arrow_up": Icon("Jogging up (Y+, Z+)", "arrow_circle_up", "arrow-circle-up"),
    "arrow_down": Icon("Jogging down (Y-, Z-)", "arrow_circle_down", "arrow-circle-down"),
    "arrow_left": Icon("Jogging left (X-)", "arrow_circle_left", "arrow-circle-left"),
    "arrow_right": Icon("Jogging right (X+)", "arrow_circle_right", "arrow-circle-right"),
    "distance": Icon("The jog step and speed", "straighten", "ruler"),
    "babystep": Icon("Babystepping, the Z offset", "vertical_align_center", "arrows-in-line-vertical"),
    "babystep_increment": Icon("Babystepping up", "vertical_align_top", "arrow-line-up"),
    "babystep_decrement": Icon("Babystepping down", "vertical_align_bottom", "arrow-line-down"),
    # Tools and temperatures
    "temperature": Icon("Temperatures and sensors", "thermometer", "thermometer"),
    "bed": Icon("Beds", drawing="bed"),
    "extrude": Icon("Extruding filament", drawing="extrude"),
    "retract": Icon("Retracting filament", drawing="retract"),
    "speed": Icon("The speed factor", "speed", "gauge"),
    "graph": Icon("The temperature graph", "show_chart", "chart-line"),
    "data": Icon("A gauge of any value", "analytics", "chart-bar"),
    # Jobs
    "progress": Icon("A job's progress", "donut_large", "chart-donut"),
    "pause": Icon("Pausing a job", "pause", "pause"),
    "resume": Icon("Resuming a job", "play_arrow", "play"),
    "print_again": Icon("Starting the last job again", "replay", "arrow-counter-clockwise"),
    "cancel": Icon("Cancelling a job or an object", "cancel", "x-circle"),
    "estop": Icon("The emergency stop", "report", "warning-octagon"),
    # Files
    "folder": Icon("A folder", "folder", "folder"),
    "folder_up": Icon("The folder above", "drive_folder_upload", "arrow-elbow-left-up"),
    "file": Icon("A file with no thumbnail", "draft", "file"),
    "open": Icon("Opening something saved", "folder_open", "folder-open"),
    "refresh": Icon("Reading files again", "refresh", "arrows-clockwise"),
    "search": Icon("Searching the files in a folder", "search", "magnifying-glass"),
    "view_list": Icon("Files as a list", "view_list", "list"),
    "view_grid": Icon("Files as tiles", "grid_view", "grid-four"),
    "usb": Icon("A USB drive plugged into the screen", "usb", "usb"),
    # Editing
    "increment": Icon("More; adding", "add", "plus"),
    "decrement": Icon("Less", "remove", "minus"),
    "clear": Icon("Emptying a text box", "backspace", "backspace"),
    "delete": Icon("Deleting or removing", "delete", "trash"),
    "send": Icon("Sending G-code", "send", "paper-plane-right"),
    "save": Icon("Saving", "save", "floppy-disk"),
    "undo": Icon("Undoing", "undo", "arrow-u-up-left"),
    "reset": Icon("Going back to the default", "restart_alt", "arrow-clockwise"),
    "done": Icon("Finishing", "check", "check"),
    "snap": Icon("Snapping to edges", "grid_4x4", "magnet"),
    "drag": Icon("A handle to drag", "drag_indicator", "dots-six-vertical"),
    "back": Icon("Back", "arrow_back_ios_new", "caret-left"),
    "close": Icon("Closing", "close", "x"),
    "lock": Icon("Locked; a secured network", "lock", "lock"),
    "info": Icon("Information; a status", "info", "info"),
    "button": Icon("A button the user sets up", "smart_button", "cursor-click"),
    "text": Icon("Text the user writes", "text_fields", "text-t"),
    # A WiFi network's signal, weakest first
    "wifi_signal_1": Icon("A weak WiFi signal", "network_wifi_1_bar", "cell-signal-low"),
    "wifi_signal_2": Icon("A fair WiFi signal", "network_wifi_2_bar", "cell-signal-medium"),
    "wifi_signal_3": Icon("A good WiFi signal", "network_wifi_3_bar", "cell-signal-high"),
    "wifi_signal_4": Icon("A strong WiFi signal", "signal_wifi_4_bar", "cell-signal-full"),
}

# ---------------------------------------------------------------------------
# The sets
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class IconSet:
    """A set of icons in one style: a folder of assets/icons"""

    name: str  # Its folder, and its key in the translations (theme.icon_sets.<name>)
    family: str  # "material" or "phosphor": which glyph names of ICONS it uses
    font: FontSource  # The font its glyphs come from
    variation: list[float] | None  # The variable font's axes (Material: fill, grade, optical size, weight)
    pen: Pen  # How its drawings are drawn, to match its glyphs


SETS: list[IconSet] = [
    # Outlined, the default: Material Symbols at weight 400 and optical size 24
    IconSet("material", "material", MATERIAL_OUTLINED, [0, 0, 24, 400], Pen(width=2.0, filled=False)),
    # Filled: Material Symbols Rounded, filled, at the same weight
    IconSet("filled", "material", MATERIAL_ROUNDED, [1, 0, 24, 400], Pen(width=2.0, filled=True)),
    # Line: Phosphor's regular weight, thinner and lighter (16 of 256 is 1.5 of 24)
    IconSet("line", "phosphor", PHOSPHOR_REGULAR, None, Pen(width=1.5, filled=False)),
    # Bold: Phosphor's bold weight (24 of 256 is 2.25 of 24)
    IconSet("bold", "phosphor", PHOSPHOR_BOLD, None, Pen(width=2.25, filled=False)),
]


def render(icon_set: IconSet, name: str, icon: Icon, font: Font) -> Image.Image:
    """The icon `name` of `icon_set`, white, its shape in its alpha channel"""
    if icon.drawing is not None:
        canvas = Canvas(icon_set.pen)
        DRAWINGS[icon.drawing](canvas)
        mask = canvas.mask_image()
    else:
        glyph = icon.material if icon_set.family == "material" else icon.phosphor
        if glyph is None or not font.has(glyph):
            raise SystemExit(f"{icon_set.name}: no glyph {glyph!r} for {name}")
        mask = font.draw(glyph)
    image = Image.new("RGBA", (SIZE, SIZE), (255, 255, 255, 0))
    image.putalpha(mask)
    return image


def write_set(icon_set: IconSet, cache: Path) -> dict[str, Image.Image]:
    """Write every icon of `icon_set`, and its font's licence, into its folder; remove icons no longer in ICONS"""
    font = Font(icon_set.font, cache, icon_set.variation)
    folder = ICONS_DIR / icon_set.name
    folder.mkdir(parents=True, exist_ok=True)
    images: dict[str, Image.Image] = {}
    for name, icon in ICONS.items():
        images[name] = render(icon_set, name, icon, font)
        images[name].save(folder / f"{name}.png", optimize=True)
    for old in folder.glob("*.png"):
        if old.stem not in ICONS:
            print(f"{icon_set.name}: removing {old.name}, no longer an icon")
            old.unlink()
    (folder / "LICENSE.txt").write_text(
        f"The icons in this folder are made by scripts/generate_icons.py from {icon_set.font.homepage},\n"
        f"used under the {icon_set.font.license_name}, below. The nozzle, extruding, bed and bed mesh icons are\n"
        f"drawn by that script.\n\n{font.license_text}",
        encoding="utf-8")
    print(f"{icon_set.name}: {len(images)} icons")
    return images


def write_sheet(path: Path, sets: dict[str, dict[str, Image.Image]], columns: int = 12) -> None:
    """A contact sheet to look the icons over: each set's icons, `columns` to a row, their names under them, on a dark
    ground, one set under another"""
    names = list(ICONS)
    cell_width, cell_height, title = SIZE + 20, SIZE + 16, 20
    rows = math.ceil(len(names) / columns)
    block = title + rows * cell_height
    sheet = Image.new("RGB", (columns * cell_width, block * len(sets)), (40, 44, 52))
    draw = ImageDraw.Draw(sheet)
    for index, (set_name, images) in enumerate(sets.items()):
        top = index * block
        draw.text((4, top + 4), set_name, fill=(255, 210, 120))
        for i, name in enumerate(names):
            x = (i % columns) * cell_width
            y = top + title + (i // columns) * cell_height
            sheet.paste(images[name], (x + 10, y), images[name])
            draw.text((x + 2, y + SIZE + 2), name[:18], fill=(170, 170, 170))
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)
    print(f"Contact sheet: {path}")


def main() -> int:
    """Generate the sets asked for"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    complete_with(parser.add_argument("--set", action="append", help="Only this set (may be given more than once)"),
                  lambda: [s.name for s in SETS])
    complete_files(parser.add_argument("--cache", type=Path, default=DEFAULT_CACHE, help="Where the fonts are downloaded to"))
    complete_files(parser.add_argument("--sheet", type=Path, help="Also write a contact sheet of the sets here"), ".png")
    complete(parser)
    args = parser.parse_args()

    chosen = [s for s in SETS if args.set is None or s.name in args.set]
    if not chosen:
        print(f"No such set; there are {', '.join(s.name for s in SETS)}", file=sys.stderr)
        return 1
    sets = {icon_set.name: write_set(icon_set, args.cache) for icon_set in chosen}
    if args.sheet is not None:
        write_sheet(args.sheet, sets)
    return 0


if __name__ == "__main__":
    sys.exit(main())
