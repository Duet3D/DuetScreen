# pyright: strict
"""Reading JSON with comments (JSONC), as the translation files are written: /* block */ and // line
comments are allowed outside strings."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, cast

from common.project import ASSETS

I18N = ASSETS / 'i18n'
"""The translation files, one per language, named after it (en-GB.json)."""

REFERENCE_LANGUAGE = 'en-GB'
"""The language the others are translated from, which has every key."""


def strip_comments(text: str) -> str:
    """JSONC as plain JSON: its comments removed, its strings left alone."""
    out: list[str] = []
    i, in_string = 0, False
    while i < len(text):
        c = text[i]
        if in_string:
            out.append(c)
            if c == '\\':
                out.append(text[i + 1])
                i += 1
            elif c == '"':
                in_string = False
        elif c == '"':
            in_string = True
            out.append(c)
        elif text.startswith('//', i):
            i = text.find('\n', i)
            if i < 0:
                break
            continue
        elif text.startswith('/*', i):
            i = text.index('*/', i + 2) + 2
            continue
        else:
            out.append(c)
        i += 1
    return ''.join(out)


def load(path: Path) -> dict[str, Any]:
    """Read a JSONC file whose top level is an object. Raises ValueError, naming the file, if it is not
    valid."""
    try:
        data = json.loads(strip_comments(path.read_text(encoding='utf-8')))
    except json.JSONDecodeError as e:
        raise ValueError(f'{path}: {e}') from e
    if not isinstance(data, dict):
        raise ValueError(f'{path}: not a JSON object')
    return cast(dict[str, Any], data)
