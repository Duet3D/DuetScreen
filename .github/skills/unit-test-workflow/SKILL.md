---
name: unit-test-workflow
description: "Use when running, updating, or validating unit tests in this repository. Covers fast reruns, full test runs, LVGL test runs, and coverage with scripts/run_tests.py."
---

# Unit Test Workflow

## Goal
Run the right level of tests for the change and only finish when relevant tests pass.

## Use When
- Code changes may affect unit tests.
- A user asks to run tests, fix test failures, or validate a patch.
- You need test coverage output for changed logic.

## Commands
Use the project virtualenv and test runner script from the workspace root.

### Fast rerun (already configured build)
```bash
env/bin/python scripts/run_tests.py --skip_configure
```

### Filtered tests (preferred during iteration)
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --skip_configure
```

### Configure + run tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>'
```

### Full run
```bash
env/bin/python scripts/run_tests.py
```

### Coverage
```bash
env/bin/python scripts/run_tests.py --coverage
```

### LVGL tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --lvgl-tests
```

## Required Workflow
1. Start with filtered tests for the changed area when possible.
2. If build/config is stale, rerun without `--skip_configure`.
3. For UI image/LVGL related changes, include LVGL tests.
4. For logic-heavy or riskier changes, run coverage.
5. If tests fail, capture failing test names and error output, fix, and rerun until green.
6. Report exactly which commands were run and whether they passed.

## Completion Criteria
- Relevant tests pass for the modified area.
- No unaddressed test failures remain.
- Any skipped test scope is explicitly called out.
