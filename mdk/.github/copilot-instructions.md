<!-- Copilot / AI agent instructions for stm32F4_IIC project (Keil/MDK) -->
# Quick orientation for AI coding agents

- **Big picture:** This is an embedded STM32F4 project built with Keil MDK (uVision). The uVision project files live under `mdk/` and reference real source folders one level up: `..\app`, `..\driver`, `..\third_lib`, `..\hardware`. Treat `mdk/` as the workspace root for build metadata.

- **Where to look first:** inspect `..\app\main.c` for the runtime entry and usage patterns (sensor reads, OLED updates). Open `mdk\stm32F407_template.uvprojx` / `uvoptx` to see include paths, defined macros and which files are compiled.

- **Key components / folders:**
  - `app/` — application logic and `main.c` (entrypoint). Example: `main.c` calls `IIC_Init()`, `OLED_Init()`, `SHT20_GetResult()`.
  - `driver/` — MCU peripheral wrappers (e.g. `IIC.c`/`IIC.h`, `lcd.c`). Drivers use standard ST peripheral headers under `mdk/.pack/...`.
  - `third_lib/` — 3rd-party helpers (e.g. `OLED.c`, `sht20.c`).
  - `hardware/` — CMSIS / startup / system files used by the linker and startup code.
  - `mdk/Objects`, `mdk/Lists`, `mdk/build/` — build artifacts; useful for understanding compiled outputs and map files.

- **Toolchain & build workflow:**
  - Builds are intended for Keil MDK (ARM) — compiler options, device packs and HSE value are set in `mdk/*.uvprojx`.
  - VS Code tasks (in this workspace) wrap Keil build/upload commands: run the `build`, `flash`, `build and flash`, `rebuild`, `clean` tasks in the `mdk` folder. These call `eide.project.build` / `uploadToDevice` bindings.
  - Important compile-time defines seen in project: `STM32F407xx`, `STM32F40_41xxx`, `USE_STDPERIPH_DRIVER`, `HSE_VALUE=8000000` — do not remove or change without verifying linker/startup.

- **Runtime/driver patterns discovered:**
  - I²C (IIC) is used for sensors and OLED; higher-level functions in `third_lib` call into `driver/IIC.h` APIs (e.g. `IIC_Init()`, blocking transfers).
  - SHT20 sensor uses a 'trigger then read' flow with required delays — `SHT20_Trigger_measurement`, wait (max 85ms for T, 29ms for RH), then `SHT20_GetResult()`; `main.c` documents timings.
  - OLED code uses buffered updates (e.g. `OLED_ShowString`, `OLED_ShowFloatNum`, then `OLED_Update()` to flush). Prefer writing to the buffer and then updating.

- **Project-specific conventions:**
  - Source file locations are referenced relative to `mdk` (e.g. `<FilePath>..\app\main.c</FilePath>` in `.uvprojx`). When editing or moving files, update project file paths.
  - Drivers live under `driver/`, third-party helpers under `third_lib/`. Keep public APIs in `*.h` and implementation in `*.c`.
  - Timing-sensitive sensor calls are implemented as blocking delays; follow existing delay helpers (`Delay_ms`) and preserve comments that document timing requirements.

- **Debugging & artifacts:**
  - Debug configurations are in `mdk/DebugConfig/*.dbgconf` for different boards; use those when loading a debug session in Keil.
  - Build logs, map files and HTML listings in `mdk/Objects/` and `mdk/Listings/` show symbol sizes and call chains — useful to inspect when optimizing.

- **What to change cautiously:**
  - Do not change MCU defines or startup files without verifying `startup_stm32f40_41xxx.s` and `system_stm32f4xx.c` — these affect clock and vector setup.
  - Avoid renaming folders (`app`, `driver`, `third_lib`, `hardware`) because `.uvprojx` references relative paths.

- **Examples to reference when patching code:**
  - To add a new sensor driver: follow `third_lib/sht20.c` style (trigger/read, documented delays) and expose a small header in `driver/` if MCU-level I2C access needed.
  - To update display handling: update buffer via `OLED_Show*` calls and call `OLED_Update()` once per frame (see `app/main.c`).

- **If you need more code:** ask to open these files for detailed patterns: `app/main.c`, `driver/IIC.c`, `driver/IIC.h`, `third_lib/sht20.c`, `third_lib/OLED.c`, `mdk/stm32F407_template.uvprojx`.

If any of the above is unclear or you want the instructions expanded (for example to include precise compiler flags or linker scripts), tell me which area to expand and I'll update this file.
