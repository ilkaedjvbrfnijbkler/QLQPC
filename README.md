# QLQPC

`dinput8.dll` proxy for Quake Live (`quakelive_steam.exe`).

## Build and Deploy

Requirements:
- Windows with Visual Studio 2022 (or Build Tools) C++ x86/x64 toolset
- PowerShell

Run from the repository root:

```bat
build.bat [clean] [debug] [nodeploy]
```

- `clean` - Remove `build\` before compiling.
- `debug` - Build Debug configuration instead of Release.
- `nodeploy` - Build `build\dinput8.dll` without deploying to the Quake Live folder.

By default, `build.bat` regenerates `build\gen\webui\webui_embedded_assets.hpp`, compiles `build\dinput8.dll`, and deploys it to your Quake Live folder.

## Active Cvars

| Cvar | Default | Range | Description |
|---|---|---|---|
| `com_maxfps` | `125` | `0`..`10000` | Frame cap (`0` = uncapped). |
| `cl_cmdrate` | `250` | `0`, `30`..`250` | Command rate in Hz (`0` = tied to FPS). |
| `r_vulkan` | `0` | `0`..`1` | Use Vulkan renderer (requires vid_restart). |
| `r_fog` | `0` | `0`..`1` | Enable custom world fog. |
| `r_fogColor` | `0xffffffff` | Hex RGBA/RGB | Fog color (`0xrrggbbff`, `0xrrggbb`, `#rrggbb`, or `rrggbb`). |
| `r_fogDistance` | `768` | `10`..`10000` | Fog distance. |
| `r_fogIntensity` | `1.00` | `0.05`..`10.0` | Fog density. |
| `r_fogOverrideNative` | `1` | `0`..`1` | Override map fog when `r_fog` is on. |
| `cg_drawAccuracy` | `0` | `0`..`1` | Show client/server LG accuracy below FPS. |
| `cg_drawOtherWeapons` | `1` | `0`..`1` | Draw other players' weapons and muzzle flashes. |
| `cg_forceWhiteNames` | `0` | `0`..`1` | Force white text globally. |
| `cg_teamLightningStyle` | `1` | `0`..`5` | Own/team LG style (`0` Beamless, `1` Default, `2` QuakeWorld, `3` Team Arena, `4` Thin Shaft, `5` Wide Beam). |
| `cg_teamLightningOpacity` | `1.00` | `0.00`..`1.00` | Own/team LG beam opacity. |
| `cg_enemyLightningStyle` | `1` | `0`..`5` | Enemy LG style (`0`..`5`). |
| `cg_enemyLightningOpacity` | `1.00` | `0.00`..`1.00` | Enemy LG beam opacity. |
| `cg_playerLean` | `1` | `0`..`50` | Extended range for `cg_playerLean`. |
