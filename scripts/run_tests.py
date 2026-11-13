#!/usr/bin/env python3
"""
Run DuetScreen UI tests and assist with image difference review.

Steps:
1) Delete any *_err images in tests/ref_imgs
2) Run tests (via CTest if available)
3) For any *_err images produced, show reference, diff, and error images
4) Prompt to update references with *_err images

Usage:
  python3 scripts/run_tests.py

Optional env vars:
  DUETSCREEN_BUILD_DIR   Override the CTest build dir (default: auto-detected under out/build)

This has been heavily vibe coded since it is none critical

"""

from __future__ import annotations

import os
import sys
import subprocess
import argparse
import shutil
import re
from pathlib import Path
from typing import List, Optional, Tuple, Any

DEFAULT_CMAKE_PRESET = "Test"
PROJECT_ROOT = Path(__file__).resolve().parents[1]
CMAKE_EXE = "cmake"
BUILD_ROOT = PROJECT_ROOT / "out" / "build"
SRC_DIR = PROJECT_ROOT / "src"
REF_IMGS_DIR = PROJECT_ROOT / "tests" / "ref_imgs"
CWD_LVGL = PROJECT_ROOT / "libraries" / "lvgl"
LVGL_REF_DIRS = [
	CWD_LVGL / "tests" / "ref_imgs",
	CWD_LVGL / "tests" / "ref_imgs_vg_lite",
]
CURRENT_REF_DIRS: List[Path] = [REF_IMGS_DIR]
COVERAGE_REPORT_DIR = PROJECT_ROOT / "tests" / "report"
COVERAGE_HTML = COVERAGE_REPORT_DIR / "index.html"


def log(msg: str) -> None:
	print(msg, flush=True)


def maximize_window(win: "Any") -> None:
	"""Best-effort maximize that prefers WM maximize over fullscreen.

	Tries, in order:
	  1) state('zoomed')
	  2) attributes('-zoomed', True)
	"""
	try:
		win.update_idletasks()
	except Exception:
		pass

	# Attempt native maximize
	tried_zoomed = False
	try:
		win.state('zoomed')
		tried_zoomed = True
		win.update_idletasks()
	except Exception:
		# Some WMs support this attribute
		try:
			win.attributes('-zoomed', True)
			win.update_idletasks()
		except Exception:
			pass

	ww, wh = win.winfo_width(), win.winfo_height()
	sw, sh = win.winfo_screenwidth(), win.winfo_screenheight()
	if (abs(ww - sw) > 80) or (abs(wh - sh) > 80):
		width = min(sw, 2500)
		height = min(sh - 80, 1500)
		win.geometry(f"{width}x{height}+0+0")
	pass


def clean_err_images(ref_dir: Path) -> List[Path]:
	"""Remove any *_err.png images recursively under ref_dir.

	Returns list of removed paths.
	"""
	removed: List[Path] = []
	if not ref_dir.exists():
		return removed
	for p in sorted(ref_dir.rglob("*_err.png")):
		if not p.is_file():
			continue
		try:
			p.unlink()
			removed.append(p)
		except Exception as e:
			log(f"Warning: failed to remove '{p}': {e}")
	return removed


def clean_err_images_in_dirs(ref_dirs: List[Path]) -> List[Path]:
	removed: List[Path] = []
	for d in ref_dirs:
		removed.extend(clean_err_images(d))
	return removed


def snapshot_existing_refs(ref_dir: Path) -> set[str]:
	"""Return a snapshot of existing reference image relative paths before tests run.

	Uses paths relative to ref_dir (POSIX style) to avoid collisions between
	files with identical basenames in different subdirectories. Excludes any
	*_err.* files; only baseline references are tracked.
	"""
	existing: set[str] = set()
	if not ref_dir.exists():
		return existing
	for p in ref_dir.rglob("*"):
		if not p.is_file():
			continue
		if "_err.png" in p.name:
			continue
		try:
			rel = p.relative_to(ref_dir).as_posix()
		except Exception:
			rel = p.name
		existing.add(rel)
	return existing


def snapshot_existing_refs_in_dirs(ref_dirs: List[Path]) -> set[str]:
	existing: set[str] = set()
	for d in ref_dirs:
		existing.update(snapshot_existing_refs(d))
	return existing


def list_new_refs(ref_dir: Path, before: set[str]) -> List[Path]:
	"""List reference images (recursively) that did not exist in the snapshot."""
	created: List[Path] = []
	if not ref_dir.exists():
		return created
	for p in sorted(ref_dir.rglob("*")):
		if not p.is_file():
			continue
		if "_err.png" in p.name:
			continue
		try:
			rel = p.relative_to(ref_dir).as_posix()
		except Exception:
			rel = p.name
		if rel not in before:
			created.append(p)
	return created


def list_new_refs_in_dirs(ref_dirs: List[Path], before: set[str]) -> List[Path]:
	created: List[Path] = []
	for d in ref_dirs:
		created.extend(list_new_refs(d, before))
	return created


