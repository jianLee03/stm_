# Agent Notes

## Project Shape
- STM32CubeIDE VS Code CMake project for `STM32F103RBT6` / Cortex-M3 / no FPU, configured in `CMakePresets.json`.
- Real application source is `Src/main.c`; startup, syscalls, and heap support are generated support files in `Src/startup_stm32f103xx.S`, `Src/syscall.c`, and `Src/sysmem.c`.
- `cmake/vscode_generated.cmake` is generated and says not to edit it; add user-maintained build changes in `CMakeLists.txt` instead.
- Linker script is `stm32f103xb_flash.ld`; build outputs are generated under `build/` and ignored by git.

## Build
- Use the presets: `cmake --preset Debug` then `cmake --build --preset Debug`.
- Release build: `cmake --preset Release` then `cmake --build --preset Release`.
- The VS Code settings expect STM32Cube commands: `cube-cmake`, Ninja, and `arm-none-eabi-*` from the STM32Cube bundle/toolchain on PATH.
- Successful builds emit `.elf`, `.hex`, `.bin`, `.map`, and memory usage from the CMake post-build step.

## Firmware Gotchas
- `Reset_Handler` calls `SystemInit` before C runtime init and `main`; keep a `SystemInit(void)` definition available unless adding a real CMSIS system file.
- Current `main.c` uses direct STM32F1 register addresses and assumes the reset/default 8 MHz CPU clock for SysTick timing.
- PA5 is configured through `GPIOA_CRL` bits 20-23; changing the LED pin may require moving between `GPIOx_CRL` and `GPIOx_CRH`.
