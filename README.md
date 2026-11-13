# LeakGuard — Memory Leak Detection Tool

LeakGuard is an educational C++ memory tracking project with a custom hash table, linked-list reporting, allocation/deallocation hooks, and an SDL2 graphical dashboard written in C.

## Features

- Records allocation address, byte count, source file, and line.
- Uses a fixed custom hash table with separate chaining for average constant-time address lookup.
- Keeps a singly linked list for complete report traversal.
- Reports active leaks, totals, repeated deallocation attempts, and unknown-address frees.
- Includes a C + SDL2 dashboard with allocation charts and three interactive demo scenarios (`1`–`3`).
- Includes focused checks for hash lookup and free validation.

## Build and run (macOS with SDL2)

Install SDL2 with Homebrew if needed: `brew install sdl2`. Build a test program with the LeakGuard implementation, then run it:

```sh
clang++ -std=c++17 -Wall -Wextra -pedantic test_dynamic_allocation.cpp MemoryLeakDetector.cpp -o test_dynamic_allocation
./test_dynamic_allocation
```

At normal program exit, LeakGuard writes `leakguard_report.csv`, compiles `LeakGuardGUI.c` into `leakguard_gui`, and launches the dashboard automatically. VS Code's Code Runner setting builds the active C++ source with the tracker; its shutdown hook builds and opens the GUI. The GUI needs SDL2 headers and libraries at `/opt/homebrew` or `/usr/local`.

## Platform note

The automatic report and GUI launcher currently uses macOS APIs. The tracker and dashboard are C++/C respectively, but Windows needs a platform-specific launcher before the same automatic run flow will build there.

## Use the tracker in a program

Include `LeakGuard.h` and allocate with `new`. The default `new` macro records the source location. Call `leakguardTracker().generateReport()` at a point where the report is useful. Define `LEAKGUARD_NO_MACRO` before including the header if source-location tagging is not wanted. The demo test intentionally uses the tracker API directly to test invalid and repeated free attempts without invoking undefined behavior by actually freeing invalid memory.

## Design notes and limitations

A repeated or unknown free is rejected by the explicit `MemoryTracker::recordDeallocation` API and counted in the report. The global delete hook avoids releasing an already-recorded pointer twice. Passing an arbitrary invalid pointer to the language-level `delete` operator is undefined behavior in C++; use the explicit tracker API to validate suspect addresses safely. The test includes one intentional leaked allocation to demonstrate the leak state. This is an educational process-wide hook, not a production performance profiler. Allocation hooks cover ordinary and array `new`/`delete`; over-aligned allocations are not intercepted yet.
