# DevBench GFx 0.3.0

A developer tool for [DevBench](https://www.nexusmods.com/skyrimspecialedition/mods/181326). It lets an agent or script read, set and call anything inside an open Scaleform menu, so RaceMenu, the HUD and other mods' menus can be tested without touching the screen.

This mod is for mod authors and automated testing. It adds nothing to normal play: no gameplay changes, no plugin and no save data. It does nothing unless DevBench is installed.

## Requirements

- [DevBench](https://www.nexusmods.com/skyrimspecialedition/mods/181326)
- [SKSE](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Target versions

Skyrim SE and AE. Tested on AE 1.7.104 only. VR is untested.

## The Problem

DevBench can open menus, answer message boxes and send keys, but it cannot see inside a Flash menu. It can press a key in RaceMenu with no way to know which slider is selected or whether the key landed.

## What It Does

It adds a `gfx` tool to DevBench, with four operations. `path` is dotted, starts at `_root` or `_global`, and uses numbers for array indexes.

| Op | What it does |
|---|---|
| `get` | Reads a value. `depth` defaults to 2, maximum 8. `limit` caps array items, default 500. Results stop at 20,000 values, and a cut result returns `"truncated": true` |
| `set` | Writes `value` |
| `call` | Calls a function with `args`. Objects work, for handlers that take an event, such as `{"index":13}` |
| `batch` | Runs a list of `get`, `set` and `call` steps in `ops`, at most 64, in one UI task. It stops at the first failure and returns `failedIndex` |

Read RaceMenu's races and sliders:

```bash
curl -X POST http://127.0.0.1:8920/api/tool/gfx -H "Content-Type: application/json" -d '{"op":"get","menu":"RaceSex Menu","path":"_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.itemList.entryList","depth":2,"limit":800}'
```

Press a RaceMenu list entry through the menu's own handler:

```bash
curl -X POST http://127.0.0.1:8920/api/tool/gfx -H "Content-Type: application/json" -d '{"op":"call","menu":"RaceSex Menu","path":"_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.onItemPress","args":[{"index":13}]}'
```

Set a slider, commit it and refresh the list in one step:

```bash
curl -X POST http://127.0.0.1:8920/api/tool/gfx -H "Content-Type: application/json" -d '{"op":"batch","menu":"RaceSex Menu","ops":[{"op":"set","path":"_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.itemList.entryList.71.position","value":0.5},{"op":"call","path":"_global.gfx.io.GameDelegate.call","args":["ChangeWeight",[0.5,2]]},{"op":"call","path":"_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.itemList.requestUpdate"}]}'
```

Entry indexes change with your load order, so read the list first. Port 8920 is DevBench's default on SE and AE. DevBench's MCP server exposes the same tool.

## Install

Install with a mod manager. It needs no load order position.

## Known Issues

- `set` and `call` act on the live menu with no safety net. A wrong call can leave a menu in a bad state, so use it on a test character.
- Methods appear as `{}` in `get` output, because this Scaleform version has no function type.
- After `onItemPress` changes the race, RaceMenu's `equipState` stays on the old race. `listState.activeEntry` and the game are correct.
- RaceMenu paths and `callbackName`s come from RaceMenu's menu file, and another interface replacer may differ.
- A step that runs past the 3-second timeout may still finish. A step that has not started is cancelled.

## Build from Source

See `BUILDING.md`.

## Verification Status

Tested in game on AE 1.7.104 under CrossOver on macOS, with DevBench 1.32.0 and a RaceMenu Untarnished UI patched menu:

- `get` on the Main Menu, RaceMenu, HUD, Console, Fader, Cursor and TrueHUD menus. The 8,551-value RaceMenu list read in 32 ms.
- `set`, `call` and `batch`, confirmed from the engine: race change, slider value and name.
- A full character creation from NEW to the end of RaceMenu through RaceMenu's own handlers.
- Bad input returns clean errors: paths through numbers, missing members, bad ops and nested `batch`.

The wide-string path and timeout cancellation are in the code but were not exercised, because neither came up in testing.

Not tested on Windows, SE or VR.

## Credits and Permissions

- [alandtse](https://github.com/alandtse) for [DevBench](https://github.com/alandtse/devbench), which this mod extends and needs. Copyright (c) 2026 Alan Tse and devbench contributors.
- `src/DevBenchAPI.h` and `src/DevBenchAPI.cpp` are DevBench's cross-plugin API, vendored under their MIT licence (`src/DevBenchAPI.LICENSE.txt`). DevBench itself is GPL-3.0 and no other part of it is included.
- Built with [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG).
- expired6978 for RaceMenu, whose menu these examples use.

## License

MIT, see `LICENSE`.
