#!/usr/bin/env python3
# pyright: strict
"""
Generate assets/object_model/schema.json, the object model's shape, from @duet3d/objectmodel: the typed object model
Duet Web Control uses. Run it again for a new firmware release, with that release's package version.

The schema lists every object type with its fields' types, and every enum with its values. DuetScreen reads it to
know which fields an object model item has before the printer has sent them (src/ObjectModel/Tree/Schema.h), the API
lint checks `om.*` names against it, and docs/XML_UI_OBJECT_MODEL.md is generated from it.

A field's type is one of:
    number, string, boolean, any     a value (any: whatever the printer sends)
    <Type>                           an object type, or an enum (a string with known values)
    <type>[]                         an array of <type>
    {<type>}                         an object whose field names are data, each of <type> (e.g. `global`)

A type has the fields of its whole family: every class derived from its topmost base, since the printer sends
whichever it has (e.g. `move.kinematics` is declared as one kinematics, but may be any).

Usage:
    scripts/generate_om_schema.py [--version 3.6.4] [--out assets/object_model/schema.json]
"""

from __future__ import annotations

import argparse
import io
import json
import re
import sys
import tarfile
import urllib.request
from pathlib import Path
from typing import cast

from common.project import ASSETS, complete, complete_files

PACKAGE = "@duet3d/objectmodel"
REGISTRY = "https://registry.npmjs.org/@duet3d/objectmodel/-/objectmodel-{version}.tgz"

CLASS_RE = re.compile(
    r"^export (?:declare |default )*(?:abstract )?class (\w+)(?:<[^{]*?>)?(?:\s+extends\s+(\w+)(?:<.*?>)?)?.*\{\s*$"
)
ENUM_RE = re.compile(r"^export declare enum (\w+) \{\s*$")
ENUM_VALUE_RE = re.compile(r'^\s*\w+\s*=\s*"([^"]*)"\s*,?\s*$')
FIELD_RE = re.compile(r"^\s*(?:readonly\s+)?(\w+)\??:\s*(.+?);\s*$")
GETTER_RE = re.compile(r"^\s*get\s+(\w+)\(\):\s*(.+?);\s*$")

Classes = dict[str, dict[str, str]]
"""Each class's own fields, as TypeScript types, by its name"""
Bases = dict[str, str]
"""The class each class derives from"""
Enums = dict[str, list[str]]
"""Each enum's values"""
Schema = dict[str, object]
"""The schema, as it is written"""

# Classes that aren't object model types: the containers, and the base every type derives from
NOT_TYPES = {"ModelObject", "ModelCollection", "ModelDictionary", "ModelSet"}


def download(version: str) -> dict[str, str]:
    """The package's declaration files, by path"""
    with urllib.request.urlopen(REGISTRY.format(version=version), timeout=60) as response:
        data = response.read()
    files: dict[str, str] = {}
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
        for member in archive.getmembers():
            if member.isfile() and member.name.endswith(".d.ts") and (f := archive.extractfile(member)):
                files[member.name] = f.read().decode("utf-8")
    return files


def parse(files: dict[str, str]) -> tuple[Classes, Bases, Enums]:
    """Classes (name -> fields, as TypeScript types), their bases, and enums (name -> values)"""
    classes: Classes = {}
    bases: Bases = {}
    enums: Enums = {}
    for text in files.values():
        lines = text.splitlines()
        i = 0
        while i < len(lines):
            line = lines[i]
            if m := ENUM_RE.match(line):
                values: list[str] = []
                i += 1
                while i < len(lines) and not lines[i].strip().startswith("}"):
                    if v := ENUM_VALUE_RE.match(lines[i]):
                        values.append(v.group(1))
                    i += 1
                enums[m.group(1)] = values
            elif m := CLASS_RE.match(line):
                name, base = m.group(1), m.group(2)
                fields: dict[str, str] = {}
                i += 1
                while i < len(lines) and not lines[i].startswith("}"):
                    member = lines[i]
                    stripped = member.strip()
                    if stripped.startswith(("constructor", "static ", "private ", "protected ", "set ")):
                        pass
                    elif g := GETTER_RE.match(member):
                        fields[g.group(1)] = g.group(2)
                    elif "(" not in stripped.split(":")[0] and (f := FIELD_RE.match(member)):
                        fields[f.group(1)] = f.group(2)
                    i += 1
                classes[name] = fields
                if base:
                    bases[name] = base
            i += 1
    return classes, bases, enums


