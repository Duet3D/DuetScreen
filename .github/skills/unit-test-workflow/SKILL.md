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

## UI Screenshot Tests
- For new UI tests, use `EXPECT_EQUAL_SCREENSHOT` to compare the rendered UI against a reference image.
- If no reference image exists yet, the test run will create it automatically.
- If the screenshot assertion fails, a new image with the same base name and an `_err` suffix will be created for review.
- Review any created reference images or `*_err.png` images through `scripts/run_tests.py --compare-mode cli` before finishing.

## Commands
Use the project virtualenv and test runner script from the workspace root.

### Fast rerun (already configured build)
```bash
env/bin/python scripts/run_tests.py --skip_configure --compare-mode cli
```

### Filtered tests (preferred during iteration)
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --skip_configure --compare-mode cli
```

### Configure + run tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --compare-mode cli
```

### Full run
```bash
env/bin/python scripts/run_tests.py --compare-mode cli
```

### Coverage
```bash
env/bin/python scripts/run_tests.py --coverage --compare-mode cli
```

### LVGL tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --lvgl-tests --compare-mode cli
```

## Required Workflow
1. Start with filtered tests for the changed area when possible.
2. If build/config is stale, rerun without `--skip_configure`.
3. For UI image/LVGL related changes, include LVGL tests and prefer `EXPECT_EQUAL_SCREENSHOT` in new UI test coverage.
4. For logic-heavy or riskier changes, run coverage.
5. Run tests with `--compare-mode cli` so any `*_err.png` images are reviewed in terminal prompts.
6. For every produced `*_err.png`, explicitly decide to update or skip and finish review.
7. If tests fail, capture failing test names and error output, fix, and rerun until green.
8. Report exactly which commands were run and whether they passed.

## Completion Criteria
- Relevant tests pass for the modified area.
- No unaddressed test failures remain.
- Any produced `*_err.png` images were reviewed in CLI mode.
- Any skipped test scope is explicitly called out.