def find_build_dir() -> Optional[Path]:
	# 1) Env override
	env_override = os.environ.get("DUETSCREEN_BUILD_DIR")
	if env_override:
		d = Path(env_override).expanduser().resolve()
		if (d / "CTestTestfile.cmake").exists() or (d / "tests" / "DuetScreen.tests").exists():
			return d

	# 2) Prefer an existing Simulation build (most common for local UI testing)
	build_dir = BUILD_ROOT / get_cmake_preset()
	if (build_dir / "CTestTestfile.cmake").exists() or (build_dir / "tests" / "DuetScreen.tests").exists():
		return build_dir

	return None


def get_cmake_preset() -> str:
	return os.environ.get("DUETSCREEN_CMAKE_PRESET", DEFAULT_CMAKE_PRESET)


def run_lvgl_tests(test_filter: Optional[str] = None) -> int:
	log(f"Running lvgl tests in: {CWD_LVGL}")
	cmd = [sys.executable, "tests/main.py"]
	if test_filter:
		cmd.append(f"--test-suite={test_filter}")
	try:
		log(" ".join(cmd))
		result = subprocess.run(cmd, cwd=str(CWD_LVGL), check=False)
		return result.returncode
	except Exception as e:
		log(f"Failed to run lvgl tests: {e}")
		return 127


def run_tests(build_dir: Path, test_filter: Optional[str] = None) -> int:
	# Prefer running `ctest` in the build dir. If not available, try the test binary.
	log(f"Running tests in: {build_dir}")

	# Normal path: use ctest if available
	ctest_cmd = ["ctest", "--test-dir", (build_dir / "tests").as_posix(), "--output-on-failure", "-j1"]
	if test_filter:
		ctest_cmd.extend(["-R", test_filter])

	try:
		log(" ".join(ctest_cmd))
		result = subprocess.run(ctest_cmd, cwd=str(PROJECT_ROOT), check=False)
		return result.returncode
	except FileNotFoundError:
		log("ctest not found, trying to run test binary directly…")

	test_bin = build_dir / "tests" / "DuetScreen.tests"
	if test_filter:
		test_bin = f"{test_bin} --gtest_filter={test_filter}"

	if test_bin.exists() and os.access(test_bin, os.X_OK):
		log(str(test_bin))
		result = subprocess.run([str(test_bin)], cwd=str(PROJECT_ROOT), check=False)
		return result.returncode

	log("Error: Neither ctest nor the test binary was found/executable.")
	return 127
def configure_cmake(preset: Optional[str] = None) -> bool:
	"""Run `cmake --preset <preset>` in the project root to configure the build."""
	preset = preset or get_cmake_preset()
	log(f"Configuring CMake preset: {preset}")
	try:
		result = subprocess.run([CMAKE_EXE, "--preset", preset], cwd=str(PROJECT_ROOT), check=False)
		if result.returncode != 0:
			log(f"CMake configure failed with code {result.returncode}")
			return False
		return True
	except FileNotFoundError:
		log("Error: cmake not found in PATH.")
		return False


def build_tests(build_dir: Path) -> int:
	"""Build the test target before running them."""
	jobs = str(os.cpu_count() or 2)
	log(f"Building tests in: {build_dir}")
	try:
		result = subprocess.run(
			[
				CMAKE_EXE,
				"--build",
				str(build_dir),
				"--target",
				"DuetScreen.tests",
				"--parallel",
				jobs,
			],
			cwd=str(build_dir),
			check=False,
		)
		return result.returncode
	except FileNotFoundError:
		log("Error: cmake not found in PATH.")
		return 127


def list_err_images(ref_dir: Path) -> List[Path]:
	"""Return list of *_err.* images found recursively under ref_dir."""
	if not ref_dir.exists():
		return []
	return sorted(p for p in ref_dir.rglob("*_err.png") if p.is_file())


def list_err_images_in_dirs(ref_dirs: List[Path]) -> List[Path]:
	items: List[Path] = []
	for d in ref_dirs:
		items.extend(list_err_images(d))
	return sorted(items)


def rel_to_active_dirs(p: Path) -> str:
	for d in CURRENT_REF_DIRS:
		try:
			return p.relative_to(d).as_posix()
		except Exception:
			continue
	try:
		return p.relative_to(REF_IMGS_DIR).as_posix()
	except Exception:
		return p.name


