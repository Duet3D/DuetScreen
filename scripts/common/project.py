# pyright: strict
"""What every script shares: where the project is, running commands, and answering the shell's tab
completion (see scripts/completions)."""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
from collections.abc import Callable, Iterable, Mapping, Sequence
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
"""The DuetScreen repository."""

BUILD = ROOT / 'out' / 'build'
"""Where CMake builds each preset, in a directory named after it."""

ASSETS = ROOT / 'assets'
"""The files the screen loads at run time: the UI, fonts, images and translations."""


def run(command: Sequence[str | Path], cwd: Path = ROOT, env: Mapping[str, str] | None = None) -> bool:
    """Run a command after showing it, and say whether it succeeded. env is added to this process's
    environment."""
    print('$ ' + shlex.join(str(c) for c in command), flush=True)
    full_env = None if env is None else {**os.environ, **env}
    return subprocess.run([str(c) for c in command], cwd=cwd, env=full_env, check=False).returncode == 0


def output(command: Sequence[str | Path], cwd: Path = ROOT) -> str | None:
    """What a command prints, less the trailing newline, or None if it fails or cannot be run."""
    try:
        result = subprocess.run([str(c) for c in command], cwd=cwd, capture_output=True, text=True, check=False)
    except OSError:
        return None
    return result.stdout.rstrip('\n') if result.returncode == 0 else None


def complete(parser: argparse.ArgumentParser) -> None:
    """Answer the shell's completion request, if it made one and argcomplete is installed. Call it once
    every argument has been added, before parsing them."""
    try:
        import argcomplete
    except ImportError:
        return
    argcomplete.autocomplete(parser)


def complete_with(action: argparse.Action, values: Callable[[], Iterable[str]]) -> argparse.Action:
    """Have the shell offer these values for an argument. They are only looked up when the shell asks."""

    def completer(**_: object) -> list[str]:
        return list(values())

    setattr(action, 'completer', completer)
    return action


def complete_files(action: argparse.Action, *suffixes: str) -> argparse.Action:
    """Have the shell offer files with these suffixes, and directories, for an argument."""
    try:
        from argcomplete.completers import FilesCompleter
    except ImportError:
        return action
    setattr(action, 'completer', FilesCompleter(allowednames=[s.lstrip('.') for s in suffixes], directories=True))
    return action
