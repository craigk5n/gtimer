# GTimer

GTimer is a lightweight, modern time tracking application for the GNOME desktop, built with GTK 4 and Libadwaita. It allows you to track time spent on various tasks, organize them into projects, and generate detailed reports.

![GTimer](data/icons/hicolor/scalable/apps/us.k5n.GTimer.svg)

## Features

- **Simplified Tracking**: Uses a daily-total model for robust and easy management.
- **Modern UI**: Clean, Libadwaita-based interface with a secondary action bar for quick access.
- **Real-time Search**: Quickly filter your task list by task or project name.
- **Background Notifications**: Stay informed of active timers even when the app is in the background.
- **Idle Detection**: Automatically pauses timers when you're away (supports Wayland and X11).
- **Auto-save & Rollover**: Automatically saves data every minute and handles midnight transitions seamlessly.
- **Flexible Reports**: Generate Plain Text or HTML reports for any time period (daily, weekly, monthly, yearly).
- **Keyboard Friendly**: Full support for GNOME-compliant keyboard shortcuts.
- **Command Line Interface**: Scriptable CLI for automation and integration with other tools.

## Building and Running

### Prerequisites

- GTK 4
- Libadwaita 1.1 or later
- SQLite 3
- Meson and Ninja
- (Optional) libXss for X11 idle detection fallback

### Build

```bash
meson setup build
meson compile -C build
```

### Run Locally

For the application to find its GSettings schema when running from the build
directory (the schema is compiled into `build/data` automatically during the build):

```bash
GSETTINGS_SCHEMA_DIR=build/data ./build/src/gtimer
```

### Install

```bash
sudo meson install -C build
```

Installation compiles the GSettings schema into the prefix's
`share/glib-2.0/schemas/` directory automatically, so a standard
`--prefix=/usr` install just works.

If you install to a **non-standard prefix** (e.g. `meson setup build
--prefix=$HOME/.local` or some other location), GLib will not search that
prefix by default and the GUI will abort at startup with a missing-schema
error. Put the prefix's data directory on the runtime search path:

```bash
# add to ~/.profile, a wrapper script, or the launching shell
export XDG_DATA_DIRS="<prefix>/share:$XDG_DATA_DIRS"
```

Alternatively point `GSETTINGS_SCHEMA_DIR` directly at the compiled schema:

```bash
export GSETTINGS_SCHEMA_DIR="<prefix>/share/glib-2.0/schemas"
```

To avoid setting an environment variable in every shell, you can install a
small wrapper script inside the prefix and put it on your `$PATH` instead of
`<prefix>/bin`. Create `<prefix>/wrap/gtimer` with:

```sh
#!/bin/sh
DIR=$(dirname "$0") && cd "$DIR" && cd ..
export GSETTINGS_SCHEMA_DIR="./share/glib-2.0/schemas/"
exec ./bin/gtimer
```

Make it executable (`chmod +x <prefix>/wrap/gtimer`) and add `<prefix>/wrap` to
your `$PATH`. The wrapper resolves the schema directory relative to its own
location, so it keeps working even if the prefix is moved.
(Thanks to [@sedererdj](https://github.com/sedererdj) for this approach — see
[#10](https://github.com/craigk5n/gtimer/issues/10).)

## Command Line Usage

GTimer supports command line options for scripting and automation:

```bash
# Show version
gtimer --version

# List all tasks
gtimer --list-tasks

# Show current timer status
gtimer --status

# Start timing a task (by ID from --list-tasks)
gtimer --start 3

# Stop current timer
gtimer --stop

# Stop all timers
gtimer --stop-all

# Add a new task
gtimer --add-task "New Task"

# Add task to a project (use project ID from list)
gtimer --add-task "Task Name" --project 2

# Generate a report
gtimer --report daily
gtimer --report weekly --report-file /tmp/report.txt

# Export tasks to CSV
gtimer --export-csv backup.csv

# Use alternate database directory
gtimer --datadir /path/to/data --list-tasks

# Suppress non-essential output
gtimer --quiet --status
```

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New task |
| `Ctrl+E` | Edit task |
| `Alt+S` | Start/Stop selected task |
| `Alt+T` | Stop all tasks |
| `Ctrl+R` | Generate report |
| `Ctrl+Q` | Quit |
| `Ctrl+Shift+A` | Add annotation |
| `Ctrl+Shift+H` | Hide task |
| `Ctrl+Shift+U` | Unhide tasks |
| `Ctrl+Delete` | Delete task |
| `Ctrl+Shift+I/D` | Adjust time +/- 60 seconds |
| `Ctrl+I/D` | Adjust time +/- 5 minutes |

## Configuration

GTimer stores configuration in GSettings under `us.k5n.GTimer`:

- **auto-save**: Automatically save time entries every minute
- **animate-running-tasks**: Animate running task indicators
- **resume-on-startup**: Resume timing on application startup
- **enable-idle-detection**: Detect idle time and pause timers
- **idle-threshold**: Minutes of idle time before pausing
- **midnight-offset**: Hour (0-23) to start new day

Data is stored in SQLite at `$XDG_DATA_HOME/gtimer/gtimer.db` (typically `~/.local/share/gtimer/`).

## License

GTimer is released under the GPL-2.0 License.