def load_images_for_compare(ref_path: Path, err_path: Path):
	try:
		from PIL import Image, ImageChops, ImageOps, ImageDraw, ImageFont
	except Exception as e:  # pragma: no cover - optional dependency
		log("Note: Install 'Pillow' (pip install pillow) for visual diff popups.")
		return None

	try:
		ref = Image.open(ref_path).convert("RGBA")
		err = Image.open(err_path).convert("RGBA")
	except Exception as e:
		log(f"Warning: failed to open images: {e}")
		return None

	# Ensure same size for comparison canvas by padding smaller image
	w = max(ref.width, err.width)
	h = max(ref.height, err.height)

	def pad(img):
		if img.width == w and img.height == h:
			return img
		bg = Image.new("RGBA", (w, h), (0, 0, 0, 0))
		bg.paste(img, (0, 0))
		return bg

	ref_p = pad(ref)
	err_p = pad(err)

	# Compute difference on RGB channels only (ignore alpha), so the diff isn't fully transparent
	ref_rgb = ref_p.convert("RGB")
	err_rgb = err_p.convert("RGB")
	diff_raw = ImageChops.difference(ref_rgb, err_rgb)
	diff = diff_raw
	# Improve visibility of small changes
	try:
		diff = ImageOps.autocontrast(diff)
	except Exception:
		pass

	# Build an additional view: the NEW image in greyscale with the difference highlighted in red
	# 1) Greyscale version of the new image
	new_gray_rgba = ImageOps.grayscale(err_p).convert("RGBA")
	# 2) Generate a luminance mask from the raw diff (before autocontrast to avoid artifacts)
	try:
		diff_mask = diff_raw.convert("L")
		# Optionally boost contrast for clearer highlights
		diff_mask = ImageOps.autocontrast(diff_mask)
	except Exception:
		diff_mask = diff_raw.convert("L")
	# 3) Create red overlay with alpha from the diff mask
	red_overlay = Image.new("RGBA", (w, h), (255, 0, 0, 0))
	red_overlay.putalpha(diff_mask)
	# 4) Composite red overlay over greyscale new image
	new_gray_with_red = Image.alpha_composite(new_gray_rgba, red_overlay)

	# Composite layout:
	#  Row 1: [Reference]               [New (_err)]
	#  Row 2: [Difference]              [New (grey + red diff)]
	gap = 10
	label_h = 24

	top_row_content_w = w * 2 + gap
	out_w = top_row_content_w + gap * 2
	out_h = (label_h + h) * 2 + gap * 3  # top label+img + gap + bottom label+img + margins
	canvas = Image.new("RGBA", (out_w, out_h), (24, 24, 24, 255))

	# Label drawing helper (with custom width)
	def draw_label(x: int, y: int, width: int, text: str):
		draw = ImageDraw.Draw(canvas)
		font = ImageFont.load_default()
		bx = x
		by = y
		bw = width
		bh = label_h
		draw.rectangle([bx, by, bx + bw, by + bh], fill=(40, 40, 40, 255))
		# Center vertically
		try:
			tw, th = draw.textsize(text, font=font)
		except Exception:
			tw, th = (len(text) * 6, 12)
		draw.text((bx + 6, by + (bh - th) // 2), text, fill=(220, 220, 220, 255), font=font)

	# Positions
	x_left = gap
	x_right = gap + w + gap
	y_top_label = gap
	y_top_img = y_top_label + label_h

	# Top row labels
	draw_label(x_left, y_top_label, w, "Reference")
	draw_label(x_right, y_top_label, w, "New (_err)")

	# Top row images
	canvas.paste(ref_p, (x_left, y_top_img))
	canvas.paste(err_p, (x_right, y_top_img))

	# Bottom row (Difference + Greyscale with red diff)
	y_bottom_label = y_top_img + h + gap
	y_bottom_img = y_bottom_label + label_h
	# Left: Difference
	draw_label(x_left, y_bottom_label, w, "Difference")
	canvas.paste(diff.convert("RGBA"), (x_left, y_bottom_img))
	# Right: New greyscale with red diff overlay
	draw_label(x_right, y_bottom_label, w, "New (grey + red diff)")
	canvas.paste(new_gray_with_red, (x_right, y_bottom_img))

	return canvas


def load_single_image_view(img_path: Path):
	"""Build a labeled canvas to display a single image (for new references)."""
	try:
		from PIL import Image, ImageDraw, ImageFont  # type: ignore
	except Exception:
		log("Note: Install 'Pillow' (pip install pillow) for visual previews.")
		return None

	try:
		img = Image.open(img_path).convert("RGBA")
	except Exception as e:
		log(f"Warning: failed to open image '{img_path}': {e}")
		return None

	gap = 10
	label_h = 24
	out_w = img.width + gap * 2
	out_h = label_h + gap + img.height + gap
	canvas = Image.new("RGBA", (max(1, out_w), max(1, out_h)), (24, 24, 24, 255))

	# Header
	draw = ImageDraw.Draw(canvas)
	try:
		font = ImageFont.load_default()
	except Exception:
		font = None
	draw.rectangle([gap, gap, gap + img.width, gap + label_h], fill=(40, 40, 40, 255))
	label = f"New reference image: {img_path.name}"
	try:
		tw, th = draw.textsize(label, font=font)
	except Exception:
		tw, th = (len(label) * 6, 12)
	draw.text((gap + 6, gap + (label_h - th) // 2), label, fill=(220, 220, 220, 255), font=font)

	# Image body
	canvas.paste(img, (gap, gap + label_h + gap))
	return canvas


class UINewRefReviewer:
	"""Tkinter UI to review newly created reference images (keep/delete)."""

	def __init__(self, items: List[Path]):
		import tkinter as tk  # type: ignore
		from PIL import ImageTk  # type: ignore

		self.tk = tk
		self.ImageTk = ImageTk

		self.items = items
		self.index = 0
		self.keep_all = False

		self.root = tk.Tk()
		self.root.title("DuetScreen New Reference Reviewer")
		# Start maximized (not fullscreen)
		self._fullscreen = False
		maximize_window(self.root)

		self.root.rowconfigure(1, weight=1)
		self.root.columnconfigure(0, weight=1)

		self.header_var = tk.StringVar()
		self.header = tk.Label(self.root, textvariable=self.header_var, anchor="w")
		self.header.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 0))

		self.image_frame = tk.Frame(self.root, bd=0, highlightthickness=0)
		self.image_frame.grid(row=1, column=0, sticky="nsew", padx=8, pady=8)
		self.image_frame.rowconfigure(0, weight=1)
		self.image_frame.columnconfigure(0, weight=1)

		self.image_label = tk.Label(self.image_frame, bd=0)
		self.image_label.grid(row=0, column=0, sticky="nsew")
		self.image_frame.bind("<Configure>", self._on_resize)

		self.controls = tk.Frame(self.root)
		self.controls.grid(row=2, column=0, sticky="ew", padx=8, pady=(0, 8))
		for i in range(6):
			self.controls.columnconfigure(i, weight=1)

		self.btn_prev = tk.Button(self.controls, text="Previous [←]", command=self.prev_item)
		self.btn_prev.grid(row=0, column=0, padx=4)

		self.btn_delete = tk.Button(self.controls, text="Delete [D]", command=self.delete_item)
		self.btn_delete.grid(row=0, column=1, padx=4)

		self.btn_keep = tk.Button(self.controls, text="Keep [Y]", command=self.keep_item)
		self.btn_keep.grid(row=0, column=2, padx=4)

		self.btn_keep_all = tk.Button(self.controls, text="Keep All [A]", command=self.keep_all_items)
		self.btn_keep_all.grid(row=0, column=3, padx=4)

		self.btn_next = tk.Button(self.controls, text="Next [→]", command=self.next_item)
		self.btn_next.grid(row=0, column=4, padx=4)

		self.btn_quit = tk.Button(self.controls, text="Quit [Q/Esc]", command=self.root.destroy)
		self.btn_quit.grid(row=0, column=5, padx=4)

		self.root.bind("<KeyPress-q>", lambda e: self.root.destroy())
		self.root.bind("<KeyPress-Escape>", lambda e: self.root.destroy())
		self.root.bind("<F11>", self._toggle_fullscreen)
		self.root.bind("<KeyPress-Right>", lambda e: self.next_item())
		self.root.bind("<KeyPress-Left>", lambda e: self.prev_item())
		self.root.bind("<KeyPress-y>", lambda e: self.keep_item())
		self.root.bind("<KeyPress-d>", lambda e: self.delete_item())
		self.root.bind("<KeyPress-a>", lambda e: self.keep_all_items())

		self._tk_image = None
		self._current_composite = None  # PIL Image for current index

		self.render_current()

	def _toggle_fullscreen(self, event=None):
		self._fullscreen = not self._fullscreen
		try:
			self.root.attributes('-fullscreen', self._fullscreen)
		except Exception:
			try:
				self.root.state('zoomed' if self._fullscreen else 'normal')
			except Exception:
				pass

	def _current_path(self) -> Path:
		return self.items[self.index]

	def _ensure_current_composite(self):
		if self._current_composite is None:
			p = self._current_path()
			self._current_composite = load_single_image_view(p)
		return self._current_composite

	def _set_header(self):
		p = self._current_path()
		# Show path relative to the active reference dirs when possible
		self.header_var.set(f"{self.index + 1}/{len(self.items)}  —  {rel_to_active_dirs(p)}")

	def _on_resize(self, event=None):
		self._render_image_to_label()

	def _render_image_to_label(self):
		from PIL import Image  # type: ignore

		if not self.items:
			return
		try:
			self.root.update_idletasks()
		except Exception:
			pass

		comp = self._ensure_current_composite()
		if comp is None:
			return
		w = max(1, self.image_frame.winfo_width() - 4)
		h = max(1, self.image_frame.winfo_height() - 4)
		if w <= 1 or h <= 1:
			try:
				self.root.after(50, self._render_image_to_label)
			except Exception:
				pass
			return

		scale = min(w / comp.width, h / comp.height, 1.0)
		if scale < 1.0:
			new_size = (max(1, int(comp.width * scale)), max(1, int(comp.height * scale)))
			disp = comp.resize(new_size, Image.LANCZOS)
		else:
			disp = comp

		self._tk_image = self.ImageTk.PhotoImage(disp)
		self.image_label.configure(image=self._tk_image)
		self.image_label.image = self._tk_image

	def render_current(self):
		if not self.items:
			self.header_var.set("No new reference images to review.")
			return
		self.index = max(0, min(self.index, len(self.items) - 1))
		self._current_composite = None
		self._set_header()
		self._render_image_to_label()
		self._update_buttons_state()

	def _update_buttons_state(self):
		self.btn_prev.configure(state=("normal" if self.index > 0 else "disabled"))
		self.btn_next.configure(state=("normal" if self.index < len(self.items) - 1 else "disabled"))

	def prev_item(self):
		if self.index > 0:
			self.index -= 1
			self.render_current()

	def next_item(self):
		if self.index < len(self.items) - 1:
			self.index += 1
			self.render_current()

	def skip_and_close_if_done(self):
		if not self.items or self.index >= len(self.items) - 1:
			try:
				self.header_var.set("Review complete. Closing…")
			except Exception:
				pass
			try:
				self.root.after(100, self.root.destroy)
			except Exception:
				self.root.destroy()
			return True
		return False

	def delete_item(self):
		p = self._current_path()
		try:
			p.unlink()
		except Exception:
			pass
		del self.items[self.index]
		if self.index >= len(self.items):
			self.index = len(self.items) - 1
		if not self.items:
			self.header_var.set("All deletions applied. Done.")
			self.image_label.configure(image="")
			self.root.after(600, self.root.destroy)
			return
		self.render_current()

	def keep_item(self):
		# Keeping means do nothing and advance
		if self.keep_all:
			# If already keeping all, fast-forward
			self.next_item()
			return
		if self.skip_and_close_if_done():
			return
		self.next_item()

	def keep_all_items(self):
		self.keep_all = True
		# Nothing to delete; just close quickly
		self.header_var.set("All kept. Done.")
		self.image_label.configure(image="")
		self.root.after(400, self.root.destroy)

	def run(self):
		self.root.mainloop()


class UIDiffReviewer:
	"""Single-window Tkinter UI to review and update reference images."""

	def __init__(self, items: List[Tuple[Path, Path]]):
		import tkinter as tk  # type: ignore
		from PIL import ImageTk  # type: ignore

		self.tk = tk
		self.ImageTk = ImageTk

		self.items = items
		self.index = 0
		self.apply_all = False

		self.root = tk.Tk()
		self.root.title("DuetScreen Visual Diff Reviewer")
		# Start maximized (not fullscreen); allow toggling fullscreen with F11
		self._fullscreen = False
		maximize_window(self.root)

		# Window layout
		self.root.rowconfigure(1, weight=1)
		self.root.columnconfigure(0, weight=1)

		# Header with filename and index
		self.header_var = tk.StringVar()
		self.header = tk.Label(self.root, textvariable=self.header_var, anchor="w")
		self.header.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 0))

		# Image display area
		self.image_frame = tk.Frame(self.root, bd=0, highlightthickness=0)
		self.image_frame.grid(row=1, column=0, sticky="nsew", padx=8, pady=8)
		self.image_frame.rowconfigure(0, weight=1)
		self.image_frame.columnconfigure(0, weight=1)

		self.image_label = tk.Label(self.image_frame, bd=0)
		self.image_label.grid(row=0, column=0, sticky="nsew")
		self.image_frame.bind("<Configure>", self._on_resize)

		# Controls
		self.controls = tk.Frame(self.root)
		self.controls.grid(row=2, column=0, sticky="ew", padx=8, pady=(0, 8))
		for i in range(6):
			self.controls.columnconfigure(i, weight=1)

		self.btn_prev = tk.Button(self.controls, text="Previous [←]", command=self.prev_item)
		self.btn_prev.grid(row=0, column=0, padx=4)

		self.btn_skip = tk.Button(self.controls, text="Skip [N]", command=self.skip_item)
		self.btn_skip.grid(row=0, column=1, padx=4)

		self.btn_update = tk.Button(self.controls, text="Update [Y]", command=self.update_item)
		self.btn_update.grid(row=0, column=2, padx=4)

		self.btn_update_all = tk.Button(self.controls, text="Update All [A]", command=self.update_all)
		self.btn_update_all.grid(row=0, column=3, padx=4)

		self.btn_next = tk.Button(self.controls, text="Next [→]", command=self.next_item)
		self.btn_next.grid(row=0, column=4, padx=4)

		self.btn_quit = tk.Button(self.controls, text="Quit [Q/Esc]", command=self.root.destroy)
		self.btn_quit.grid(row=0, column=5, padx=4)

		# Keyboard shortcuts
		self.root.bind("<KeyPress-q>", lambda e: self.root.destroy())
		self.root.bind("<KeyPress-Escape>", lambda e: self.root.destroy())
		self.root.bind("<F11>", self._toggle_fullscreen)
		self.root.bind("<KeyPress-Right>", lambda e: self.next_item())
		self.root.bind("<KeyPress-Left>", lambda e: self.prev_item())
		self.root.bind("<KeyPress-y>", lambda e: self.update_item())
		self.root.bind("<KeyPress-n>", lambda e: self.skip_item())
		self.root.bind("<KeyPress-a>", lambda e: self.update_all())

		self._tk_image = None  # keep reference to prevent GC
		self._current_composite = None

		self.render_current()

	def _toggle_fullscreen(self, event=None):
		self._fullscreen = not self._fullscreen
		try:
			self.root.attributes('-fullscreen', self._fullscreen)
		except Exception:
			try:
				self.root.state('zoomed' if self._fullscreen else 'normal')
			except Exception:
				pass

	def _current_paths(self) -> Tuple[Path, Path]:
		ref, err, _ = self.items[self.index]
		return ref, err

	def _current_composite_image(self):
		_, _, comp = self.items[self.index]
		return comp

	def _set_header(self):
		ref, err = self._current_paths()
		self.header_var.set(
			f"{self.index + 1}/{len(self.items)}  —  {err.name}  (ref: {rel_to_active_dirs(ref)})")

	def _on_resize(self, event=None):
		# Re-render scaled image to fit the available area
		self._render_image_to_label()

	def _render_image_to_label(self):
		from PIL import Image  # type: ignore

		if not self.items:
			return

		# Make sure geometry is up-to-date before measuring
		try:
			self.root.update_idletasks()
		except Exception:
			pass

		comp = self._current_composite_image()

		# Fit within image_frame size with a small margin
		w = max(1, self.image_frame.winfo_width() - 4)
		h = max(1, self.image_frame.winfo_height() - 4)

		# If the frame hasn't been laid out yet, retry shortly
		if w <= 1 or h <= 1:
			try:
				self.root.after(50, self._render_image_to_label)
			except Exception:
				pass
			return

		scale = min(w / comp.width, h / comp.height, 1.0)
		if scale < 1.0:
			new_size = (max(1, int(comp.width * scale)), max(1, int(comp.height * scale)))
			disp = comp.resize(new_size, Image.LANCZOS)
		else:
			disp = comp

		self._tk_image = self.ImageTk.PhotoImage(disp)
		# Attach to label as well to ensure strong reference
		self.image_label.configure(image=self._tk_image)
		self.image_label.image = self._tk_image

	def render_current(self):
		if not self.items:
			self.header_var.set("No differences to review.")
			return
		self.index = max(0, min(self.index, len(self.items) - 1))
		self._set_header()
		self._render_image_to_label()
		self._update_buttons_state()

	def _update_buttons_state(self):
		self.btn_prev.configure(state=("normal" if self.index > 0 else "disabled"))
		self.btn_next.configure(state=("normal" if self.index < len(self.items) - 1 else "disabled"))

	def prev_item(self):
		if self.index > 0:
			self.index -= 1
			self.render_current()

	def next_item(self):
		if self.index < len(self.items) - 1:
			self.index += 1
			self.render_current()

	def skip_item(self):
		del self.items[self.index]
		if self.index >= len(self.items):
			self.index = len(self.items) - 1
		if not self.items:
			self.header_var.set("All differences reviewed. Done.")
			self.image_label.configure(image="")
			# Auto-close after a short delay
			self.root.after(600, self.root.destroy)
			return
		self.render_current()

	def update_item(self):
		ref, err = self._current_paths()
		update_reference(ref, err)
		# Remove from list and render next
		del self.items[self.index]
		if self.index >= len(self.items):
			self.index = len(self.items) - 1
		if not self.items:
			self.header_var.set("All updates applied. Done.")
			self.image_label.configure(image="")
			# Auto-close after a short delay
			self.root.after(600, self.root.destroy)
			return
		self.render_current()

	def update_all(self):
		# Apply updates to all remaining items
		while self.items:
			ref, err = self.items[0][0], self.items[0][1]
			update_reference(ref, err)
			del self.items[0]
		self.header_var.set("All updates applied. Done.")
		self.image_label.configure(image="")
		self.root.after(600, self.root.destroy)

	def run(self):
		self.root.mainloop()


