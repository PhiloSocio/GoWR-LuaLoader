# GoWR-LuaLoader

Lua script loader with crash logger for God of War Ragnarok.

Runs as a `version.dll` proxy. On game startup it initializes a Lua 5.4 state, executes every script under `mods/scripts/`, and produces a full crash report if the process hits an unhandled exception.

## Features

- Lua 5.4.6 scripting (`print`, `log_info`, `log_warn`, `log_error`, `log_debug`)
- `LOADER` global table exposing loader name, version and author
- Vectored Exception Handler based crash log inspired by Nukem9's design
- Register dump, callstack, symbol and source-line resolution via `dbghelp`
- Breadcrumb ring buffer capturing the last 16 events before a crash
- `spdlog` based leveled logging (console and file)
- TOML configuration
- `version.dll` proxy that forwards all 13 system exports

## Installation

1. Copy `version.dll` next to `GoWR.exe`.
2. Create the following layout in the game root:

```
God of War Ragnarok/
├── GoWR.exe
├── version.dll
└── mods/
    ├── loader_config.toml
    ├── scripts/
    │   └── example.lua
    └── logs/
```

The loader creates `mods/scripts/` and `mods/logs/` automatically if they are missing.

## Configuration

`mods/loader_config.toml`:

```toml
[Lua]
LoadScripts = true

[Logging]
ConsoleLogLevel = "warn"
FileLogLevel = "info"
```

Log levels: `off`, `trace`, `debug`, `info`, `warning`, `error`, `critical`.

If the file is missing or malformed, the loader falls back to the defaults above and prints a warning at startup.

## Writing Scripts

Place `.lua` files in `mods/scripts/`. They are executed in alphabetical order. A script that fails to compile or throws at runtime is skipped; the rest continue to load.

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

## Uninstallation

Delete `version.dll` from the game folder or rename it like `version.dll.bkp`. The real `version.dll` living in `C:\Windows\System32` is never touched, so the game runs normally afterwards. The `mods/` folder can be kept or removed.

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

Output: `build/Release/version.dll`.

Dependencies (Lua 5.4.6, spdlog, toml11) are resolved through vcpkg using the manifest in `vcpkg.json`. They are statically linked, so the output is a single DLL with no runtime dependencies beyond the Windows system libraries.

## Project Layout

```
src/
├── main.cpp               Loader thread, startup/shutdown
├── ProxyStubs.cpp         version.dll export forwarders, DllMain
├── CrashHandler.cpp       Vectored Exception Handler, callstack, breadcrumbs
├── LuaManager.cpp         Lua state and script loading
├── Logger.cpp             spdlog wrapper
└── Config.cpp             TOML reader
version.def                Export name mapping
resources/version.rc.in    DLL version resource
```

## Credits

- [Nukem9](https://github.com/Nukem9/godofwar-gameplay-tweaks) — crash handler design and logging approach
- [Eiton](https://github.com/Eiton/GoWR-Script-Loader) — DLL proxy + Lua loader concept
- [Darthrolton](https://www.nexusmods.com/profile/Darthrolton) — for his request

## License

MIT.
