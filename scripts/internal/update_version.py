#!/usr/bin/env python3
# pyright: strict
"""Write DuetScreen's version to version.h, as FIRMWARE_VERSION.

Run by the build (CMake's update_version_h target), by CI with the tag or branch it builds, and by
Buildroot, whose copy of the repository has no git history, with the version it took from git. Without
a version, it is git's description of the checkout (git describe), and when that cannot be had the
version is left as it is. The file is only written when the version changes, so it does not make the
build recompile what includes it.

    scripts/internal/update_version.py           the checkout's version
    scripts/internal/update_version.py v3.7.0    that version
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))  # common is in scripts/, this in scripts/internal/

from common.project import ROOT, output

VERSION_H = ROOT / 'version.h'
"""The generated header."""

DEFINE_RE = re.compile(r'^#define FIRMWARE_VERSION "(.*)"$', re.MULTILINE)
"""The version's definition in the header."""


def describe() -> str | None:
    """git's description of the checkout: the last v* tag, the commits since, and whether it is dirty."""
    return output(['git', 'describe', '--tags', '--dirty', '--always', '--match=v*']) or None


def main() -> int:
    """Write the version asked for."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('version', nargs='?', help="the version (default: git's description of the checkout)")
    version: str | None = parser.parse_args().version or describe()
    if version is None:
        print('Could not determine the version from git; version.h is left as it is')
        return 0
    define = f'#define FIRMWARE_VERSION "{version}"'
    if not VERSION_H.exists():
        VERSION_H.write_text(f'#pragma once\n// This file is auto-generated. Do not edit manually.\n\n{define}\n')
    else:
        text = VERSION_H.read_text()
        current = DEFINE_RE.search(text)
        if current and current[1] == version:
            print(f'Version is already up to date: {version}')
            return 0
        VERSION_H.write_text(DEFINE_RE.sub(lambda _: define, text) if current else text.rstrip('\n') + f'\n{define}\n')
    print(f'Updated the version to {version}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
