# DuetScreen – GitHub Copilot Instructions

These instructions guide Copilot (and AI agents) when proposing code or edits in this repository. The code runs on an embedded target with strict CPU and memory constraints and builds via CMake with a Simulation preset for local runs.

## Mission
- Produce small, surgical diffs that fit the existing architecture and style.
- Prefer modern C++23 for clarity and safety without increasing runtime or memory costs on the device.
- Optimize for performance, determinism, and memory efficiency. Avoid allocations in hot paths.

## Architecture & Data Flow
- **Binary + Library**: `DuetScreen` executable links `DuetScreen.lib`; see `src/CMakeLists.txt` for core sources and submodules.
- **Comm** (`src/Comm/`): Connection orchestration to Duet mainboard via USB/WiFi/UART with request/response handling. Key files: `Communication.*`, `Network.*`, `Usb.*`, `Commands.*`, `JsonDecoder.*`, `Thumbnail.*`, `FileInfo.*`.
- **Object Model** (`src/ObjectModel/`): Structured printer state (e.g., `Job`, `Tool`, `Heat`, `Sensor`, `PrinterStatus`) updated by Comm. UI reads via subscribers to be notified of changes.
- **Subscribers** (`src/Subscribers/`): Lightweight observer bridge from Object Model to UI components.
- **UI** (`src/UI/`): LVGL-based views. Structure: `Core/` (screen/root management), `Screens/`, `Components/`, `Widgets/`, `Styles/`. Follow existing patterns in `src/UI/README.md` and component READMEs.
- **i18n** (`assets/i18n/`): JSON language files loaded at runtime (PC: `assets/i18n`, device: `/etc/assets/i18n`). Use nested keys; formatting via `fmt`. Reference `en-GB.json`.
- **Utils/Infrastructure**: `Debug.*` (logging), `DeadlockDetector.*`, `LockWrapper.*`, `BuildDate.*`. Avoid allocations in render/ISR paths.

## Developer Workflows (VS Code + CLI)
- **Remote Deploy**: Requires `buildroot-duetscreen` and SSH.
	- Build + push: `Push DuetScreen - SSH - Release` (or `Debug`).
	- Start service: `Start DuetScreen on remote`; start gdb: `Start gdbserver on DuetScreen`.
	- Assets: `Push Assets - SSH`; libraries: `Push Libraries - SSH`.
	- Package update: `Create update.tar.gz`; print notes: `Print Release Notes`.
- **Testing**: Run `env/bin/python scripts/run_tests.py`. UI image tests under `tests/src/test_cases/UI/` compare against `tests/ref_imgs/` with `_err` diffs and a review GUI.
- **Logs**: Default log file `DuetScreen.log` (PC: CWD; device: `/var/log/`). Filter with `scripts/filter_logs.py`.

## Integration & Assets
- **LVGL**: Primary UI framework; vendored under `libraries/lvgl/`. Calls to raw lvgl functions should be minimised, instead use the wrapper classes defined in `src/UI`. If a wrapper does not currently exists then create or extend an existing one.
- **3rd-Party**: CMake modules in `libraries/*.cmake` (e.g., `lvgl.cmake`, `spdlog.cmake`, `json.cmake`, `nameof.cmake`, `hv.cmake`, `colorm.cmake`). Prefer existing integration points.
- **Configuration**: Non-volatile settings via `config.json` (first boot). Keys use `:`-hierarchy (see `src/Storage.h`, e.g., `ui:move:selected_feedrate`).
- **Assets**: Icons/fonts under `assets/`; device path `/etc/assets/`. Keep asset loading deterministic and bounded.

## Defaults and Constraints
- Standard: C++23. Prefer `target_compile_features(... cxx_std_23)`.
- Exceptions/RTTI: avoid on target paths unless explicitly required. Use `noexcept` where possible.
- Errors: use `std::expected<T, Error>` or `std::error_code` in new/modified code; mark results `[[nodiscard]]` if ignoring is unsafe.
- Ownership: prefer `std::unique_ptr` for owned resources; use raw pointers/`std::span`/`std::string_view` for non-owning references.
- Memory: prefer stack/static storage; preallocate buffers; avoid heap churn and fragmentation.
- Logging: keep logs minimal on target; do not allocate in logging hot paths.

## C++23 Feature Preferences
- Use `constexpr`/`consteval`/`constinit` to shift work to compile time and ensure static initialization safety.
- Use `std::span` for array-like parameters; avoid copies.
- Consider `std::pmr` with `monotonic_buffer_resource` for bounded dynamic needs (preallocated arenas).
- Prefer `std::move_only_function` for callbacks when copyability isn’t needed.
- Use `std::ranges`/`views` when the toolchain support is robust and no extra allocations occur; otherwise prefer classic algorithms.
- Attributes: `[[nodiscard]]` for important returns, `[[likely]]/[[unlikely]]` only when backed by measurement.

## Patterns
- RAII for all resources (files, locks, buffers, LVGL wrappers).
- Static polymorphism (CRTP) or `std::variant` over virtual dispatch in hot paths.
- Strategy/State for behavior selection without branching spaghetti.
- Observer with lightweight callback lists; avoid allocation-heavy event buses.
- Prefer free functions in namespaces over singletons. Pimpl only where ABI boundaries require.

## Performance and Memory
- Measure first; only optimize hotspots. Favor predictable branching and locality.
- Favor contiguous containers (`std::array`, `std::vector` with `reserve`), or `std::pmr::vector` on a preallocated arena.
- For small sets, prefer sorted `std::vector` + binary search instead of node-based containers.
- Avoid dynamic allocation in ISRs, render callbacks, or tight loops.
- Use `std::string_view` for non-owning text; avoid constructing `std::string` unless necessary.

## Error Handling and APIs
- Prefer explicit error channels (`std::expected`, `std::error_code`), no exceptions on target paths.
- Make ownership explicit; document lifetimes when non-obvious.
- Add `noexcept` where the function cannot throw; propagate errors instead of asserting in library-style code.
- Use `enum class` for flags/options. Use `std::to_underlying` when needed.

## Concurrency & Timing
- Prefer single-threaded event-driven UI. If threads are used, keep ownership, lifetimes, and synchronization explicit.
- Use `std::atomic` where needed with clear memory orders; avoid locks in ISRs.
- Use `std::chrono` types; avoid ad-hoc units.

## Tests
- Prefer adding or updating focused tests for changed logic under `tests/`.
- Run tests with the `env/bin/python scripts/run_tests.py` command; get coverage via `env/bin/python scripts/run_tests.py --coverage`.
- Keep tests deterministic and fast; avoid external dependencies in unit tests.

## File Hygiene
- Use `#pragma once` in headers; prefer forward declarations to reduce includes.
- Keep headers light; do not introduce heavy includes into widely used headers.
- Keep diffs minimal: do not reorder includes or reformat unrelated code. Touch only what is necessary.

## Commit & PR Guidance
- Scope: one logical change per commit/PR; keep diffs small and reviewable.
- Message: concise imperative summary plus brief rationale if not obvious.
- Do not rename/move files unless essential to the change.

## When to Ask for Clarification
- Unclear ownership/lifetimes, ABI constraints, or toolchain support for a C++23 feature on the target.
- Changes that require broad refactors or architectural shifts.

Note: If a rule conflicts with existing established patterns in a touched file, match the local style unless there is a clear, measurable benefit to change it—and limit changes to the scope of your edit.
