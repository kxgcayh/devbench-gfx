# DevBench GFx 0.1.0

A developer tool for [DevBench](https://www.nexusmods.com/skyrimspecialedition/mods/181326). It lets an agent or script read, set and call anything inside an open Scaleform menu, such as RaceMenu's sliders. Without it, DevBench can send keys to a menu but cannot see what the menu holds.

It adds no gameplay, no plugin and no save data. It does nothing unless DevBench is installed.

## Requirements

- [DevBench](https://www.nexusmods.com/skyrimspecialedition/mods/181326) 1.5.0 or newer
- SKSE and Address Library

## Target versions

Skyrim SE and AE. Built on CommonLibSSE-NG with the same setup as the author's other plugins. VR is untested.

## Usage

Three operations go through DevBench's `menu` tool. `path` is dotted, starts at `_root` or `_global`, and uses numbers for array indexes.

```bash
bash tools/devbench-cli.sh tool menu '{"action":"invoke","name":"gfx","op":"get","menu":"RaceSex Menu","path":"_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.itemList.entryList","depth":2}'
bash tools/devbench-cli.sh tool menu '{"action":"invoke","name":"gfx","op":"set","menu":"RaceSex Menu","path":"_root.Some.value","value":1}'
bash tools/devbench-cli.sh tool menu '{"action":"invoke","name":"gfx","op":"call","menu":"RaceSex Menu","path":"_global.gfx.io.GameDelegate.call","args":["PlaySound",["UIMenuOK"]]}'
```

`get` takes `depth` (default 2, maximum 8) and `limit` (array items, default 500). Results are capped at 20,000 values.

Methods appear as `{}` in `get` output, because this Scaleform version has no function type.

## Install

Install with a mod manager. It needs no load order position.

## Build from Source

See `BUILDING.md`.

## Verification Status

Built and checked statically: the DLL exports the SKSE entry points. Not yet run in game.

## Credits and Permissions

`src/DevBenchAPI.h` and `src/DevBenchAPI.cpp` are the MIT-licensed consumer API from [DevBench](https://github.com/alandtse/devbench) by alandtse. Their licence is `src/DevBenchAPI.LICENSE.txt`.

## License

MIT, see `LICENSE`.
