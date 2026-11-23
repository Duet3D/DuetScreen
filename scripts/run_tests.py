#!/usr/bin/env python3
# pyright: strict
"""Build and run DuetScreen's tests, then review the screenshots they changed.

The screenshot tests compare the UI with the reference images in tests/ref_imgs. A test that finds no
reference writes one, and one whose screenshot differs writes it next to the reference with an _err
suffix. Once the tests have run, each of those is reviewed in a browser page (or in the terminal with
--compare-mode cli): accepted, it becomes the reference; rejected, the reference is left as it was.

    scripts/run_tests.py                                  configure, build and run every test
    scripts/run_tests.py -f 'XmlUi.*' --skip-configure    only these tests, without configuring
    scripts/run_tests.py --coverage                       then write tests/report/index.html
    scripts/run_tests.py --lvgl -f test_label             run LVGL's own tests instead
    scripts/run_tests.py --compare-only                   review the images the last run left
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common import cmake
from common.image_review import KIND_DIFF, KIND_NEW, ReviewItem, reference_for, review_in_browser, review_in_terminal
from common.project import ROOT, complete, complete_with, run

LVGL = ROOT / 'libraries' / 'lvgl'
"""LVGL, whose own tests --lvgl runs."""

REFERENCE_DIRS = [ROOT / 'tests' / 'ref_imgs']
"""Where DuetScreen's tests keep their reference images."""

LVGL_REFERENCE_DIRS = [LVGL / 'tests' / 'ref_imgs', LVGL / 'tests' / 'ref_imgs_vg_lite']
"""Where LVGL's tests keep theirs."""

ERR_SUFFIX = '_err.png'
"""The suffix of a screenshot that differs from its reference."""

COVERAGE_HTML = ROOT / 'tests' / 'report' / 'index.html'
"""The coverage report --coverage writes."""

COVERAGE_SOURCES = ROOT / 'src'
"""The sources the coverage report covers."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    filter: str
    preset: str
    coverage: bool
    skip_configure: bool
    skip_build: bool
    compare_only: bool
    lvgl: bool
    compare_mode: str
    review_port: int


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-f', '--filter', default='',
                        help="only the tests matching this: a CTest regular expression, or LVGL's test suite")
    complete_with(parser.add_argument('-p', '--preset', default='Test', help='the CMake preset to test (default: Test)'),
                  cmake.local_presets)
    parser.add_argument('-c', '--coverage', action='store_true', help='write a coverage report to ' +
                        str(COVERAGE_HTML.relative_to(ROOT)))
    parser.add_argument('--skip-configure', action='store_true', help='build without configuring first')
    parser.add_argument('--skip-build', action='store_true', help='run the tests without building them')
    parser.add_argument('--compare-only', action='store_true',
                        help='only review the images the last run left, without running the tests')
    parser.add_argument('--lvgl', action='store_true', help="run LVGL's own tests (libraries/lvgl/tests/main.py)")
    parser.add_argument('--compare-mode', choices=['browser', 'cli'], default='browser',
                        help='review the images in a browser page (default) or in the terminal')
    parser.add_argument('--review-port', type=int, default=0,
                        help="the review page's port (default: any free one)")
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def log(message: str) -> None:
    """Say how the run is going."""
    print(message, flush=True)


# Reference images


def images(dirs: Sequence[Path]) -> set[Path]:
    """The reference images under these directories, not the _err screenshots."""
    return {p for d in dirs if d.exists() for p in d.rglob('*.png') if not p.name.endswith(ERR_SUFFIX)}


def err_images(dirs: Sequence[Path]) -> list[Path]:
    """The _err screenshots under these directories."""
    return sorted(p for d in dirs if d.exists() for p in d.rglob('*' + ERR_SUFFIX))


def label(image: Path, dirs: Sequence[Path]) -> str:
    """An image's name for the reviewer: its path under its reference directory."""
    return next((image.relative_to(d).as_posix() for d in dirs if image.is_relative_to(d)), image.name)


