#!/usr/bin/env python3
# pyright: strict
"""Print the notes of a release: the RepRapFirmware versions it works with (version_compatibility.json),
and the changes since the previous tag, grouped by their conventional commit type.

A commit's subject may hold more than one change, each starting with its type and scope, such as
"fix(ui): ... feat(files): ...": each is listed under its own type, with its scope in bold. The release
workflow puts this in the GitHub release.

    scripts/release_notes.py
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from typing import cast

from common.project import ROOT, complete, output

COMPATIBILITY = ROOT / 'version_compatibility.json'
"""The RepRapFirmware versions DuetScreen works with."""

GROUPS = [
    ('fix', '🛠️  Fixes'),
    ('feat', '✨ Features'),
    ('chore', '🧹 Chores'),
    ('refactor', '♻️ Refactoring'),
    ('docs', '📄 Documentation'),
    ('test', '🧪 Tests'),
    ('perf', '⚡ Performance'),
]
"""The heading of each conventional commit type, in the order they are listed."""

CHANGE_RE = re.compile(r'(?=(?:fix|feat|perf|chore|refactor|docs|tests?)\()')
"""Where each change starts in a commit's subject."""

TYPED_RE = re.compile(r'(fix|feat|perf|chore|refactor|docs|test)s?\(([^)]*)\)(.*)', re.DOTALL)
"""A change: its type (tests counts as test), its scope, and the rest, from the colon on."""


def compatibility() -> list[str]:
    """The section on the RepRapFirmware versions."""
    versions = cast(dict[str, str | None], json.loads(COMPATIBILITY.read_text()))
    lines = ['## Version Compatibility:', f'- Minimum Compatible RRF Version: {versions["minCompatibleVersion"]}']
    if versions.get('maxCompatibleVersion') is not None:
        lines.append(f'- Maximum Compatible RRF Version: {versions["maxCompatibleVersion"]}')
    return lines


def changelog() -> list[str]:
    """The section on the changes since the previous tag, or every change if there is none."""
    previous = output(['git', 'describe', '--tags', '--abbrev=0', 'HEAD^'])
    lines = ['## CHANGELOG']
    if previous:
        lines += [f'Changes since {previous}:', '']
        subjects = output(['git', 'log', '--pretty=format:%s', f'{previous}..HEAD'])
    else:
        lines += ['Initial Release', '']
        subjects = output(['git', 'log', '--pretty=format:%s'])

    merges: list[str] = []
    others: list[str] = []
    typed: dict[str, list[str]] = {t: [] for t, _ in GROUPS}
    for subject in (subjects or '').splitlines():
        if not subject:
            continue
        if subject.startswith('Merge '):
            merges.append(subject)
            continue
        changes = [m for part in CHANGE_RE.split(subject) if (m := TYPED_RE.match(part.strip()))]
        for change in changes:
            typed[change[1]].append(f'**{change[2]}**{change[3]}')
        if not changes:
            others.append(subject)

    for heading, entries in [('🔄 Merges', merges), *((h, typed[t]) for t, h in GROUPS), ('Other Changes', others)]:
        if entries:
            lines += ['', f'### {heading}', *(f'* {e}' for e in entries)]
    return lines + ['']


def main() -> int:
    """Print the notes."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    complete(parser)
    parser.parse_args()
    print('\n'.join(compatibility() + [''] + changelog()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