def ask_yes_no(prompt: str, default: str = "n") -> str:
	valid = {"y", "n", "a", "q"}
	default = default.lower()
	assert default in {"y", "n"}
	suffix = " [y/N/a/q]: " if default == "n" else " [Y/n/a/q]: "
	while True:
		resp = input(prompt + suffix).strip().lower()
		if not resp:
			return default
		if resp in valid:
			return resp
		log("Please respond with 'y', 'n', 'a' (all), or 'q' (quit).")


def update_reference(ref_path: Path, err_path: Path) -> None:
	data = err_path.read_bytes()
	ref_path.write_bytes(data)
	try:
		err_path.unlink()
	except Exception:
		pass


def parse_args(argv: List[str]):
	parser = argparse.ArgumentParser(description="Run DuetScreen tests & review image diffs")
	parser.add_argument("--test_filter", "-f", default="",
                     help="Test filter expression (passed to ctest or test binary)")
	parser.add_argument("--coverage", "-c", action="store_true",
                     help="Generate gcovr HTML coverage report (tests/report/index.html)")
	parser.add_argument("--skip_configure", action="store_true", help="Skip CMake configure step")
	parser.add_argument("--skip_build", action="store_true", help="Skip CMake build step")
	parser.add_argument(
		"--compare-only",
		action="store_true",
		help="Do not clean/build/run tests; only compare existing *_err images against references",
	)
	parser.add_argument("--lvgl-tests", action="store_true", dest="lvgl_tests",
                     help="Run lvgl unit tests (python tests/main.py) and use lvgl ref image dirs")
	return parser.parse_args(argv)


