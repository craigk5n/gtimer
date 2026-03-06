# IDEAS.md - Feature Ideas and Competitive Analysis

> Competitive analysis of time tracking applications across Linux, Mac, and Windows,
> with prioritized feature ideas for GTimer.
>
> Last updated: 2026-03-06

---

## Competitive Landscape

### Desktop-Native Apps (Linux)

| App | Type | Key Differentiators |
|-----|------|---------------------|
| **Hamster** | GTK/GNOME | Activity@category format, tags, workspace tracking, graphical reports, GNOME Shell extension |
| **Timewarrior** | CLI | Tags, JSON export, calendar backfill, correction commands, terminal charts |
| **Watson** | CLI (Python) | Projects + tags, JSON/CSV export, filtering by project/tag/date |
| **ActivityWatch** | Cross-platform | Fully automatic tracking, privacy-first (local data), open source |

### Cross-Platform (Web + Desktop)

| App | Type | Key Differentiators |
|-----|------|---------------------|
| **Toggl Track** | Freemium SaaS | Autotracker (keyword-based), Pomodoro, calendar view, cross-device sync, browser extension |
| **Clockify** | Freemium SaaS | Auto-tracker (app usage), Pomodoro, offline mode, team features, kiosk mode |
| **Harvest** | Paid SaaS | Invoicing from tracked time, expense tracking, budget tracking, accounting integrations |
| **Kimai** | Self-hosted OSS | Multi-user, invoicing, LDAP/SAML, budgets, plugin system, REST API |
| **RescueTime** | Freemium SaaS | Fully automatic, productivity scoring, distraction blocking, focus sessions |

### macOS-Exclusive

| App | Type | Key Differentiators |
|-----|------|---------------------|
| **Timing** | Paid | Fully automatic tracking per window title, rule-based auto-categorization, Screen Time import |
| **Timemator** | Paid | Automatic tracking with manual override, clean UI |

---

## GTimer Current Feature Summary

**Core**: Task timing (start/stop/multiple concurrent), projects, manual time adjustments, annotations, hide/unhide tasks, search, keyboard shortcuts.

**Data**: SQLite storage, CSV export, backup/restore, SQLite raw export, XML import from legacy.

**Reports**: Daily/weekly/monthly/yearly aggregation, HTML and plain text output, print support.

**Desktop Integration**: Idle detection (Wayland + X11), auto-save, resume on startup, midnight rollover, background notifications, GSettings preferences, i18n.

**CLI**: Full feature parity via command-line interface.

---

## Feature Gaps Identified

### Gap 1: No Tags/Labels System
Every major competitor supports tags or labels for cross-cutting categorization (e.g., "meeting", "deep-work", "billable"). GTimer only has the project/task hierarchy.

### Gap 2: No Data Visualization
Hamster, Toggl, Clockify, and RescueTime all provide charts and graphs. GTimer reports are text/HTML tables only.

### Gap 3: No Pomodoro / Focus Timer
Toggl and Clockify both offer built-in Pomodoro timers. This is a commonly requested productivity feature.

### Gap 4: No Calendar/Timeline View
Toggl and Clockify show time entries on a calendar/timeline. GTimer only has a flat task list.

### Gap 5: No Data Sync or Export Formats Beyond CSV
Competitors offer JSON export, iCalendar, and cloud sync. GTimer only exports CSV.

### Gap 6: No Goal/Budget Tracking
Kimai and RescueTime support time budgets and goals (e.g., "spend 4h/day on deep work"). GTimer has no target/goal system.

### Gap 7: No Undo for Time Operations
Many competitors have full undo. GTimer's time adjustments and deletions are permanent.

### Gap 8: No Reminders/Notifications to Track
Toggl and Clockify remind users if they forget to start tracking. GTimer only notifies on idle and background timing.

### Gap 9: No Week/Day Navigation in Main View
The main view only shows "today" column data. There's no way to browse past days without generating a report.

### Gap 10: No Keyboard-Driven Quick Entry
Hamster's "activity@category #tag" and Watson's "watson start project +tag" allow very fast entry. GTimer requires opening a dialog.

---

## Prioritized Feature Ideas

### Priority 1: High Value, Moderate Effort

**1.1 Tags/Labels System**
Add optional tags to tasks (stored in DB). Display as pills/chips in the UI. Filter task list and reports by tag. CLI: `gtimer start --tag meeting`.
- *Why*: Universal feature across competitors. Enables cross-project categorization.
- *Effort*: DB schema change + UI for tag entry/display + report filtering.

