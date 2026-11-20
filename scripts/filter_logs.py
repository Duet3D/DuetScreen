#!/usr/bin/env python3
# pyright: strict
"""Show DuetScreen's logs, only the lines matching the filters, coloured by level.

The logs are read from the working directory, where the simulator writes them, or with --remote from a
screen over SSH. spdlog rotates them, so the older files (DuetScreen.2.log, DuetScreen.1.log) are shown
first, then DuetScreen.log. Each filter is a case-insensitive regular expression; a line is shown when
any of them matches it, and the matches are highlighted.

    scripts/filter_logs.py Connection Model             lines mentioning either
    scripts/filter_logs.py --follow "\\[ERROR\\]"         errors as they are logged
    scripts/filter_logs.py -r 192.168.1.20 -f -o out.log  follow a screen's log into a file
"""

from __future__ import annotations

import argparse
import os
import re
import sys
import time
from collections.abc import Iterator, Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

from common import screen
from common.project import complete

LOCAL_LOG = Path('DuetScreen.log')
"""The simulator's log, in the directory it was run from."""

POLL_INTERVAL = 0.1
"""Seconds between looks at a followed log for new lines."""

ROTATION_TIMEOUT = 30
"""Seconds to wait for a rotated log to be replaced before giving up following it."""

RESET = '\033[0m'
"""Ends a colour."""

HIGHLIGHT, END_HIGHLIGHT = '\033[7m', '\033[27m'
"""Start and end the inverse video of a filter's match."""

LEVEL_COLOURS = [
    (re.compile(r'\[(CRITICAL|FATAL)\]', re.IGNORECASE), '\033[95m'),  # purple
    (re.compile(r'\[ERROR\]', re.IGNORECASE), '\033[91m'),  # red
    (re.compile(r'\[WARN(ING)?\]', re.IGNORECASE), '\033[93m'),  # yellow
    (re.compile(r'\[INFO\]', re.IGNORECASE), '\033[92m'),  # green
    (re.compile(r'\[DEBUG\]', re.IGNORECASE), '\033[94m'),  # blue
    (re.compile(r'\[TRACE\]', re.IGNORECASE), '\033[97m'),  # white
]
"""The colour of a line, by the first level tag it carries."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    filters: list[str]
    follow: bool
    output: str | None
    remote: str | None


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('filters', nargs='*', help='regular expressions; a line matching any is shown (default: all)')
    parser.add_argument('-f', '--follow', action='store_true', help='keep showing lines as they are logged')
    parser.add_argument('-o', '--output', help='write the lines to this file, uncoloured')
    parser.add_argument('-r', '--remote', metavar='HOST', help="read a screen's logs over SSH")
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


# Showing lines


@dataclass(frozen=True)
class Filter:
    """Which lines to show, and how."""
    patterns: list[re.Pattern[str]]
    colour: bool

    def matches(self, line: str) -> bool:
        """Whether a line is shown."""
        return not self.patterns or any(p.search(line) for p in self.patterns)

    def format(self, line: str) -> str:
        """A line as it is shown: coloured by its level, its matches highlighted."""
        if not self.colour:
            return line
        for pattern in self.patterns:
            line = pattern.sub(lambda m: f'{HIGHLIGHT}{m[0]}{END_HIGHLIGHT}', line)
        colour = next((c for level, c in LEVEL_COLOURS if level.search(line)), None)
        return f'{colour}{line}{RESET}' if colour else line


def show(lines: Iterator[str], filter: Filter, out: TextIO, flush: bool) -> None:
    """Write the lines that match, flushing each one when following a log into a file."""
    for line in lines:
        if filter.matches(line):
            print(filter.format(line), file=out, flush=flush)


# Reading the logs


def rotated_logs(base: Path) -> list[Path]:
    """A log and its rotated files, oldest first."""
    older: list[Path] = []
    while (rotated := base.with_suffix(f'.{len(older) + 1}{base.suffix}')).exists():
        older.append(rotated)
    return older[::-1] + ([base] if base.exists() else [])


def read_local(base: Path) -> Iterator[str]:
    """The lines of a log and its rotated files, oldest first."""
    logs = rotated_logs(base)
    if not logs:
        sys.exit(f'No log at {base.resolve()}')
    for log in logs:
        with log.open(errors='replace') as f:
            for line in f:
                yield line.rstrip('\n')


def follow_local(log: Path) -> Iterator[str]:
    """The lines of a log, then those added to it, carrying on into the new file when it is rotated."""
    position = 0
    inode = log.stat().st_ino if log.exists() else None
    while True:
        deadline = time.monotonic() + ROTATION_TIMEOUT
        while not log.exists():
            if time.monotonic() > deadline:
                sys.exit(f'{log} has not come back after being rotated')
            time.sleep(POLL_INTERVAL)
        if os.stat(log).st_ino != inode:
            inode, position = os.stat(log).st_ino, 0
        with log.open(errors='replace') as f:
            f.seek(position)
            while line := f.readline():
                yield line.rstrip('\n')
            position = f.tell()
        time.sleep(POLL_INTERVAL)


def read_remote(host: str) -> Iterator[str]:
    """The lines of a screen's log and its rotated files, oldest first."""
    base = screen.LOG.removesuffix('.log')
    yield from screen.lines_of(host, f'for n in 9 8 7 6 5 4 3 2 1; do f={base}.$n.log; [ -f $f ] && cat $f; done; '
                                     f'cat {screen.LOG}')


def follow_remote(host: str) -> Iterator[str]:
    """The lines of a screen's log, then those added to it."""
    yield from screen.lines_of(host, f'tail -n +1 -f {screen.LOG}')


def main() -> int:
    """Show the logs asked for."""
    options = parse_options(sys.argv[1:])
    if options.remote:
        lines = follow_remote(options.remote) if options.follow else read_remote(options.remote)
    else:
        lines = follow_local(LOCAL_LOG) if options.follow else read_local(LOCAL_LOG)
    patterns = [re.compile(f, re.IGNORECASE) for f in options.filters]
    try:
        if options.output:
            with open(options.output, 'w') as out:
                show(lines, Filter(patterns, colour=False), out, flush=options.follow)
        else:
            show(lines, Filter(patterns, colour=sys.stdout.isatty()), sys.stdout, flush=False)
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == '__main__':
    sys.exit(main())