# Running the tests


def run_ctest(preset: str, filter: str) -> bool:
    """Run a preset's tests through CTest, one at a time as they share the screenshot directories."""
    command = ['ctest', '--test-dir', cmake.build_dir(preset) / 'tests', '--output-on-failure', '-j1']
    return run(command + (['-R', filter] if filter else []))


def run_lvgl_tests(filter: str) -> bool:
    """Run LVGL's tests with its own runner."""
    command = [sys.executable, 'tests/main.py'] + ([f'--test-suite={filter}'] if filter else [])
    return run(command, cwd=LVGL)


# Coverage


def gcov_for(preset: str) -> str | None:
    """The gcov matching the GCC a preset is built with, such as gcov-15 for gcc-15, if it is installed;
    None to let gcovr use plain gcov."""
    cache = cmake.build_dir(preset) / 'CMakeCache.txt'
    if not cache.exists():
        return None
    compiler = re.search(r'^CMAKE_CXX_COMPILER:[^=]*=.*?(?:gcc|g\+\+)-(\d+)$', cache.read_text(errors='ignore'),
                         re.MULTILINE)
    gcov = f'gcov-{compiler[1]}' if compiler else None
    return gcov if gcov and shutil.which(gcov) else None


def write_coverage(preset: str) -> bool:
    """Write the coverage report of the tests that have just run."""
    if shutil.which('gcovr') is None:
        log('gcovr is not installed, so there is no coverage report')
        return False
    COVERAGE_HTML.parent.mkdir(parents=True, exist_ok=True)
    command = ['gcovr', cmake.build_dir(preset), '--root', ROOT, '--filter', COVERAGE_SOURCES, '--html-details',
               '--output', COVERAGE_HTML, '-j', str(os.cpu_count() or 1), '--print-summary',
               '--html-title', 'DuetScreen Test Coverage', '--html-theme', 'github.dark-green']
    if gcov := gcov_for(preset):
        command += ['--gcov-executable', gcov]
    if not run(command, cwd=cmake.build_dir(preset)):
        return False
    log(f'Coverage report: {COVERAGE_HTML}')
    return True


# The run


def test(options: Options, dirs: Sequence[Path]) -> tuple[bool, list[Path]]:
    """Build and run the tests; whether they passed, and the reference images they created."""
    for stale in err_images(dirs):
        stale.unlink()
    if not options.lvgl:
        if not options.skip_configure and not cmake.configure(options.preset):
            sys.exit('Configuring failed')
        if not options.skip_build and not cmake.build(options.preset, cmake.default_targets(options.preset)):
            sys.exit('Building the tests failed')
    before = images(dirs)
    passed = run_lvgl_tests(options.filter) if options.lvgl else run_ctest(options.preset, options.filter)
    log('The tests passed' if passed else 'Some tests failed')
    return passed, sorted(images(dirs) - before)


def main() -> int:
    """Run and review the tests asked for."""
    options = parse_options(sys.argv[1:])
    dirs = LVGL_REFERENCE_DIRS if options.lvgl else REFERENCE_DIRS
    passed, created = (True, []) if options.compare_only else test(options, dirs)

    items = [ReviewItem(KIND_NEW, p, label(p, dirs)) for p in created]
    items += [ReviewItem(KIND_DIFF, p, label(reference_for(p), dirs)) for p in err_images(dirs)]
    if items:
        log(f'Reviewing {len(items)} image(s)')
        if options.compare_mode == 'cli':
            summary = review_in_terminal(items, log)
        else:
            summary = review_in_browser(items, log, port=options.review_port)
        summary.log_to(log)
        if summary.abandoned:
            return 130
    elif not options.compare_only:
        log('No screenshot changed')

    if options.coverage and not options.lvgl:
        write_coverage(options.preset)
    return 0 if passed else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
