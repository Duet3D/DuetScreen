# pyright: strict
"""Building DuetScreen for the screen with Buildroot (https://github.com/Duet3D/buildroot-duetscreen),
which has the toolchain for its T113 and builds the rest of its firmware. Its duetscreen package builds
this repository with a T113 CMake preset, and installs the binary and the assets in output/target, which
the SD card image is made from."""

from __future__ import annotations

import os
import re
import shutil
import sys
from collections.abc import Sequence
from pathlib import Path

from common.project import ROOT, run

BUILDROOT = Path(os.environ.get('BUILDROOT_WORKSPACE') or ROOT.parent / 'buildroot-duetscreen')
"""The Buildroot tree: $BUILDROOT_WORKSPACE, as the devcontainer sets it, else beside this repository."""

DEFCONFIG = 'duet3d_duetscreen_defconfig'
"""The configuration of the screen's firmware."""

TARGET = BUILDROOT / 'output' / 'target'
"""The screen's root file system, as Buildroot installs it."""

BINARY = TARGET / 'usr' / 'bin' / 'DuetScreen'
"""DuetScreen as the last build installed it, with its debug symbols."""

VERSION_FILE = TARGET / 'etc' / 'buildroot_version'
"""The version of the Buildroot the screen's firmware was built with."""

IMAGE = BUILDROOT / 'output' / 'images' / 'sdcard.img'
"""The SD card image a full build makes."""

STRIP = BUILDROOT / 'output' / 'host' / 'bin' / 'arm-buildroot-linux-gnueabihf-strip'
"""The toolchain's strip."""

PACKAGE_BUILD = (BUILDROOT / 'output' / 'build' / 'duetscreen-HEAD', BUILDROOT / 'dl' / 'duetscreen')
"""Where Buildroot copies and builds this repository; removed to build it from clean."""

PATH = '/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
"""The PATH Buildroot builds with: the system's tools, without the devcontainer's Python venv or VS Code's
own tools, which are earlier in the PATH the scripts run with."""

SERIAL_NUMBER_RE = re.compile(r'[A-Za-z0-9_.-]*')
"""What a hardware test serial number may contain: it ends up in a make recipe and a C string literal."""


def require() -> None:
    """Stop with a hint unless the Buildroot tree is there."""
    if not (BUILDROOT / 'Makefile').exists():
        sys.exit(f'No Buildroot at {BUILDROOT}: clone https://github.com/Duet3D/buildroot-duetscreen beside this '
                 'repository, or set BUILDROOT_WORKSPACE to where it is')


def options(duet_simulator: bool, hardware_test: str | None) -> list[str]:
    """The CMake options that change what is built from the preset's: the simulated Duet, and a hardware
    test image whose serial numbers start with hardware_test."""
    result: list[str] = []
    if duet_simulator:
        result.append('-DDUETSCREEN_DUET_SIMULATOR=ON')
    if hardware_test is not None:
        if not SERIAL_NUMBER_RE.fullmatch(hardware_test):
            sys.exit("A hardware test serial number may only contain letters, digits, '_', '.' and '-'")
        # The single quotes are removed by the shell running Buildroot's cmake, leaving a C string literal
        result += ['-DDUETSCREEN_HARDWARE_TEST=ON', f"-DDUETSCREEN_HARDWARE_TEST_SERIAL_NUMBER='\"{hardware_test}\"'"]
    return result


def make(targets: Sequence[str], preset: str, cmake_options: Sequence[str] = ()) -> bool:
    """Run Buildroot's make, its duetscreen package building this repository with a T113 preset and these
    CMake options added."""
    env = {'DUETSCREEN_SRC': str(ROOT), 'DUETSCREEN_PRESET': preset,
           'DUETSCREEN_EXTRA_CONF_OPTS': ' '.join(cmake_options), 'PATH': PATH}
    return run(['make', *targets], cwd=BUILDROOT, env=env)


def configure() -> bool:
    """Configure Buildroot for the screen, unless it has a configuration, which is left as it is."""
    return (BUILDROOT / '.config').exists() or run(['make', DEFCONFIG], cwd=BUILDROOT, env={'PATH': PATH})


def clean() -> None:
    """Remove Buildroot's copy and build of this repository, so the next build starts from clean."""
    for path in PACKAGE_BUILD:
        print(f'Removing {path}')
        shutil.rmtree(path, ignore_errors=True)


def stripped(binary: Path, out: Path) -> Path:
    """A copy of a binary without its debug symbols, much smaller to copy to the screen."""
    shutil.copy2(binary, out)
    if not run([STRIP, out]):
        sys.exit(f'Could not strip {binary}')
    return out
