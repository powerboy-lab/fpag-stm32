# Embedded Autodebug Checklists

## Build / Flash / Serial Loop

1. Locate project entry files: `.ioc`, `.uvprojx`, `.qpf`, `.qsf`, `CMakeLists.txt`, `Makefile`, startup files.
2. Read `local-toolchain.md` and try the known tool paths before searching with `where.exe` on Windows or `which` on Unix.
3. Build before editing when possible to establish a baseline.
4. Add raw debug output: counts, status registers, clock config, error codes, buffer lengths.
5. Rebuild and inspect warnings/errors.
6. Flash only when requested or clearly within the user's stated goal.
7. Capture UART with a small script or existing tool; save logs under the workspace.
8. Compare logs against expected numeric ranges and update the diagnosis.

## Scripted Agent Loop

1. Create a repeatable script when the task needs repeated compile, flash, serial capture, log parsing, or parameter sweeps.
2. Prefer Python for serial/log/data analysis and PowerShell for Windows tool orchestration.
3. Make firmware print machine-parseable lines for raw measurements.
4. Keep generated logs and parsed summaries in the workspace.
5. Re-run the same script after each change so improvements are comparable.

## Example / Manual Search

1. Search local examples and manuals before writing unfamiliar peripheral code.
2. Use official or generated examples for complex MCU/FPGA features.
3. Copy patterns from the closest local working module when project style matters.
4. Treat external blog/video advice as secondary to local project files, datasheets, and official examples.

## Frequency Measurement Checklist

1. Confirm the measured quantity: analog input frequency, ADC sampling clock, FPGA internal clock, DDS output, or timer output.
2. Trace the counter input net. A counter connected to `ADC_CLK` measures sampling frequency, not input signal frequency.
3. Trace the gate/reference counter. Confirm base clock frequency and expected count.
4. Read raw count registers before trusting formatted frequency.
5. For ADC-derived frequency, require an edge detector or period estimator from sample data:
   - midscale threshold for unsigned ADC data,
   - hysteresis to reject noise,
   - one pulse per full period,
   - clock-domain crossing if pulse and counter use different clocks.
6. Check with known stimuli: 1 kHz, 10 kHz, and a stable square wave if available.

## Safe Editing Rules

1. Copy or isolate experiments when the user asks for a test project.
2. Keep unrelated generated files unchanged.
3. Preserve user code regions and existing hardware abstraction style.
4. Prefer temporary instrumentation over large rewrites until the failure is confirmed.
5. Remove or clearly label temporary debug code before finalizing if it should not ship.

## Algorithm And Hardware Boundary

1. Simulate or numerically test signal algorithms before committing firmware changes when practical.
2. Validate against known synthetic and hardware stimuli.
3. Use chip-internal peripherals confidently when supported by manuals/examples.
4. Be cautious with external analog circuit suggestions; calculate and measure before changing hardware.
