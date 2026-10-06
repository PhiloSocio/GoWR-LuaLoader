# GoWR-LuaLoader

Lua script loader with crash logger for God of War Ragnarok.

Runs as a `winmm.dll` proxy. On game startup it initializes a Lua 5.4 state,
executes every script under the configured script roots (`mod/` and `mods/scripts/`by default), 
and produces a full crash report.

## Features

- Lua 5.4.6 scripting (`print`, `log_info`, `log_warn`, `log_error`, `log_debug`)
- `LOADER` global table exposing loader name, version and author
- Vectored Exception Handler based crash log inspired by Nukem9's design
- Register dump, callstack, symbol and source-line resolution via `dbghelp`
- Breadcrumb ring buffer capturing the last 16 events before a crash
- `spdlog` based leveled logging (console and file)
- TOML configuration
- `winmm.dll` proxy that forwards all 180 system exports via QuickDllProxy

Note: The loader now uses winmm.dll as the proxy DLL, which does not conflict with God of War Ragnarok's PlayStation PC SDK. No separate "real" DLL is required; QuickDllProxy resolves the original exports at runtime.

## Installation

1. Copy `winmm.dll` next to `GoWR.exe`.
2. Create `mods/loader_config.toml` in the game root.
3. Place Lua scripts in either of these locations — both are scanned:

**Option A — flat layout** (Nukem-style):

```
God of War Ragnarok/
└── mods/
    └── scripts/
        └── mymod.lua
```

**Option B — game-internal layout** (Eiton-style):

```
God of War Ragnarok/
└── mod/
    └── int9/
        └── gameart/
            └── scripts/
                └── characters/
                    └── heroa00/
                        └── mymod.lua
```

All `.lua` files under both roots are discovered recursively. Duplicate paths are de-duplicated. Both roots are added to Lua's `package.path`, so `require("int9.gameart.scripts.foo")` and `require("foo")` both resolve.

## Configuration

`mods/loader_config.toml`:

```toml
[Lua]
LoadScripts = true
ScriptRoots = ["mod", "mods/scripts"]
ScanScriptsRecursively = true

[Logging]
ConsoleLogLevel = "warn"
FileLogLevel = "info"
```

Log levels: `off`, `trace`, `debug`, `info`, `warning`, `error`, `critical`.
The first root in `ScriptRoots` has the highest priority in `package.path`,
so `require` resolves there first.
If the file is missing or malformed, the loader falls back to the defaults above and prints a warning at startup.

## Writing Scripts

Scripts can live under either root listed in `ScriptRoots`. Both are scanned recursively, de-duplicated by canonical path, and added to Lua's `package.path`.

```lua
-- mod/int9/gameart/scripts/main.lua
local util = require("int9.gameart.scripts.characters.heroa00.utility")
```

```lua
-- mods/scripts/main.lua
local util = require("utility")
```

Example `mods/scripts/example.lua`:

```lua
log_info("Script loaded")
log_warn("This is a warning")
print("Hello", 42)

if LOADER and LOADER.version then
    log_info("Loader: " .. LOADER.name .. " v" .. LOADER.version .. " by " .. LOADER.author)
end
```

Available globals inside Lua:

| Name | Description |
|---|---|
| `print(...)` | Writes a tab-separated line at info level. |
| `log_info(s)` | Logs at info level. |
| `log_warn(s)` | Logs at warning level. |
| `log_error(s)` | Logs at error level. |
| `log_debug(s)` | Logs at debug level. |
| `LOADER` | Table with `name`, `version`, `author`. |

## Output Files

| File | Contents |
|---|---|
| `mods/logs/loader_log.txt` | Runtime log (startup, config, script load results, errors). |
| `gowr_crash.log` (game root) | Full crash report: exception code, registers, breadcrumbs, callstack. |

_Logs remain under `mods/logs/` regardless of where scripts live._

## Uninstallation

Delete `winmm.dll` from the game folder or rename it like `winmm.dll.bkp`. The real `winmm.dll` living in `C:\Windows\System32` is never touched, so the game runs normally afterwards. The `mods/` folder can be kept or removed.

## Building

- Visual Studio 2022 with the C++ desktop workload
- CMake 3.21+
- Git
- vcpkg (install and set environment variable named `VCPKG_ROOT` to describe its root folder)

```powershell
git clone https://github.com/PhiloSocio/GoWR-LuaLoader.git
cd GoWR-LuaLoader
cmake --preset default
cmake --build --preset release
```

Output: `build/Release/winmm.dll`.

Dependencies (Lua 5.4.6, spdlog, toml11) are resolved through vcpkg using the manifest in `vcpkg.json`. They are statically linked, so the output is a single DLL with no runtime dependencies beyond the Windows system libraries.

## Project Layout

```
src/
├── main.cpp               Loader thread, startup/shutdown
├── ProxyStubs.cpp         winmm.dll proxy and DllMain (QuickDllProxy)
├── CrashHandler.cpp       Vectored Exception Handler, callstack, breadcrumbs
├── LuaManager.cpp         Lua state and script loading
├── Logger.cpp             spdlog wrapper
├── Config.cpp             TOML reader
└── winmm_exports.inc      Proxied export list for QuickDllProxy
resources/version.rc.in    DLL version resource
```

## Credits

- [Nukem9](https://github.com/Nukem9/godofwar-gameplay-tweaks) — crash handler design and logging approach
- [Eiton](https://github.com/Eiton/GoWR-Script-Loader) — DLL proxy + Lua loader concept
- [Darthrolton](https://www.nexusmods.com/profile/Darthrolton) — for his request

## License

MIT.
