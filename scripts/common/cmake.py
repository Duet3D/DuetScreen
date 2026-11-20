# pyright: strict
"""The CMake presets (CMakePresets.json) and building them. Each preset builds in out/build/<preset>.
The T113 presets cross-compile for the screen and need Buildroot's toolchain, so they are built through
Buildroot (see common/buildroot.py) rather than here."""

from __future__ import annotations

import json
from collections.abc import Sequence
from pathlib import Path
from typing import cast

from common.project import BUILD, ROOT, run

PRESETS_FILE = ROOT / 'CMakePresets.json'
"""Where the presets are defined."""

SCREEN_PREFIX = 'T113-'
"""What the names of the presets that build for the screen start with."""

DEFAULT_TARGETS = {'Test': ['DuetScreen.tests']}
"""What a preset builds when no target is named, if not DuetScreen."""


def presets() -> list[str]:
    """The configure presets that can be built, in the order CMakePresets.json gives them."""
    data = cast(dict[str, list[dict[str, object]]], json.loads(PRESETS_FILE.read_text()))
    return [str(p['name']) for p in data['configurePresets'] if not p.get('hidden', False)]


def screen_presets() -> list[str]:
    """The presets that build for the screen."""
    return [p for p in presets() if p.startswith(SCREEN_PREFIX)]


def local_presets() -> list[str]:
    """The presets this machine builds itself: the simulator and the tests."""
    return [p for p in presets() if not p.startswith(SCREEN_PREFIX)]


def build_dir(preset: str) -> Path:
    """Where a preset is built."""
    return BUILD / preset


def default_targets(preset: str) -> list[str]:
    """What a preset builds when no target is named."""
    return DEFAULT_TARGETS.get(preset, ['DuetScreen'])


def configure(preset: str, fresh: bool = False, options: Sequence[str] = ()) -> bool:
    """Configure a preset's build tree, from scratch if fresh, with these -D options added."""
    return run(['cmake', '--preset', preset, *(['--fresh'] if fresh else []), *options])


def build(preset: str, targets: Sequence[str], clean: bool = False, jobs: int | None = None) -> bool:
    """Build targets of a configured preset, cleaning it first if asked."""
    command = ['cmake', '--build', '--preset', preset, '--target', *targets]
    if clean:
        command.append('--clean-first')
    if jobs is not None:
        command += ['--parallel', str(jobs)]
    return run(command)
