#!/usr/bin/env python3
# pyright: strict
"""Format the code: DuetScreen's with clang-format 19, LVGL's with its own formatter.

DuetScreen's is formatted by the build's clang-format target (libraries/clang-format.cmake), which every
build of DuetScreen also runs, and which installs clang-format 19 in the build tree if it is not on the
PATH. That target is built here in a preset's build tree, configured first if it has not been.

    scripts/format.py           DuetScreen's code
    scripts/format.py --lvgl    LVGL's code (libraries/lvgl), as LVGL formats it
"""

from __future__ import annotations

import argparse
import sys
from collections.abc import Sequence
from dataclasses import dataclass

from common import cmake
from common.project import ROOT, complete, complete_with, run

LVGL = ROOT / 'libraries' / 'lvgl'
"""LVGL, with its formatter in scripts/code-format.py."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    lvgl: bool
    preset: str


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--lvgl', action='store_true', help="format LVGL's code instead")
    complete_with(parser.add_argument('-p', '--preset', default='Simulation',
                                      help='the preset whose build tree formats it (default: Simulation)'),
                  cmake.local_presets)
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def main() -> int:
    """Format the code asked for."""
    options = parse_options(sys.argv[1:])
    if options.lvgl:
        return 0 if run([sys.executable, 'scripts/code-format.py'], cwd=LVGL) else 1
    if not (cmake.build_dir(options.preset) / 'CMakeCache.txt').exists() and not cmake.configure(options.preset):
        return 1
    return 0 if cmake.build(options.preset, ['clang-format']) else 1


if __name__ == '__main__':
    sys.exit(main())
