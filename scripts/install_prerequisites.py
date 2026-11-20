#!/usr/bin/env python3
# pyright: strict
"""Install what building and testing DuetScreen needs, outside the devcontainer, which has it all.

On Linux (Debian or Ubuntu, with apt) it installs the packages, then GCC 15: from apt, from the
ubuntu-toolchain-r/test PPA on Ubuntu, or else built from source into /opt/gcc-15. On macOS it installs
the packages with Homebrew. On both it then makes the Python virtual environment env/ with
requirements.txt; activate it (source env/bin/activate) before building, so python3 and cmake are its.

It needs only Python 3, and sudo on Linux.

    scripts/install_prerequisites.py
    scripts/install_prerequisites.py --skip-gcc     everything but GCC 15
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import sys
import tempfile
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common.project import ROOT, complete, output, run

APT_PACKAGES = [
    'build-essential', 'libsdl2-dev', 'cmake', 'ninja-build', 'libssl-dev', 'libusb-1.0-0-dev', 'pkg-config',
    'libavcodec-dev', 'libavformat-dev', 'libswscale-dev', 'libavutil-dev', 'libpng-dev', 'gdb-multiarch',
    'libgtest-dev', 'imagemagick', 'gcovr', 'libfreetype6', 'libfreetype-dev', 'python3', 'python3-dev',
    'python3-pip', 'python3-venv',
]
"""What Linux needs from apt."""

BREW_PACKAGES = [
    'gcc', 'cmake', 'ninja', 'openssl', 'sdl2', 'libusb', 'pkg-config', 'ffmpeg', 'libpng', 'freetype',
    'imagemagick', 'googletest', 'gcovr', 'tcl-tk', 'python',
]
"""What macOS needs from Homebrew."""

GCC_VERSION = '15.2.0'
"""The GCC built from source when no package has GCC 15."""

GCC_PREFIX = Path('/opt/gcc-15')
"""Where GCC built from source is installed."""

GCC_BUILD_PACKAGES = ['build-essential', 'flex', 'bison', 'wget', 'xz-utils', 'texinfo', 'libgmp-dev',
                      'libmpfr-dev', 'libmpc-dev', 'libisl-dev', 'zlib1g-dev']
"""What building GCC from source needs."""

TOOLCHAIN_PPA = 'ppa:ubuntu-toolchain-r/test'
"""The PPA with newer GCCs for Ubuntu."""

VENV = ROOT / 'env'
"""The Python virtual environment made for the scripts and the build."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    skip_gcc: bool


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--skip-gcc', action='store_true', help='do not install GCC 15 (Linux)')
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def must(ok: bool, what: str) -> None:
    """Stop if a step failed."""
    if not ok:
        sys.exit(f'Failed to {what}')


# Linux


def apt_has(package: str) -> bool:
    """Whether apt can install a package."""
    return output(['apt-cache', 'show', package]) is not None


def apt_install(packages: Sequence[str]) -> bool:
    """Install packages with apt, without the ones they only recommend."""
    return run(['sudo', 'apt-get', 'install', '-y', '--no-install-recommends', *packages])


def is_ubuntu() -> bool:
    """Whether this is Ubuntu, which has the toolchain PPA."""
    release = Path('/etc/os-release')
    return release.exists() and 'ID=ubuntu' in release.read_text().splitlines()


def build_gcc() -> None:
    """Build GCC 15 from source and install it, as gcc-15 and g++-15. It takes a while."""
    print(f'No package has GCC 15, so GCC {GCC_VERSION} is built from source; this takes a while')
    must(apt_install(GCC_BUILD_PACKAGES), "install GCC's build dependencies")
    with tempfile.TemporaryDirectory(prefix='gcc15-') as directory:
        work = Path(directory)
        source = f'gcc-{GCC_VERSION}'
        must(run(['wget', f'https://ftp.gnu.org/gnu/gcc/{source}/{source}.tar.xz'], cwd=work), 'download GCC')
        must(run(['tar', '-xf', f'{source}.tar.xz'], cwd=work), 'unpack GCC')
        (work / 'build').mkdir()
        must(run([work / source / 'configure', f'--prefix={GCC_PREFIX}', '--enable-languages=c,c++',
                  '--disable-multilib'], cwd=work / 'build'), 'configure GCC')
        must(run(['make', '-j', str(os.cpu_count() or 1)], cwd=work / 'build'), 'build GCC')
        must(run(['sudo', 'make', 'install'], cwd=work / 'build'), 'install GCC')
    for compiler in ('gcc', 'g++'):
        must(run(['sudo', 'ln', '-sf', GCC_PREFIX / 'bin' / compiler, f'/usr/local/bin/{compiler}-15']),
             f'link {compiler}-15')


def install_gcc() -> None:
    """Install GCC 15 as gcc-15 and g++-15, unless it is there."""
    if shutil.which('gcc-15') and shutil.which('g++-15'):
        return
    if not apt_has('gcc-15') and is_ubuntu():
        must(run(['sudo', 'apt-get', 'install', '-y', 'software-properties-common']), 'install add-apt-repository')
        must(run(['sudo', 'add-apt-repository', '--yes', '--update', TOOLCHAIN_PPA]), 'add the toolchain PPA')
    if not apt_has('gcc-15') or not apt_install(['gcc-15', 'g++-15']):
        build_gcc()
    print(output(['gcc-15', '--version']) or 'gcc-15 is not on the PATH')


def install_linux(gcc: bool) -> None:
    """Install the packages Linux needs, and GCC 15."""
    if not shutil.which('apt-get'):
        sys.exit('Only Linux distributions with apt (Debian, Ubuntu) are supported')
    must(run(['sudo', 'apt-get', 'update']), 'update the package lists')
    must(run(['sudo', 'apt-get', 'install', '-y', *APT_PACKAGES]), 'install the packages')
    if gcc:
        install_gcc()


# macOS


def install_macos() -> None:
    """Install the packages macOS needs."""
    if not shutil.which('brew'):
        sys.exit('Homebrew is needed on macOS: install it from https://brew.sh and run this again')
    must(run(['brew', 'update']), 'update Homebrew')
    must(run(['brew', 'install', *BREW_PACKAGES]), 'install the packages')


def main() -> int:
    """Install everything for this system."""
    options = parse_options(sys.argv[1:])
    system = platform.system()
    if system == 'Linux':
        install_linux(gcc=not options.skip_gcc)
    elif system == 'Darwin':
        install_macos()
    else:
        sys.exit(f'{system} is not supported: only Linux and macOS are')
    must(run([sys.executable, '-m', 'venv', VENV]), 'make the Python virtual environment')
    must(run([VENV / 'bin' / 'pip', 'install', '-r', ROOT / 'requirements.txt']), 'install the Python packages')
    print(f'Done. Activate the Python environment before building: source {VENV.relative_to(ROOT)}/bin/activate')
    return 0


if __name__ == '__main__':
    sys.exit(main())