def convert(ts: str, classes: Classes, enums: Enums) -> str:
    """A TypeScript type as a schema type (see the file comment)"""
    parts = [p.strip() for p in split_top(ts, "|") if p.strip() not in ("null", "undefined")]
    if not parts:
        return "any"
    if len(parts) > 1:
        converted = {convert(p, classes, enums) for p in parts}
        if converted <= {"number"}:
            return "number"
        return converted.pop() if len(converted) == 1 else "any"
    t = parts[0]
    if t.endswith("[]"):
        return convert(t[:-2], classes, enums) + "[]"
    if m := re.fullmatch(r"(Array|ModelCollection|ModelSet)<(.+)>", t):
        return convert(m.group(2), classes, enums) + "[]"
    if m := re.fullmatch(r"ModelDictionary<(.+)>", t):
        return "{" + convert(m.group(1), classes, enums) + "}"
    if m := re.fullmatch(r"Record<\s*string\s*,\s*(.+)>", t):
        return "{" + convert(m.group(1), classes, enums) + "}"
    if t in ("number", "bigint"):
        return "number"
    if t in ("string", "boolean", "any"):
        return t
    if t in enums or (t in classes and t not in NOT_TYPES):
        return t
    return "any"


def split_top(ts: str, separator: str) -> list[str]:
    """Split `ts` at `separator` outside angle brackets"""
    parts: list[str] = []
    depth, start = 0, 0
    for i, c in enumerate(ts):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == separator and depth == 0:
            parts.append(ts[start:i])
            start = i + 1
    parts.append(ts[start:])
    return parts


def build(classes: Classes, bases: Bases, enums: Enums, version: str) -> Schema:
    """The schema"""

    def inherited(name: str) -> dict[str, str]:
        """A class's fields, its bases' included"""
        fields: dict[str, str] = {}
        chain: list[str] = []
        base: str | None = name
        while base and base not in NOT_TYPES:
            chain.append(base)
            base = bases.get(base)
        for cls in reversed(chain):
            fields.update(classes.get(cls, {}))
        return fields

    def descendants(name: str) -> list[str]:
        """Every class derived from `name`"""
        found = [c for c, b in bases.items() if b == name]
        return found + [d for c in found for d in descendants(c)]

    def family(name: str) -> str:
        """The topmost base of `name` that is an object model type"""
        while bases.get(name) and bases[name] not in NOT_TYPES:
            name = bases[name]
        return name

    types: dict[str, dict[str, str]] = {}
    for name in classes:
        if name in NOT_TYPES:
            continue
        fields = inherited(name)
        top = family(name)
        for relative in [top] + descendants(top):
            for field, ts in inherited(relative).items():
                fields.setdefault(field, ts)
        types[name] = {field: convert(ts, classes, enums) for field, ts in sorted(fields.items())}
    return {
        "source": f"{PACKAGE} {version}",
        "root": "ObjectModel",
        "types": dict(sorted(types.items())),
        "enums": {name: values for name, values in sorted(enums.items()) if values},
    }


def main(argv: list[str] | None = None) -> int:
    """Write the schema of the package version asked for."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="3.6.4", help=f"{PACKAGE} version (default 3.6.4)")
    complete_files(parser.add_argument("--out", type=Path, default=ASSETS / "object_model" / "schema.json",
                                       help="Schema file written (default: assets/object_model/schema.json)"), ".json")
    complete(parser)
    args = parser.parse_args(argv)

    classes, bases, enums = parse(download(args.version))
    if "ObjectModel" not in classes:
        print("The package has no ObjectModel class; its layout has changed", file=sys.stderr)
        return 1
    schema = build(classes, bases, enums, args.version)
    out: Path = args.out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(schema, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {out}: {len(cast(dict[str, object], schema['types']))} types, "
          f"{len(cast(dict[str, object], schema['enums']))} enums")
    return 0


if __name__ == "__main__":
    sys.exit(main())
