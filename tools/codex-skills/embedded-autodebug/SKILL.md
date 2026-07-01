---
name: embedded-autodebug
description: End-to-end embedded debugging workflow for MCU, FPGA, ADC/DAC, STM32CubeMX .ioc, Keil/MDK, Quartus, Make/CMake projects, firmware build/flash, serial-log capture, Keil5 Chinese-encoding fixes, and Git versioning after edits. Use when Codex is asked to debug inaccurate measurements, peripheral failures, clock/configuration mismatches, generated-code issues, Chinese comments rendering as乱码 in Keil, or to inspect local examples, modify configuration/code, compile, capture logs, and commit each completed file change into Git from evidence.
---

# Embedded Autodebug

Use this skill to turn an embedded problem into a closed-loop debug session: inspect the local project, map the hardware/software chain, change the smallest necessary code or configuration, build, optionally flash, capture logs, and update the diagnosis from measured evidence.

## Core Workflow

1. Define the observable failure:
   - Expected input/output values, tolerance, board revision, toolchain, firmware target, and test stimulus.
   - Identify whether the symptom is configuration, clocking, pinmux, bus protocol, algorithm, or measurement setup.

2. Build the chain map before editing:
   - Signal path: source -> analog front end -> ADC/DAC/peripheral -> FPGA/MCU logic -> bus/registers -> application print/display.
   - Clock path: oscillator -> PLL/divider -> peripheral clocks -> sampling/update clocks -> counter gate clocks.
   - Control path: `.ioc`/QSF/BDF/project file -> generated code -> runtime register writes -> UART/logic analyzer/oscilloscope evidence.

3. Search local evidence first:
   - Use `rg` for symbols, register addresses, pin names, clock names, and print strings.
   - Read `.ioc`, `.uvprojx`, `.qsf`, `.bdf`, `.sdc`, linker scripts, startup files, generated init code, and board notes when relevant.
   - Prefer existing local examples and vendor-generated code over inventing a new driver style.

4. Instrument before guessing:
   - Add temporary prints or debug registers for raw counts, status bits, error flags, clock counts, and register values.
   - Print raw data alongside calculated values.
   - For UART/serial tasks, create or reuse a script to capture logs repeatedly with timestamps.
   - Prefer repeatable Python/PowerShell scripts for compile, flash, reset, serial capture, log parsing, and parameter sweeps.

5. Build and validate:
   - Prefer the known local tool paths in `references/local-toolchain.md`; fall back to PATH discovery only when those paths are missing.
   - Locate other available compiler/toolchain commands as needed (`UV4`, `armcc`, `armclang`, `make`, `cmake`, `quartus_sh`, vendor CLI).
   - Run the narrowest build that proves the changed target.
   - If flashing or interacting with hardware is needed, ask before actions that may modify the board state unless the user has already requested flashing/testing.

6. Iterate from evidence:
   - Compare captured data with numeric expectations.
   - If the result is still wrong, update the hypothesis and repeat with a smaller probe.
   - Keep the final answer grounded in file paths, register names, raw readings, and expected ranges.

## Project Versioning

When this skill changes project files, automatically create a local Git version after validation unless the user explicitly says not to.

- Before editing, run `git status --short --branch` and note unrelated user changes. Do not revert unrelated changes.
- After editing and verification, inspect `git status --short` and `git diff --stat`.
- Stage only the intended files for the task, then commit with a concise message such as `Fix ADC sampling clock setup` or `Add UART capture script`.
- If the directory is not a Git repository and the user asks for version management, initialize it, add an embedded-friendly `.gitignore`, and make an initial commit.
- Do not push to GitHub unless the user asks for remote upload or has already asked to manage the GitHub version.
- If a push is requested, confirm `git remote -v`, current branch, and `git status --short --branch` first.
- Prefer Git LFS for large binary embedded artifacts before the first push: PDFs, archives, generated FPGA captures, firmware images, static libraries, and executables. Typical tracking rules: `*.pdf`, `*.zip`, `*.7z`, `*.sof`, `*.pof`, `*.jic`, `*.stp`, `*.lib`, `*.a`, `*.exe`, `*.bin`, `*.hex`.

## Keil5 Encoding

When a Keil5 project file opens with Chinese comments rendered as乱码, preserve the text content and rewrite only the affected source/header file in GBK/ANSI code page 936 unless the repository already standardizes another local encoding.

- Prefer the smallest encoding-only change.
- Do not re-encode unrelated files.
- If a file is intentionally kept in UTF-8, note that Keil5 may need a matching editor setting or file-specific conversion.
- Verify the result by reopening the file in Keil or by checking that the same bytes decode correctly as code page 936.

