#!/usr/bin/env python3
"""
Run DuetScreen UI tests and assist with image difference review.

Steps:
1) Delete any *_err images in tests/ref_imgs
2) Run tests (via CTest if available)
3) Review any reference images the run created and any *_err images it produced,
   in a browser page (default) or with terminal prompts (--compare-mode cli).
   See scripts/image_review.py.

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
from typing import List, Optional

from image_review import KIND_DIFF, KIND_NEW, ReviewItem, reference_for, review_in_browser, review_in_terminal

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
	parser.add_argument("--compare-mode", choices=["browser", "cli"], default="browser",
                     help="Review images in a browser page (default) or with terminal prompts")
	parser.add_argument("--review-port", type=int, default=0,
                     help="Port for the browser review page (default: any free port)")
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

		created_refs = list_new_refs_in_dirs(CURRENT_REF_DIRS, before_snapshot)
		if created_refs:
			log(f"  {len(created_refs)} new reference image(s) were created.")
		else:
			log("No newly created reference images were detected.")
	else:
		created_refs = []

	log("Step 5/5: Scanning for *_err images…")
	err_images = list_err_images_in_dirs(CURRENT_REF_DIRS)
	if not err_images:
		log("No *_err images were produced. Visual checks passed.")

	items = [ReviewItem(KIND_NEW, p, rel_to_active_dirs(p)) for p in created_refs]
	items += [ReviewItem(KIND_DIFF, p, rel_to_active_dirs(reference_for(p))) for p in err_images]
	if items:
		log(f"Reviewing {len(items)} image(s)…")
		if args.compare_mode == "cli":
			summary = review_in_terminal(items, log)
		else:
			summary = review_in_browser(items, log, port=args.review_port)
		summary.log_to(log)
		if summary.abandoned:
			return 130
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
