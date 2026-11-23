#!/usr/bin/env python3
# pyright: strict
"""
Manage i18n translation files for DuetScreen.

Uses en-GB.json as the reference file. For each other translation file:
  1. Removes keys not present in the reference.
  2. Reports missing keys (use --show-missing to list them).
  3. Validates that values have the same number of formatting `{}` braces.
  4. Optionally prompts for translations of missing keys (--interactive).

Usage:
  scripts/manage_translations.py [OPTIONS]

Options:
  --show-missing    List every missing key per language file.
  --interactive     Prompt to enter translations for missing keys.
  --dry-run         Report changes without writing files.
  --file LANG       Process only the specified language file (e.g. de-DE.json).
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, cast

from common import jsonc
from common.project import complete, complete_with

I18N_DIR = jsonc.I18N
REFERENCE_FILE = jsonc.REFERENCE_LANGUAGE + ".json"


def load_jsonc(path: Path) -> dict[str, Any] | None:
    """Load a translation file, or None (having said why) if it is not valid JSONC."""
    try:
        return jsonc.load(path)
    except ValueError as e:
        print(f"\n[ERROR] Failed to parse JSON in {e}\n", file=sys.stderr)
        return None


def save_json(path: Path, data: dict[str, Any]) -> None:
    """Write data as pretty-printed JSON (no comments)."""
    path.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


# ---------------------------------------------------------------------------
# Key flattening / inflation helpers
# ---------------------------------------------------------------------------

def flatten_keys(d: dict[str, Any], prefix: str = "") -> dict[str, Any]:
    """Flatten a nested dict into dot-separated key -> value pairs."""
    items: dict[str, Any] = {}
    for k, v in d.items():
        full_key = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            items.update(flatten_keys(cast(dict[str, Any], v), full_key))
        else:
            items[full_key] = v
    return items


def set_nested(d: dict[str, Any], dotted_key: str, value: Any) -> None:
    """Set a value in a nested dict using a dot-separated key."""
    parts = dotted_key.split(".")
    for part in parts[:-1]:
        d = d.setdefault(part, {})
    d[parts[-1]] = value


def delete_nested(d: dict[str, Any], dotted_key: str) -> bool:
    """Delete a value from a nested dict using a dot-separated key.

    Returns True if the key was found and deleted.
    Cleans up empty parent dicts afterwards.
    """
    parts = dotted_key.split(".")
    parents: list[tuple[dict[str, Any], str]] = []
    node: Any = d
    for part in parts[:-1]:
        if not isinstance(node, dict) or part not in node:
            return False
        parents.append((cast(dict[str, Any], node), part))
        node = cast(dict[str, Any], node)[part]
    if not isinstance(node, dict) or parts[-1] not in node:
        return False
    del node[parts[-1]]
    # Clean up empty parent dicts
    for parent, key in reversed(parents):
        if not parent[key]:
            del parent[key]
    return True


# ---------------------------------------------------------------------------
# Format-brace counting
# ---------------------------------------------------------------------------

_FMT_BRACE_RE = re.compile(r"\{[^}]*\}")


def count_format_braces(value: object) -> int:
    """Count the number of {…} formatting placeholders in a string."""
    if not isinstance(value, str):
        return 0
    return len(_FMT_BRACE_RE.findall(value))


# ---------------------------------------------------------------------------
# Core processing
# ---------------------------------------------------------------------------

def process_file(
    ref_flat: dict[str, Any],
    lang_path: Path,
    *,
    show_keys: bool,
    remove_extra: bool,
    interactive: bool,
    dry_run: bool,
) -> bool:
    """Process a single translation file. Returns True if any issues found."""

    lang_data = load_jsonc(lang_path)
    if lang_data is None:
        print(f"[SKIP] Skipping file due to JSON error: {lang_path}")
        return True
    translations = lang_data.get("translations", {})
    lang_flat = flatten_keys(translations)
    ref_keys = set(ref_flat.keys())
    lang_keys = set(lang_flat.keys())

    extra_keys = sorted(lang_keys - ref_keys)
    missing_keys = sorted(ref_flat.keys() - lang_flat.keys())

    # --- Format-brace validation ---
    brace_mismatches: list[tuple[str, int, int]] = []
    for key in sorted(ref_keys & lang_keys):
        ref_count = count_format_braces(ref_flat[key])
        lang_count = count_format_braces(lang_flat[key])
        if ref_count != lang_count:
            brace_mismatches.append((key, ref_count, lang_count))

    has_issues = bool(extra_keys or missing_keys or brace_mismatches)

    # --- Print summary ---
    lang_name = lang_path.name
    readable = lang_data.get("readable", "")
    print(f"\n{'=' * 60}")
    print(f"  {lang_name}  ({readable})")
    print(f"{'=' * 60}")
    print(f"  Total reference keys : {len(ref_keys)}")
    print(f"  Translated keys      : {len(ref_keys & lang_keys)}")
    print(f"  Missing keys         : {len(missing_keys)}")
    print(f"  Extra keys           : {len(extra_keys)}")
    print(f"  Format mismatches    : {len(brace_mismatches)}")

    if missing_keys and show_keys:
        print(f"\n  Missing keys:")
        for key in missing_keys:
            print(f"    - {key}  (ref: {ref_flat[key]!r})")

    if extra_keys and show_keys:
        print(f"\n  Extra keys:")
        for key in extra_keys:
            print(f"    - {key}  (translation: {lang_flat[key]!r})")

    if brace_mismatches:
        print(f"\n  Format brace mismatches:")
        for key, ref_c, lang_c in brace_mismatches:
            print(
                f"    - {key}: expected {ref_c} placeholder(s), "
                f"got {lang_c}  "
                f"(ref: {ref_flat[key]!r}, "
                f"translation: {lang_flat[key]!r})"
            )

    # --- Remove extra keys ---
    modified = False
    if extra_keys and remove_extra:
        for key in extra_keys:
            delete_nested(translations, key)
        modified = True
        print(f"\n  Removed {len(extra_keys)} extra key(s).")
    elif extra_keys and interactive:
        print(f"\n  Remove extra keys? (y/n/key-by-key)")
        removed = 0
        for key in extra_keys:
            try:
                answer = input(f"    Remove {key} [{lang_flat[key]!r}]? (y/n): ").strip().lower()
            except (EOFError, KeyboardInterrupt):
                print("\n  (input cancelled)")
                break
            if answer == "y":
                delete_nested(translations, key)
                removed += 1
                modified = True
                if not dry_run:
                    lang_data["translations"] = translations
                    save_json(lang_path, lang_data)
        if removed:
            print(f"  Removed {removed} extra key(s).")

    # --- Interactive mode: prompt for missing translations ---
    if interactive and missing_keys:
        print(f"\n  Enter translations for missing keys (empty to skip):")
        for key in missing_keys:
            ref_val = ref_flat[key]
            try:
                user_input = input(f"    {key} [{ref_val!r}]: ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\n  (input cancelled)")
                break
            if user_input:
                set_nested(translations, key, user_input)
                modified = True
                if not dry_run:
                    lang_data["translations"] = translations
                    save_json(lang_path, lang_data)

    # --- Write back ---
    if modified:
        if dry_run:
            print(f"\n  [dry-run] Would write changes to {lang_name}")
        else:
            lang_data["translations"] = translations
            save_json(lang_path, lang_data)
            print(f"\n  Wrote changes to {lang_name}")

    return has_issues


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "-v",
        "--show-keys",
        action="store_true",
        help="List missing and extra keys per language file.",
    )
    parser.add_argument(
        "-i",
        "--interactive",
        action="store_true",
        help="Prompt to enter translations for missing keys.",
    )
    parser.add_argument(
        "-r",
        "--remove-extra",
        action="store_true",
        help="Remove keys not present in the reference file.",
    )
    parser.add_argument(
        "-d",
        "--dry-run",
        action="store_true",
        help="Report changes without writing files.",
    )
    complete_with(parser.add_argument(
        "-f",
        "--file",
        type=str,
        default=None,
        metavar="LANG",
        help="Process only the specified language file (e.g. de-DE.json).",
    ), lambda: sorted(p.name for p in I18N_DIR.glob("*.json")))
    complete(parser)
    args = parser.parse_args()

    ref_path = I18N_DIR / REFERENCE_FILE
    if not ref_path.exists():
        print(f"Error: reference file not found: {ref_path}", file=sys.stderr)
        return 1

    ref_data = load_jsonc(ref_path)
    if ref_data is None:
        return 1
    ref_flat = flatten_keys(ref_data.get("translations", {}))

    print(f"Reference: {REFERENCE_FILE} ({len(ref_flat)} keys)")

    lang_files = sorted(I18N_DIR.glob("*.json"))
    if args.file:
        target = I18N_DIR / args.file
        if not target.exists():
            print(f"Error: file not found: {target}", file=sys.stderr)
            return 1
        lang_files = [target]

    any_issues = False
    for lang_path in lang_files:
        if lang_path.name == REFERENCE_FILE:
            continue
        issues = process_file(
            ref_flat,
            lang_path,
            show_keys=args.show_keys,
            remove_extra=args.remove_extra,
            interactive=args.interactive,
            dry_run=args.dry_run,
        )
        any_issues = any_issues or issues

    # In interactive mode, re-check for issues after possible user edits
    if args.interactive and not args.dry_run:
        # Reload and re-validate all files
        post_issues = False
        for lang_path in lang_files:
            if lang_path.name == REFERENCE_FILE:
                continue
            lang_data = load_jsonc(lang_path)
            if lang_data is None:
                post_issues = True
                continue
            translations = lang_data.get("translations", {})
            lang_flat = flatten_keys(translations)
            ref_keys = set(ref_flat.keys())
            lang_keys = set(lang_flat.keys())
            extra_keys = lang_keys - ref_keys
            missing_keys = ref_keys - lang_keys
            brace_mismatches: list[tuple[str, int, int]] = []
            for key in ref_keys & lang_keys:
                ref_count = count_format_braces(ref_flat[key])
                lang_count = count_format_braces(lang_flat[key])
                if ref_count != lang_count:
                    brace_mismatches.append((key, ref_count, lang_count))
            if extra_keys or missing_keys or brace_mismatches:
                post_issues = True
        if not post_issues:
            print("\nAll translation files are in sync after interactive edits.")
            return 0
        else:
            print("\nTranslation file issues remain after interactive edits.")
            return 1

    if not any_issues:
        print("\nAll translation files are in sync.")
        return 0
    else:
        print("\nTranslation file issues detected.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