## GitHub Push Workflow

For this Windows machine, Git may not inherit the browser proxy. If GitHub opens in Chrome but `git push` or `git ls-remote` times out:

1. Check the browser/system proxy:
   - `netsh winhttp show proxy`
   - PowerShell registry read for `HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings` fields `ProxyEnable`, `ProxyServer`, and `AutoConfigURL`.
2. If `ProxyEnable` is `1` and `ProxyServer` is like `127.0.0.1:7892`, set repository-local Git proxy:
   - `git config http.proxy http://127.0.0.1:7892`
   - `git config https.proxy http://127.0.0.1:7892`
3. Verify connectivity before push:
   - `git ls-remote origin main`
4. Push and bind upstream:
   - `git push -u origin main`

If SSH remote `git@github.com:owner/repo.git` fails on port 22, try `ssh://git@ssh.github.com:443/owner/repo.git`. If it then reports `Permission denied (publickey)`, switch to HTTPS if the user is logged in through Git Credential Manager. Keep proxy configuration repository-local, not global, unless the user asks for global Git configuration.

## STM32CubeMX / .ioc Guidance

When `.ioc` configuration is implicated, inspect the `.ioc` text and generated `MX_*_Init` functions together. If changing `.ioc` is requested or clearly necessary:

- Make the minimal `.ioc` change that matches the intended peripheral, pin, DMA, NVIC, or clock configuration.
- Regenerate code only with an available local generator/tool if it exists and the user permits it.
- If regeneration is unavailable, patch generated code carefully and state that the `.ioc` still needs regeneration or manual sync.
- Check that user code remains inside `USER CODE BEGIN/END` blocks.

## FPGA / Signal Chain Guidance

For FPGA-assisted measurement, do not trust module names alone. Trace actual nets through QSF/BDF/Verilog/netlists:

- Confirm which clock drives each counter or sampler.
- Confirm whether ADC data bits are actually used by the measurement algorithm.
- Confirm FMC/SPI/UART register address mappings from both firmware macros and FPGA decode logic.
- Use expected counts. Example: with a 100 ms gate and 150 MHz base clock, base count should be near `15,000,000`; a 1 kHz measured signal should produce about `100` pulses.

## Reference Routing

Read `references/checklists.md` when a task involves build/flash/serial automation, or when you need a concise checklist for MCU/FPGA measurement debugging.

Read `references/local-toolchain.md` before running local embedded tools on this machine.

Read `references/ai-debug-methods.md` when the task asks Codex to debug like an embedded agent: generate configuration/code, write scripts, consult local examples/manuals, run tools, capture logs, or compare algorithm and hardware options.

Read `references/zuolan-signal-project.md` when the workspace or request involves `zuolan_signal_fpga_stm32_v4`, Zuolan STM32/FPGA signal-processing code, the STM32F429 + EP4CE10 FMC bridge, AD/DA waveform generation, AD9959/AD9833, or the project's four documentation files.

## Reporting

Report as a debug result, not a generic lecture:

- State the most likely cause and confidence.
- List the exact files/nets/registers that support it.
- Give the next measurement or code change that would confirm or fix it.
- Distinguish direct evidence from inference.

## STM32 Flash And UART Capture Automation

When a user asks to automate STM32 bring-up, prefer a repeatable project-local script that can build, flash, reset, and capture UART logs.

- Use known local tools first: Keil `D:\Keil_v5\UV4\UV4.exe`, STM32CubeProgrammer CLI `C:\ST\STM32CubeCLT\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`, and Python `D:\minicoda3\envs\newenvs\env\python.exe`.
- Put scripts under the project, for example `tools/flash_and_capture.py`, rather than hard-coding one-off shell commands in the conversation.
- Make serial port, baud rate, capture duration, project path, and hex path command-line arguments. Include `--list-ports`, `--skip-build`, `--skip-flash`, and `--skip-capture` options.
- For flashing through ST-LINK/SWD, use STM32CubeProgrammer CLI with a verify and reset flow, for example `STM32_Programmer_CLI.exe -c port=SWD -w firmware.hex -v -rst`.
- For UART logs, use `pyserial`, timestamp each received line, print it live, and save it under a project-local log directory.
- Do not flash or reset hardware unless the user has requested hardware interaction or approved the action. If serial ports are absent or multiple ports exist, list ports and ask for or require an explicit `--port COMx`.
- In the final report, include the exact hex/axf path, serial port, baud rate, log path, and the raw observed values used for diagnosis.
