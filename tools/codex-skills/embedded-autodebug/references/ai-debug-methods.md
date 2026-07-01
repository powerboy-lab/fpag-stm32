# AI Embedded Debug Methods

Use these methods to make Codex act like a local embedded debugging agent instead of a chat-only assistant.

## Project Generation And Configuration

1. Let Codex inspect the existing project before manual edits: `.ioc`, generated `MX_*_Init` code, `.uvprojx`, `.qsf`, `.bdf`, `.sdc`, startup files, linker scripts, and board notes.
2. For STM32CubeMX projects, prefer changing `.ioc` or the generated configuration source intentionally, then rebuild to catch mismatches.
3. If a peripheral setup needs extra logic, add a small configuration/helper function in the project style instead of scattering register writes.
4. Keep generated-code edits inside user-code blocks when possible. If editing outside them is unavoidable, state that regeneration may overwrite the change.

## Scripted Debug Loop

Use scripts when an action will be repeated or when evidence matters more than one-off observation:

1. Write PowerShell scripts for local tool orchestration: compile, clean, call vendor CLI, copy artifacts, launch serial tools.
2. Write Python scripts for serial capture, binary/log parsing, register table decoding, parameter sweeps, and numeric analysis.
3. Store logs in the workspace with timestamps.
4. Print raw data as CSV-like lines when possible so scripts can parse them.
5. Iterate as: edit -> build -> flash if approved -> capture serial -> parse -> compare with expected values -> revise hypothesis.

## Local Examples And Manuals

1. Search local vendor examples, generated drivers, manuals, and existing project code before inventing a setup.
2. Use `rg` to find peripheral names, register names, pin names, example init functions, and error strings.
3. Prefer official examples for complex peripherals such as HRTIM, ADC injected/regular conversion, DAC DMA, CORDIC, FMAC, comparators, op-amps, USB, Ethernet, and FPGA IP.
4. When a local manual or example contradicts a guess, trust the artifact and revise the plan.

## Algorithm Workflow

1. If the user has an algorithm idea, implement a quick simulation first when feasible.
2. If the algorithm is unclear, ask or research what instrument/industry method solves the same measurement problem, then implement the smallest testable version.
3. For signal problems, validate with synthetic data before hardware data.
4. Compare algorithm output against known stimuli such as 1 kHz, 10 kHz, phase-aligned signals, DC offsets, and noise cases.
5. For frequency/phase/amplitude measurement, report expected raw counts or sample relationships, not only final formatted values.

## MCU And FPGA Strengths

Use AI aggressively for chip-internal hardware and firmware where examples/manuals exist:

1. STM32G4-class parts can use rich internal peripherals: high-speed ADC/DAC, HRTIM, comparators, op-amps, CORDIC, FMAC, DMA, timers, and synchronization fabric.
2. Prefer internal op-amp/comparator/follower paths when they simplify wiring and are supported by the chip/manual.
3. Use HRTIM/timers to synchronize ADC and DAC when phase accuracy matters.
4. Consider FMAC/CORDIC for real-time filtering or trigonometric operations when CPU load is a concern.
5. For FPGA projects, trace actual synthesized nets and generated netlists; names like `FREQ_MEASURE` do not prove the measured signal is correct.

## Hardware Design Boundary

Be more cautious with external analog circuit design than with firmware/peripheral configuration:

1. AI can help inspect schematics, derive constraints, and find likely circuit issues, but should not assume an analog circuit is valid from text alone.
2. For filters, bias networks, op-amp front ends, and RC networks, calculate corner frequency, input/output impedance, headroom, common-mode range, and loading.
3. Prefer simulating or calculating analog choices before recommending board changes.
4. Treat discrete analog fixes as hypotheses unless confirmed by schematic, datasheet, oscilloscope, or simulation.
5. When possible, choose chip-internal analog/peripheral features that are documented and easy to configure over fragile external add-ons.

## Practical Agent Rules

1. Do not stop at "possible causes" when the local project is available. Read files, add probes, build, and gather evidence.
2. Do not hand-edit blindly when the tool can generate or validate configuration.
3. Do not rely on final values only; expose raw registers, counters, flags, and timestamps.
4. Use scripts to make debugging reproducible and cheap.
5. State what was directly proven, what is inferred, and what measurement will close the remaining gap.
