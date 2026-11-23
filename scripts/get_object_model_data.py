#!/usr/bin/env python3
# pyright: strict
"""Save a Duet's object model, as the tests use it, from its HTTP API (rr_model).

Each section the screen asks for is saved with the flags it asks with, as model_<key>_<flags>.json, and
the whole model as model_all_d99f.json, to a directory under tests/object_model for a test to load.

    scripts/get_object_model_data.py 192.168.1.123
    scripts/get_object_model_data.py duet.local --out tests/object_model/5_axis
"""

from __future__ import annotations

import argparse
import sys
import urllib.parse
import urllib.request
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from common.project import ROOT, complete, complete_files

SECTIONS = (
    ('network', 'v'),
    ('boards', 'v'),
    ('move', 'vn'),
    ('heat', 'v'),
    ('tools', 'v'),
    ('spindles', 'v'),
    ('directories', 'v'),
    ('fans', 'v'),
    ('job', 'vn'),
    ('sensors', 'v'),
    ('state', 'vn'),
    ('volumes', 'v'),
    ('', 'd99f'),
)
"""The object model keys saved, with the flags they are asked for with; the empty key is the whole model."""

TIMEOUT = 30
"""Seconds to wait for each section."""


@dataclass(frozen=True)
class Options:
    """The command line, typed."""
    host: str
    out: Path


def parse_options(argv: Sequence[str]) -> Options:
    """Read the command line."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('host', help="the Duet's address, e.g. 192.168.1.50, duet.local or https://duet.local")
    complete_files(parser.add_argument('-o', '--out', type=Path, default=ROOT / 'tests' / 'object_model',
                                       help='the directory to save to (default: tests/object_model)'))
    complete(parser)
    return Options(**vars(parser.parse_args(argv)))


def main() -> int:
    """Save each section, carrying on past any that fail."""
    options = parse_options(sys.argv[1:])
    base = options.host if '://' in options.host else f'http://{options.host}'
    options.out.mkdir(parents=True, exist_ok=True)
    failures = 0
    for key, flags in SECTIONS:
        url = f'{base}/rr_model?' + urllib.parse.urlencode({'key': key, 'flags': flags})
        path = options.out / f'model_{key or "all"}_{flags}.json'
        try:
            with urllib.request.urlopen(url, timeout=TIMEOUT) as response:
                path.write_bytes(response.read())
        except OSError as e:
            print(f'{url}: {e}', file=sys.stderr)
            failures += 1
    print(f'Wrote {len(SECTIONS) - failures} files to {options.out}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
