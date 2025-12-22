<!-- Copilot / AI agent instructions for stm32F4_IIC project -->
# Quick orientation for AI coding agents

- **Project type:** Embedded STM32F4 project (Keil MDK / uVision). Sources are at the repository root (`app/`, `driver/`, `third_lib/`, `hardware/`) and build metadata lives under `mdk/`.
- **Primary goal for edits:** Make minimal, safe changes that keep the Keil project file references intact. When adding/removing source files, update `mdk/*.uvprojx` accordingly (paths are relative to `mdk/`).

## Big picture

- `app/` contains application logic and `main.c` (runtime entry). Start here to understand program flow (initialization → sensor reads → display updates).
- `driver/` contains MCU-level drivers (I2C: `IIC.c/h`, LCD driver: `lcd.c`, `lcd_init.c/h`, delays: `Delay.c/h`, USART). Drivers call CMSIS / vendor HAL pieces from `mdk/.pack/...`.
- `third_lib/` contains higher-level device libraries (OLED, SHT20 sensor). They use `driver/IIC.h` for bus access.
- `hardware/` holds CMSIS/startup and system files required by the linker/startup code.

## Important files to inspect first

- `app/main.c` — runtime flow, init order, example usage of `IIC`, `OLED`, `SHT20`.
- `mdk/stm32F407_template.uvprojx` and `uvoptx` — include paths, defines (e.g. `STM32F407xx`, `HSE_VALUE=8000000`), and the compiled file list.
- `driver/IIC.c`, `driver/IIC.h` — I2C patterns (blocking transfers); used by sensors and OLED.
- `driver/lcd.c` — graphics primitives (e.g. `LCD_DrawLine` uses a Bresenham-like algorithm and always calls `LCD_DrawPoint` which sets address and writes data). Useful when changing display logic.
- `third_lib/sht20.c` — sensor timing expectations (trigger → delay → read). Preserve documented delays.
- `third_lib/OLED.c` — high-level display buffering patterns (write to buffer then flush/update).

## Build / debug workflow (discoverable from repo)

- Toolchain: Keil MDK (ARM). Use Keil to build, link and flash — `mdk/*.uvprojx` is authoritative for build flags.
- Debug configs: `mdk/DebugConfig/*.dbgconf` (use appropriate board config when starting a debug session).
- Artifacts: `mdk/Objects/`, `mdk/Listings/`, `mdk/build/` contain compiled outputs, map files, and listings for size/section review.

## Project-specific conventions and patterns

- Keep driver implementations in `driver/` with the public API in the corresponding `*.h`.
- Sensor libraries in `third_lib/` follow a small-blocking API: trigger/measure/delay/read (see `sht20.c`). Respect timing constants and use `Delay_ms()` from `driver/Delay.c`.
- Display updates are typically done by writing to a framebuffer or issuing many `LCD_WR_DATA` calls; prefer batch writes (set address with `LCD_Address_Set`) to reduce per-pixel overhead (examples in `lcd.c` and `LCD_ShowPicture`).
- Low-level helpers (delays, I2C routines) are blocking — do not convert to non-blocking without updating callers.

## Integration points & external dependencies

- Keil device pack and CMSIS are referenced under `mdk/.pack/Keil/...`. Do not remove or modify these folders; they supply core headers and vendor drivers.
- Hardware-specific defines (device family, HSE) live in `.uvprojx` and affect `system_stm32f4xx.c` and startup behavior.

## Safe change checklist

1. If you change source locations, update `mdk/*.uvprojx` paths.
2. Preserve declared MCU macros (e.g. `STM32F407xx`) unless intentionally retargeting hardware.
3. Maintain documented sensor timing (see `third_lib/sht20.c` comments). Tests on real hardware required for timing-sensitive changes.
4. For display changes, prefer using existing APIs: `LCD_DrawPoint`, `LCD_Address_Set`, `OLED_Update()`.

## Concrete examples to reference

- To add a new I2C sensor: follow `third_lib/sht20.c` (trigger + Delay_ms + read) and call `IIC_*` in `driver/IIC.h` for bus ops.
- To draw a new primitive on LCD: inspect `driver/lcd.c` — `LCD_DrawLine` updates via `LCD_DrawPoint(uRow,uCol,color)` which itself calls `LCD_Address_Set(x,y,x,y)` then `LCD_WR_DATA(color)`.

If anything above is unclear or you want me to expand a section (compiler flags, precise linker files, or example patch), tell me which area to expand and I'll update this file.
