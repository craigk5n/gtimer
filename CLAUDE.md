# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Setup (first time or after meson.build changes)
meson setup build

# Build
ninja -C build

# Run all tests
meson test -C build

# Run a single test by name (names: basic, db, db-manager, model, utils, service, idle-init, cli, idle-logic)
meson test -C build <test-name>

# Run tests with verbose output
meson test -C build --verbose

# Run the application (needs schema dir for GSettings)
GSETTINGS_SCHEMA_DIR=data ./build/src/gtimer
```

## Architecture

GTimer is a GTK 4 + Libadwaita time tracking app written in C (GNU11), built with Meson.

### Two-layer structure

- **`core/`** - Static library (`gtimer-core`) with no UI dependencies. Contains the data layer and business logic:
  - `db-manager` - SQLite database operations (data stored at `$XDG_DATA_HOME/gtimer/gtimer.db`)
  - `task-object` / `project-object` - GObject data models
  - `task-list-model` - GListModel implementation for task collections
  - `timer-service` - Timer lifecycle management (start/stop/auto-save/midnight rollover)
  - `timer-utils` - Time formatting and calculation helpers
  - `report-generator` - Plain text and HTML report generation
  - `idle-monitor` - Idle detection (Wayland D-Bus or X11/XSS fallback)

- **`src/`** - Application executable, depends on `core_dep`:
  - `main.c` - App startup, CLI argument parsing, GSettings initialization
  - `gtimer-window.c` - Main window with task list, action bar, and all dialog management
  - `report-window.c` - Report generation dialog

### Testing

Tests use the GLib testing framework (`g_test_*`). Each test file is a standalone executable linked against `core_dep`. The `idle-logic` test uses a mock idle monitor (link-time substitution of `idle-monitor.c` with `mock-idle-monitor.c`) to test timer-service idle behavior without real idle detection.

### Key design details

- App ID: `us.k5n.GTimer` (GSettings schema, D-Bus, Flatpak)
- Multiple tasks can run simultaneously
- Supports CLI mode (`--list-tasks`, `--start`, `--stop`, `--report`, etc.) without launching the GUI
- Legacy data in XML format can be imported via `db-manager`

## Code Style

- 2-space indentation, no tabs, 100 char line limit
- Files: `kebab-case.c`, Functions: `gtimer_module_function_name`, Types: `GTimerModuleName`
- GObject patterns: `G_DECLARE_FINAL_TYPE` / `G_DEFINE_TYPE`
- Return type on its own line for function definitions
- Include order: system headers, GLib/GTK headers, local headers
- Error handling via `GError **` parameters
- Use `g_new0()`, `g_free()`, `g_autofree`, `g_autoptr` for memory management
- Tests: file `test-feature.c`, functions `test_module_feature`, use `:memory:` for DB tests

## Reference Documents

- `USER_INTERFACE.md` - Complete UI specification (dialogs, shortcuts, all behaviors)
- `AGENTS.md` - Detailed coding conventions and style guide
- `STATUS.md` - Project progress tracking (update when completing work)