def should_run_coverage(args) -> bool:
	if getattr(args, "coverage", False):
		return True
	env = os.environ.get("DUETSCREEN_COVERAGE", "").lower()
	return env in {"1", "true", "yes", "on"}


def run_coverage(build_dir: Path) -> int:
	"""Generate an HTML coverage report using gcovr.

	Assumes the build was configured with coverage flags (e.g. -g -O0 -fprofile-arcs -ftest-coverage or LLVM equivalents).
	Skips gracefully if gcovr is not installed.
	"""
	if shutil.which("gcovr") is None:
		log("Coverage: gcovr not found in PATH, skipping coverage generation.")
		return 0

	try:
		COVERAGE_REPORT_DIR.mkdir(parents=True, exist_ok=True)
	except Exception as e:
		log(f"Coverage: failed to create report directory '{COVERAGE_REPORT_DIR}': {e}")
		return 1

	build_dir = find_build_dir()
	if (not build_dir):
		log("Coverage: could not determine build directory, skipping coverage generation.")
		return 1

	gcov_executable = detect_gcov_executable(build_dir)

	cmd = [
		"gcovr",
		build_dir.as_posix(),
		"--root", str(PROJECT_ROOT),
		"--filter", str(SRC_DIR),
		"--html-details",
		"--output", str(COVERAGE_HTML),
		'-j', str(os.cpu_count()),
		'--print-summary',
		'--html-title', 'DuetScreen Test Coverage',
		'--html-theme', 'github.dark-green'
	]

	if gcov_executable is not None:
		cmd.extend(["--gcov-executable", gcov_executable])
		log(f"Coverage: using gcov executable '{gcov_executable}'")

	log("Coverage: running gcovr to produce HTML report…")
	log("Coverage: " + " ".join(cmd))
	try:
		res = subprocess.run(cmd, cwd=str(build_dir), check=False)
		if res.returncode == 0:
			if COVERAGE_HTML.exists():
				log(f"Coverage: report written to {COVERAGE_HTML}")
			else:
				log("Coverage: gcovr completed but report file not found.")
		else:
			log(f"Coverage: gcovr failed with exit code {res.returncode}")
		return res.returncode
	except Exception as e:
		log(f"Coverage: unexpected error invoking gcovr: {e}")
		return 1


