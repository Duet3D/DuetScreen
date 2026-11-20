#!/usr/bin/env python3
# pyright: strict
"""Make the update archive, out/DuetScreen.tar.gz, from what Buildroot last built (see scripts/build.py).

The screen updates itself from it, from the settings or from a USB drive or SD card. It unpacks it over
its root file system, running its pre-update script first and its post-update script after (both in
config/update). It holds DuetScreen, as built for the screen, and the assets.

    scripts/create_upgrade.py                  DuetScreen and the assets
    scripts/create_upgrade.py --skip-binary    only the assets, to update them without rebuilding
"""

from __future__ import annotations

import argparse
import sys
import tarfile
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common import buildroot, screen
from common.project import ASSETS, ROOT, complete

ARCHIVE = ROOT / 'out' / 'DuetScreen.tar.gz'
"""The update archive."""

HOOKS = ROOT / 'config' / 'update'
"""The scripts the screen runs before and after unpacking it."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    skip_binary: bool


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-B', '--skip-binary', action='store_true', help='leave DuetScreen out: only the assets')
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def contents(binary: bool) -> list[tuple[Path, str]]:
    """What the archive holds, with where each goes on the screen; the hooks and Buildroot's version at its
    root, where the screen looks for them."""
    files = [(HOOKS / 'pre-update', 'pre-update'), (HOOKS / 'post-update', 'post-update'), (ASSETS, screen.ASSETS)]
    if binary:
        for built in (buildroot.BINARY, buildroot.VERSION_FILE):
            if not built.exists():
                sys.exit(f'{built} has not been built: build DuetScreen for the screen first (scripts/build.py T113-Release)')
        files += [(buildroot.BINARY, screen.BINARY), (buildroot.VERSION_FILE, 'buildroot_version')]
    return files


def main() -> int:
    """Make the archive."""
    options = parse_options(sys.argv[1:])
    files = contents(binary=not options.skip_binary)
    ARCHIVE.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(ARCHIVE, 'w:gz', format=tarfile.GNU_FORMAT) as tar:
        for source, target in files:
            print(f'Adding {source.relative_to(ROOT) if source.is_relative_to(ROOT) else source} as {target}')
            tar.add(source, arcname=target.lstrip('/'), filter=screen.as_installed)
    print(f'Update archive: {ARCHIVE}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
