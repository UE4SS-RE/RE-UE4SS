# EventViewerMod

A UE4SS C++ mod that captures Unreal Engine call flow and renders it live in ImGui.

It hooks **ProcessEvent**, **ProcessInternal**, and **ProcessLocalScriptFunction** concurrently and uses a **single unified depth counter** so nested and recursive call chains keep a consistent indentation story (PE → PI → PLSF → …).

## What you can do

- Watch a **live call stack** with depth-indented entries.
- Switch between **Stack** and **Frequency** modes.
- Filter captures with **case-insensitive** whitelist/blacklist substring rules.
- Pause the stream and use right-click context menus to copy names, add filters, or open a focused call-stack modal.
- Save the current view (or everything) to a timestamped text file.

## UI overview

### Enable / Start / Pause

- **Enable** toggles the mod on/off (and persists that setting).
- **Start/Stop** controls whether the middleware is actively capturing and dequeuing.
- **Pause** keeps capturing logic installed but stops dequeuing and UI growth.

### Target filter

The **Target** combo is a *view filter*, not a capture filter:

- **All** shows the call stack exactly as the middleware reports it.
- **ProcessEvent / ProcessInternal / ProcessLocalScriptFunction** show only entries that originated from that hook.

Important: depth is **not** recomputed when you filter. If you hide callers, the remaining entries keep their original depth so you can still read the true nesting structure.

### Modes

- **Stack**: live call stack history (ordered by time, per thread).
- **Frequency**: aggregates by function and tracks how often it appears.

### Thread picker

Captures are grouped by the originating `std::thread::id`. The combo lets you switch which thread you’re viewing. The game thread is labeled with `(Game)` when detected.

### Performance knobs

- **Max MS Read Time** and **Max Count Per Iteration** bound how much work `dequeue()` is allowed to do per ImGui frame.

### Saving captures

- **Save** writes the current thread + current mode to a timestamped file.
- **Save All** dumps both modes for all threads.

## Filtering (case-insensitive)

Whitelist and blacklist entries are **comma-separated tokens**.

- Tokens are trimmed and converted to lowercase (ASCII-only lowercasing).
- Filtering is done against the entry’s cached **lower-cased** strings.
- The UI always displays the original (non-lowercased) names.

Rules:

- **Whitelist**: if empty, everything passes. If non-empty, an entry passes if **any** whitelist token is a substring match.
- **Blacklist**: if any blacklist token is a substring match, the entry fails.
- **Show Tick Functions** is an additional filter gate applied on top.

## Right-click menus

Both stack entries and frequency entries have a right-click menu (when enabled by the current render flags) with helpers such as:

- Copy function/caller names to clipboard
- Add function/caller to whitelist/blacklist
- Open the call stack modal (when the stream is paused)

## Call stack modal

When the stream is paused, the context menu can open a modal window that shows an entry’s root call chain.

Definitions:

- The **root caller** of an entry is the depth `0` entry that began the call chain that ultimately led to the selected entry.

The modal provides:

- **Show full context**
  - Enabled: shows all calls produced by the root caller (the entire subtree under that root).
  - Disabled: shows the path from the root → selected entry, plus the calls triggered by the selected entry.
- **Disable Indent Colors**
  - Mirrors the main window’s behavior.

## Architecture (high-level)

- **Middleware** (`include/Middleware.hpp`, `src/Middleware.cpp`)
  - Owns the UE hooks and pushes lightweight capture entries into a `moodycamel::ConcurrentQueue`.
  - Uses thread-local producer tokens for low overhead under high call volume.
  - Uses a one-time barrier/flag to prevent enqueuing until all hooks are installed (to keep depth sane).

- **Client** (`include/Client.hpp`, `src/Client.cpp`)
  - ImGui renderer + persistent UI state.
  - Dequeues entries, groups by thread, maintains stack/frequency views, and applies filters.

- **StringPool** (`include/StringPool.hpp`, `src/StringPool.cpp`)
  - Interns function and caller strings and returns stable `std::string_view` pairs.
  - Caches both original and lowercased variants.
  - Produces a function hash (from Unreal’s `ComparisonIndex`) to avoid expensive string comparisons in hot paths.

- **EntryCallStackRenderer** (`include/EntryCallStackRenderer.hpp`, `src/EntryCallStackRenderer.cpp`)
  - Manages the call-stack modal’s state and rendering.

## Files and persistence

- UI state is stored as JSON at:
  - `Mods/EventViewerMod/config/settings.json`
- Capture dumps are written to:
  - `Mods/EventViewerMod/captures/`

## Building

This mod is intended to be compiled as a UE4SS C++ mod (MSVC, `/std:c++latest`).

## Notes and gotchas

- String views returned from `StringPool` are stable until the pool is cleared. The current implementation is designed for “grow-only” usage during a session and never clears, but if later implementation does want to support clearing it, they should also clear all threads.