def detect_gcov_executable(build_dir: Path) -> Optional[str]:
	"""Choose a gcov executable matching the compiler used in the build directory.

	Returns a gcov executable name/path suitable for gcovr (e.g. gcov-15),
	or None if no better match than plain `gcov` can be determined.
	"""
	cache_path = build_dir / "CMakeCache.txt"
	if not cache_path.exists():
		return None

	try:
		cache_text = cache_path.read_text(encoding="utf-8", errors="ignore")
	except Exception as e:
		log(f"Coverage: failed reading {cache_path}: {e}")
		return None

	compiler_value: Optional[str] = None
	for key in ("CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER"):
		match = re.search(rf"^{key}:[^=]*=(.+)$", cache_text, flags=re.MULTILINE)
		if match:
			compiler_value = match.group(1).strip()
			if compiler_value:
				break

	if not compiler_value:
		return None

	major_match = re.search(r"(?:gcc|g\+\+)(?:-([0-9]+))?$", compiler_value)
	if not major_match:
		return None

	major = major_match.group(1)
	if not major:
		return None

	candidate = f"gcov-{major}"
	if shutil.which(candidate) is not None:
		return candidate

	return None


def main(argv: List[str]) -> int:
	args = parse_args(argv)
	rc = 0
	build_dir = None

	# Choose which reference directories to operate on
	global CURRENT_REF_DIRS
	if getattr(args, "lvgl_tests", False):
		CURRENT_REF_DIRS = LVGL_REF_DIRS
	else:
		CURRENT_REF_DIRS = [REF_IMGS_DIR]

	if getattr(args, "compare_only", False):
		log("Compare-only mode: skipping clean/build/test steps and reviewing existing *_err images.")
	else:
		log("Step 1/5: Cleaning existing *_err images…")
		removed = clean_err_images_in_dirs(CURRENT_REF_DIRS)
		if removed:
			log(f"  Removed {len(removed)} file(s) from {', '.join(str(p) for p in CURRENT_REF_DIRS)}")
		else:
			log("  Nothing to remove.")

	if not getattr(args, "compare_only", False):
		log("Step 2/5: Ensure build is configured…")
		# For lvgl tests we run the lvgl test runner directly and skip CMake configure/build
		if getattr(args, "lvgl_tests", False):
			log("  Running lvgl unit tests; skipping CMake configure/build.")
		else:
			# Try to configure automatically using a preset
			if args.skip_configure:
				log("  Skipping configure step as requested.")
			else:
				if not configure_cmake():
					log("Failed to configure the project. Aborting.")
					return 2

			build_dir = find_build_dir()
			if build_dir is None:
				log("Error: Could not locate a CTest build directory after configure.")
				return 2

		log("Step 3/5: Building tests…")
		if getattr(args, "lvgl_tests", False):
			log("  Skipping build step for lvgl tests (running test script).")
		else:
			if args.skip_build:
				log("  Skipping build step as requested.")
			else:
				brc = build_tests(build_dir)
				if brc != 0:
					log(f"Build failed with return code {brc}.")
					return brc

		# Snapshot existing references before running tests so we can detect newly created ones
		before_snapshot = snapshot_existing_refs_in_dirs(CURRENT_REF_DIRS)

		log("Step 4/5: Running tests…")
		if getattr(args, "lvgl_tests", False):
			rc = run_lvgl_tests(test_filter=args.test_filter)
		else:
			rc = run_tests(build_dir, test_filter=args.test_filter)
		if rc != 0:
			log(f"Tests finished with return code {rc} (there may be failures).")
		else:
			log("Tests completed successfully.")

		# First, check for any newly created reference images (created because a reference didn't exist)
		created_refs = list_new_refs_in_dirs(CURRENT_REF_DIRS, before_snapshot)
		if created_refs:
			log("Review newly created reference images…")
			# Try Pillow/Tk UI first (lazy-loading inside the UI)
			pil_ok = True
			try:
				reviewer_nr = UINewRefReviewer(created_refs)
				reviewer_nr.run()
			except Exception as e:
				log(f"Failed to open Tk UI for new references: {e}")
				pil_ok = False

			if not pil_ok:
				# Fallback to CLI flow
				apply_all = False
				for p in created_refs:
					log("")
					log(f"New reference image created: {p.name}")
					if apply_all:
						choice = "y"
					else:
						choice = ask_yes_no("Keep this new reference image?", default="n")
						if choice == "a":
							apply_all = True
							choice = "y"
						if choice == "q":
							log("Aborting by user request.")
							return 130
					if choice == "y":
						log("  Kept.")
					else:
						try:
							p.unlink()
							log("  Deleted.")
						except Exception as e:
							log(f"  Failed to delete: {e}")
		else:
			log("No newly created reference images were detected.")

	log("Step 5/5: Scanning for *_err images…")
	err_images = list_err_images_in_dirs(CURRENT_REF_DIRS)
	if not err_images:
		log("No *_err images were produced. Visual checks passed.")
	else:
		# Build list of items for the reviewer UI
		items: List[Tuple[Path, Path, object]] = []
		pil_ok = True
		try:
			for err_path in err_images:
				if err_path.name.endswith("_err.png") is False:
					continue

				ref_path = Path(str(err_path).replace("_err.png", ".png"))
				comp = load_images_for_compare(ref_path, err_path)
				if comp is None:
					pil_ok = False
					break
				items.append((ref_path, err_path, comp))
		except Exception as e:
			pil_ok = False

		if not pil_ok:
			log("Pillow not available or failed to process images. Falling back to CLI prompts.")
			apply_all = False
			for err_path in err_images:
				ref_path = Path(str(err_path).replace("_err", ""))
				log("")
				log(f"Reviewing: {err_path.name}")
				log(f"  Reference: {ref_path}")
				log(f"  New (_err): {err_path}")
				if apply_all:
					choice = "y"
				else:
					choice = ask_yes_no("Update reference with new image?", default="n")
					if choice == "a":
						apply_all = True
						choice = "y"
					if choice == "q":
						log("Aborting by user request.")
						return 130
				if choice == "y":
					update_reference(ref_path, err_path)
					log(f"  Updated: {ref_path.name}")
				else:
					log("  Skipped update.")

		# Start Tkinter review UI
		try:
			reviewer = UIDiffReviewer(items)
			reviewer.run()
		except Exception as e:
			log(f"Failed to open Tk UI: {e}")
			return 1

		log("Done. Review complete.")

	# Optional coverage generation (run after diff review so UX not blocked by gcovr)
	if should_run_coverage(args):
		log("\nCoverage step: generating report (requested)…")
		run_coverage(build_dir)

	return rc


if __name__ == "__main__":
	try:
		sys.exit(main(sys.argv[1:]))
	except KeyboardInterrupt:
		log("Interrupted.")
		sys.exit(130)
