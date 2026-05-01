---
name: new-code-test-coverage
description: "Use when adding new production code, new features, or new logic paths. Requires adding or updating unit tests for the new behavior, validating coverage with a target above 80% for the changed code when practical, and providing explicit justification before finishing any change that does not meet that bar."
---

# New Code Test Coverage

## Goal
Do not leave new production behavior untested. Add or update unit tests alongside code changes, aim for greater than 80% coverage of the changed logic before finishing, and treat any exception as a case that must be explicitly justified.

## Use When
- A task adds new classes, functions, branches, commands, or state transitions.
- Existing production code gains new behavior.
- A refactor changes behavior or introduces logic that should be protected by tests.

## UI Screenshot Tests
- For new UI tests, use `EXPECT_EQUAL_SCREENSHOT` to compare the rendered UI against a reference image.
- If no reference image exists yet, the test run will create it automatically.
- If the screenshot assertion fails, a new image with the same base name and an `_err` suffix will be created for review.
- Review any created reference images or `*_err.png` images through `scripts/run_tests.py --compare-mode cli` before finishing.

## Required Workflow
1. Identify the new or changed behavior before editing tests.
2. Add new unit tests or extend existing tests in the closest relevant test file.
3. Cover success cases, boundary conditions, and at least one failure or edge path when the code supports it.
4. For new UI tests, prefer `EXPECT_EQUAL_SCREENSHOT` so the UI is validated against reference images.
5. Run targeted tests for the affected area first using `--compare-mode cli`.
6. Review any produced `*_err.png` images in the CLI compare prompts and make an explicit keep/update decision for each.
7. Run coverage for the affected area or the relevant test scope when practical.
8. Do not finish until the new behavior is exercised by tests and coverage is at or above 80%.
9. If that bar cannot be met, the final response must explicitly justify why, identify the remaining untested paths, and state what prevented adding the missing tests.

## Commands
Use the workspace virtualenv from the repository root.

### Targeted tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --skip_configure --compare-mode cli
```

### Configure and run filtered tests
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --compare-mode cli
```

### Coverage
```bash
env/bin/python scripts/run_tests.py --test_filter='<FILTER>' --coverage --compare-mode cli
```

### Full coverage
```bash
env/bin/python scripts/run_tests.py --coverage --compare-mode cli
```

## Completion Criteria
- New or changed production behavior has corresponding test coverage.
- Relevant tests pass.
- Any produced `*_err.png` images were reviewed in CLI mode.
- Coverage for the changed area is above 80% when practical.
- If the target cannot be reached, the response explicitly justifies the gap, names the remaining untested paths, and states the technical blocker.

## Notes
- Prefer focused tests over broad incidental coverage.
- Keep test changes in the same task as the production change.
- Use the existing `unit-test-workflow` skill for broader test execution and failure triage.