**1.2 Charts and Data Visualization**
Add a "Statistics" view (accessible from hamburger menu) with:
- Bar chart: time per project (daily/weekly/monthly)
- Pie chart: time distribution across projects
- Trend line: daily totals over time
Use Cairo drawing or a lightweight charting approach.
- *Why*: Visual feedback is the #1 reason users choose Toggl/Clockify over simpler tools.
- *Effort*: New view + Cairo rendering. No external dependencies.

**1.3 Quick Entry Bar**
Add a text entry (Ctrl+Space or similar) that accepts "task@project" format for rapid task creation and timing start, without opening a dialog.
- *Why*: Reduces friction for power users. Hamster's key UX advantage.
- *Effort*: Moderate. Parse input, match existing tasks/projects, create if new.

### Priority 2: Medium Value, Lower Effort

**2.1 JSON Export** -- DONE
Add JSON as an export format alongside CSV. Useful for scripting and integration.
- *Why*: Standard interchange format. Timewarrior and Watson both support it.
- *Effort*: Low. Serialize existing data structures.
- *Implemented*: `--export-json FILE` for full data export (projects, tasks, annotations); `--json` flag now works with `--summary`, `--total-time`, `--active-time`; proper JSON string escaping across all JSON output.

**2.2 Tracking Reminders**
Optional notification if no task is being timed for a configurable duration (e.g., 15 minutes). Preference to enable/disable.
- *Why*: Helps users who forget to start tracking. Toggl's most-loved feature.
- *Effort*: Low. Timer + GNotification, similar to existing idle detection.

**2.3 Undo for Destructive Operations**
Implement undo for: delete task, time adjustments, stop-all. Show "Undo" button in AdwToast.
- *Why*: Safety net. AdwToast already supports action buttons.
- *Effort*: Low-moderate. Buffer the previous state, revert on undo click.

**2.4 Day Navigation**
Add left/right arrows or date picker in footer/header to browse past days' data in the main task list view (showing that day's time column).
- *Why*: Currently requires generating a report to see past data.
- *Effort*: Moderate. Query DB for specific date, update column view.

### Priority 3: Nice to Have

**3.1 Pomodoro Mode**
Optional Pomodoro overlay: 25min work / 5min break cycle with notification. Tracks Pomodoro count per task.
- *Why*: Popular productivity technique. Built into Toggl and Clockify.
- *Effort*: Moderate. Timer logic + UI overlay + preference.

**3.2 Time Budgets/Goals**
Set daily or weekly time targets per project or overall. Show progress bar in UI.
- *Why*: Helps users stay on track. Feature of Kimai and RescueTime.
- *Effort*: Moderate. DB schema + UI indicator + preference.

**3.3 Weekly Calendar/Timeline View**
Visual timeline showing time blocks across a week, similar to a calendar view.
- *Why*: Gives spatial understanding of time allocation.
- *Effort*: High. Custom widget with Cairo rendering.

**3.4 GNOME Shell Extension**
Indicator in the top bar showing current task and elapsed time, with start/stop controls.
- *Why*: Hamster's extension is very popular. Quick access without switching windows.
- *Effort*: High. Separate JavaScript extension project.

**3.5 D-Bus Service Interface**
Expose start/stop/status via D-Bus so other tools can integrate with GTimer.
- *Why*: Enables scripting, shell extensions, and third-party integration.
- *Effort*: Moderate. Define interface, implement in gtimer-app.c.

### Priority 4: Long-term / Exploratory

**4.1 Automatic Time Tracking**
Track active window titles and suggest categorization (like RescueTime/Timing).
- *Why*: Eliminates manual tracking entirely for some workflows.
- *Effort*: Very high. Privacy considerations. Platform-specific (Wayland limitation).

**4.2 Invoicing / Billable Hours**
Mark tasks as billable, set hourly rates, generate simple invoices.
- *Why*: Feature of Harvest and Kimai. Useful for freelancers.
- *Effort*: High. Significant UI and data model additions.

**4.3 Multi-Device Sync**
Sync time entries across devices via a self-hosted server or file-based sync.
- *Why*: Key advantage of SaaS competitors.
- *Effort*: Very high. Conflict resolution, auth, networking.

---

## Sources

Research based on:
- [Toggl Track](https://toggl.com/track/)
- [Clockify](https://clockify.me/)
- [Harvest](https://www.getharvest.com/)
- [Kimai](https://www.kimai.org/)
- [RescueTime](https://www.rescuetime.com/)
- [Timing (macOS)](https://timingapp.com/)
- [Hamster](https://github.com/projecthamster/hamster)
- [Timewarrior](https://timewarrior.net/)
- [Watson](https://github.com/jazzband/Watson)
- [ActivityWatch](https://activitywatch.net/)
