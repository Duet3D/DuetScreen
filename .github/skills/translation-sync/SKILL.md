---
name: translation-sync
description: "Use when adding, renaming, or removing i18n translation keys (especially in assets/i18n/en-GB.json). Ensures all language files are updated and validated with scripts/manage_translations.py before finishing."
---

# Translation Sync

## Goal
Keep all locale files in `assets/i18n/` synchronized with `assets/i18n/en-GB.json`.

## Use When
- A translation key is added, renamed, or removed.
- Text in `assets/i18n/en-GB.json` changes and other locales must be updated.
- A task touches i18n settings labels, screen labels, or message strings.

## Required Workflow
1. Treat `assets/i18n/en-GB.json` as the reference source.
2. Apply the same key changes to every other locale file in `assets/i18n/*.json`.
3. Keep formatting placeholders aligned with English values (for example `{}` / `{:s}` / `{:d}`).
4. Validate after changes by running:

```bash
env/bin/python scripts/manage_translations.py --show-keys
```

5. If validation reports missing keys, extra keys, or placeholder mismatches, fix all issues and rerun validation.
6. Do not finish the task until validation passes.

## Notes
- Do not use interactive mode in automation.
- Keep edits minimal and localized to relevant translation keys.
- Preserve existing language content unless a key must be added/renamed/removed.
