# Zuolan STM32 + FPGA Signal Project Reference

Use this reference when the local project is `zuolan_signal_fpga_stm32_v4`, or when files mention the Zuolan dual-core signal-processing platform. The project documents used to build this reference are:

- Root overview Markdown in `I:\zuolan_signal_fpga_stm32_v4`.
- FPGA usage Markdown in `I:\zuolan_signal_fpga_stm32_v4\zuolan_FPGA`.
- STM32 usage Markdown in `I:\zuolan_signal_fpga_stm32_v4\zuolan_stm32`.
- Code annotation spreadsheet in `I:\zuolan_signal_fpga_stm32_v4`.

Some Markdown/source comments may appear mojibake in terminals. Treat module names, addresses, macros, QSF/IOC settings, and current source code as stronger evidence than prose comments or older documentation.

## Hardware And Toolchain Map

- MCU: STM32F429IGTx, Keil MDK project under `zuolan_stm32\MDK-ARM`, CubeMX config `zuolan_stm32\zuolan_STM32.ioc`.
- FPGA: EP4CE10F17, Quartus project `zuolan_FPGA\prj\ZUOLAN_FPGA_OBJECT.qpf`, settings `ZUOLAN_FPGA_OBJECT.qsf`, top design `TOP.bdf`.
- Main bridge: STM32 FMC 16-bit async bus to FPGA. STM32 acts as bus master, FPGA as slave.
- Signal devices: AD9959 DDS, optional AD9833 outputs, dual AD9226-style ADC inputs, AD9764-style DAC outputs.
- Nominal clocks from project docs: 25 MHz board oscillator, FPGA PLL-derived clocks, DA max clock around 125 MHz in STM32 DA driver.
- Debug tools: Keil/ST-Link for STM32, Quartus/SignalTap `zuolan_FPGA\stp\stp1.stp` for FPGA, oscilloscope for DA/ADC verification, UART1 at 115200 8N1 for logs.

## Directory Map

FPGA:

- `zuolan_FPGA\src\FMC_CONTROL.v`: FMC bus latch, 16 write channels from STM32 to FPGA and 16 read channels from FPGA to STM32.
- `zuolan_FPGA\src\MASTER_CTRL.v`: control-word register at address `0x0001`; current implementation intentionally/inadvertently infers a latch.
- `zuolan_FPGA\src\DA_PARAMETER_DEAL.v`: DA parameter address decode for frequency, amplitude, waveform, and step/phase-like values.
- `zuolan_FPGA\src\DA_WAVEFORM.v`: current generic waveform LUT mux. Older docs may call this `DA_WAVEFORM_A/B.v`.
- `zuolan_FPGA\src\AD_DATA_DEAL.v`: maps ADC FIFO data/status to FMC read channels and reverses the 12-bit ADC bit order before returning data.
- `zuolan_FPGA\src\AD_FREQ_MEASURE.v`: splits 32-bit frequency/base counters into high/low 16-bit bus reads.
- `zuolan_FPGA\src\AD_FREQ_WORD.v`, `FREQ_DEV.v`, `CNT32.v`, `ADC_ZERO_CROSS.v`, `VOLTAGE_SCALER_CLOCKED.v`: frequency word, divider/counter, zero-crossing, and scaling support logic.
- `zuolan_FPGA\ip\`: PLL, waveform ROMs (`sinromvpp`, `sqaure_rom`, `triangle_rom`, `sawtooth_rom`) and FIFO IP.

STM32:

- `zuolan_stm32\Core\Src` and `Core\Inc`: CubeMX/HAL startup, GPIO, FMC, USART, DAC.
- `zuolan_stm32\MY_APP\scheduler.c/h`: cooperative foreground scheduler. Current task table enables `key_proc` every 10 ms; `ad_proc` and frequency/modulation tasks may be commented out.
- `zuolan_stm32\MY_APP\bsp_system.h`: central include hub and shared globals.
- `zuolan_stm32\MY_Hardware_Drivers\Src\da_output.c`: two-channel DA driver using a software shadow struct; changes must call `DA_Apply_Settings()` to reach FPGA registers.
- `zuolan_stm32\MY_Hardware_Drivers\Src\ad_measure.c`: ADC sampling setup, FIFO read, voltage conversion, and sample-based frequency estimation.
- `zuolan_stm32\MY_Hardware_Drivers\Src\freq_measure.c`: frequency measurement driver.
- `zuolan_stm32\MY_Hardware_Drivers\Src\AD9959.c`, `AD9833.c`: DDS drivers.
- `zuolan_stm32\MY_Hardware_Drivers\Src\key_app.c`: current button callbacks. Current code maps key1/key2 to ASK/FSK start-stop, key3 to DA1 phase step, key4 to FFT/spectrum output; older docs describe a waveform-data-output flow, so check source before changing behavior.
- `zuolan_stm32\MY_Algorithms\Src`: FFT, filters, Kalman, phase measurement, spectrum analyzer.
- `zuolan_stm32\MY_Communication\Src`: USART, packet, HMI communication.
- `zuolan_stm32\MY_Utilities\Src`: command dispatch and shared initialization.

## FMC Bus Semantics

FPGA `FMC_CONTROL.v` exposes a 16-bit address/data bus:

- External signals: `fpga_nl_nadv` address-valid/address-latch, `fpga_cs_ne1` chip select, `fpga_wr_nwe` write enable, `fpga_rd_noe` output/read enable, `fpga_db[15:0]` bidirectional bus.
- Address phase: STM32 asserts `fpga_nl_nadv` low with chip select active and drives address on `fpga_db`; FPGA latches `addr`.
- Write phase: chip select active, `fpga_wr_nwe` low, `fpga_nl_nadv` high. FPGA stores `fpga_db` into `read_data_N__reg` selected by address `0x0000..0x000F`.
- Read phase: chip select active, `fpga_rd_noe` low, `fpga_nl_nadv` high. FPGA drives `fpga_db` from `write_data_N_` selected by address `0x0000..0x000F`.
- `FMC_CONTROL.v` currently uses `assign addr = condition ? fpga_db : addr;`, which infers a latch. If chasing sporadic bus issues, verify address setup/hold timing, latch behavior, and whether `fpga_db` is stable during `fpga_nl_nadv`.

## FMC Register Map

STM32 -> FPGA write channels:

| Addr | Purpose |
| --- | --- |
| `0x0000` | Reserved/default |
| `0x0001` | Master control word, decoded by `MASTER_CTRL.v` |
| `0x0002` | DA1 frequency control word high 16 bits |
| `0x0003` | DA1 frequency control word low 16 bits |
| `0x0004` | DA2 frequency control word high 16 bits |
| `0x0005` | DA2 frequency control word low 16 bits |
| `0x0006` | AD1 sampling frequency control word high 16 bits |
| `0x0007` | AD1 sampling frequency control word low 16 bits |
| `0x0008` | AD2 sampling frequency control word high 16 bits |
| `0x0009` | AD2 sampling frequency control word low 16 bits |
| `0x000A` | DA1 phase, mapped from degrees to 0..1023 style phase address |
| `0x000B` | DA2 phase, mapped from degrees to 0..1023 style phase address |
| `0x000C` | DA waveform selection, high byte = DA1, low byte = DA2 |
| `0x000D` | DA step/combined control in current `DA_PARAMETER_DEAL.v` |
| `0x000E` | DA1 amplitude, low 12 bits used |
| `0x000F` | DA2 amplitude, low 12 bits used |

FPGA -> STM32 read channels:

| Addr | Purpose |
| --- | --- |
| `0x0002` | AD1/base1 frequency measurement high 16 bits, depending on top-level wiring |
| `0x0003` | AD1/base1 frequency measurement low 16 bits |
| `0x0004` | AD2/base2 frequency measurement high 16 bits |
| `0x0005` | AD2/base2 frequency measurement low 16 bits |
| `0x0006` | AD1 FIFO data, 12-bit ADC data bit-reversed then zero-extended to 16 bits |
| `0x0007` | AD1 FIFO flag/status; current source returns `0x0001` when `AD1_FLAG` is true |
| `0x0008` | AD2 FIFO data, 12-bit ADC data bit-reversed then zero-extended to 16 bits |
| `0x0009` | AD2 FIFO flag/status; current source returns `0x0001` when `AD2_FLAG` is true |
| `0x000A` | AD1 measured frequency high 16 bits |
| `0x000B` | AD1 measured frequency low 16 bits |
| `0x000C` | AD2 measured frequency high 16 bits |
| `0x000D` | AD2 measured frequency low 16 bits |

The spreadsheet address-map sheet describes addresses `0..15`; the FPGA source uses 16-bit addresses like `16'h000A`. Keep the width distinction clear when comparing STM32 macros with Verilog.

## Master Control Bits

Excel sheet `CTRLDATA` defines the intended bit meanings of the control word at write address `0x0001`:

| Bit | Meaning |
| --- | --- |
| 0 | DA frequency synthesis control |
| 2 | AD1 sampling-clock generation control |
| 3 | AD2 sampling-clock generation control |
| 4 | AD1 FIFO write enable |
| 5 | AD1 FIFO read enable |
| 6 | AD2 FIFO write enable |
| 7 | AD2 FIFO read enable |
| 8 | AD1 frequency counter reset; low level clears |
| 9 | AD1 frequency counter start |
| 10 | AD2 frequency counter reset; low level clears |
| 11 | AD2 frequency counter start |

Bits 1 and 12..14 are blank in the source spreadsheet. Before changing control behavior, trace `CTRL_DATA` consumers in `TOP.bdf`/generated netlists/source.

## DA Output Notes

- STM32 API: `DA_Init()`, `DA_SetConfig()`, `DA_SetFREQ()`, `DA_SetAmp()`, `DA_SetPhase()`, `DA_SetWaveform()`, `DA_Apply_Settings()`, `DA1_OUT()`, `DA2_OUT()`.
- `DA_Set*` calls only update `da_channels[]`; no hardware effect occurs until `DA_Apply_Settings()`.
- `DA_Apply_Settings()` stops FPGA DA generation, writes DA1/DA2 frequency high/low, amplitude, phase, packed waveform selection, then restarts generation.
- Waveform enum in `da_output.h`: `WAVE_SINE=0`, `WAVE_SQUARE=1`, `WAVE_TRIANGLE=2`, `WAVE_SAWTOOTH=3`.
- FPGA `DA_PARAMETER_DEAL.v` expects waveform at `0x000C`: high 8 bits DA1, low 8 bits DA2. Current STM32 code writes `DA_WAVEFORM = (da_channels[0].waveform << 8) | da_channels[1].waveform`.
- DA frequency formula in STM32 current code: `M_DA = DA_FREQ_CONSTANT * frequency / FPGA_BASE_CLK * DA_ROM_SIZE`; verify integer promotion/order if output frequency is wrong.
- FPGA waveform ROM address width is 10 bits / 1024-point lookup in the docs. DA data output is 14-bit.

## ADC Sampling And Frequency Notes

- STM32 `ad_measure.h` defines `AD_SIGNAL_SAMPLE_MIN_HZ=1000000`, `AD_SIGNAL_SAMPLE_MAX_HZ=48000000`, and `AD_SIGNAL_SAMPLE_MULTIPLE=20.0f`.
- `setSamplingFrequency(freq, channel, mode)` accepts `FREQ_MODE_SIGNAL` or `FREQ_MODE_SAMPLING`; signal mode multiplies by `AD_SIGNAL_SAMPLE_MULTIPLE` then clamps to min/max.
- ADC sampling control words are split high/low at write addresses `0x0006/0x0007` for AD1 and `0x0008/0x0009` for AD2.
- `AD_DATA_DEAL.v` reverses ADC data bit order before returning it. If measured voltage looks mirrored, stair-stepped, or bit-scrambled, verify ADC board bit wiring against this reversal.
- FIFO flags in current FPGA source are non-empty flags, not full/empty bitfields, unless changed elsewhere in top-level glue.
- `updateAdcFrequencyResults()` estimates frequency from sampled data using hysteresis and rising-crossing interpolation; this is separate from FPGA counter-based frequency reads.

## UART, Scheduler, And Button Behavior

- USART docs and code use 115200 8N1, commonly UART1 for debug output.
- `scheduler_run()` must be called in `while(1)` after `scheduler_init()`.
- Current `scheduler_task[]` enables `{key_proc, 10, 0}`. If ADC or modulation tasks appear inactive, check whether `{ad_proc, 1, 0}`, `{freq_proc, 1000, 0}`, or `{AD9959_Modulation_State_Update, 5, 0}` are commented out.
- Button scan treats pressed as GPIO reset/low. It edge-detects newly pressed keys with `key_map_down`.
- Current key callbacks:
  - Key1: toggle AD9959 ASK transmission.
  - Key2: toggle AD9959 FSK transmission.
  - Key3: increment DA1 phase by 30 degrees, apply settings, print phase.
  - Key4: run spectrum analyzer/FFT on `fifo_data1_f` and send UART/FireWater output.

## Pin And Connector Notes From Excel

STM32 FMC pins called out in the code annotation spreadsheet and `.ioc`:

- `PB7`: `FMC_NL`
- `PG9`: `FMC_NE2`
- `PD4`: `FMC_NOE`
- `PD5`: `FMC_NWE`
- `PD14/PD15/PD0/PD1`: `FMC_DA0/DA1/DA2/DA3`
- `PE7..PE15`: `FMC_DA4..DA12`
- `PD8/PD9/PD10`: `FMC_DA13/DA14/DA15`

AD9959 STM32 pins:

- `PH13 DDS_CS`, `PC4 DDS_P0` in current `main.h`/IOC, docs may list `PA4`; verify against current `Core\Inc\main.h` and board wiring.
- `PH9 DDS_P1`, `PG6 DDS_P2`, `PI0 DDS_P3`
- `PH11 DDS_PDC`, `PH14 DDS_RST`, `PC7 DDS_SCK`
- `PH12 DDS_SDIO0`, `PH15 DDS_SDIO1`, `PG11 DDS_SDIO2`, `PI5 DDS_SDIO3`, `PH10 DDS_UP`

AD9833 STM32 pins from Excel:

- `PF9 AD9833_FSYNC1`, `PC6 AD9833_FSYNC2`
- `PF8 AD9833_SCLK1`, `PC2 AD9833_SCLK2`
- `PF6 AD9833_SDATA1`, `PC1 AD9833_SDATA2`

FPGA DA connector pins from Excel:

- DA_A: `CLKA=C8`, `A13=A8`, `A12=F8`, `A11=B8`, `A10=F9`, `A9=A9`, `A8=E9`, `A7=B9`, `A6=D9`, `A5=A10`, `A4=C9`, `A3=B10`, `A2=E10`, `A1=A11`, `A0=D11`.
- DA_B: `CLKB=B11`, `B13=C11`, `B12=A12`, `B11=E11`, `B10=B12`, `B9=D12`, `B8=A13`, `B7=F11`, `B6=B13`, `B5=C14`, `B4=A14`, `B3=D14`, `B2=B14`, `B1=F10`, `B0=A15`.

FPGA AD connector pins from Excel:

- AD1: `ACLK=C15`, `A11=G15`, `A10=F13`, `A9=J15`, `A8=F14`, `A7=J16`, `A6=G11`, `A5=K16`, `A4=J11`, `A3=L16`, `A2=K11`, `A1=L15`, `A0=J12`, `ATR=N16` (overflow not used).
- AD2: `BCLK=J13`, `B11=N15`, `B10=J14`, `B9=P16`, `B8=K12`, `B7=P15`, `B6=K15`, `B5=R16`, `B4=L12`, `B3=L14`, `B2=L13`, `B1=M12`, `B0=N14`, `BTR=N13`.

## Debugging Checklist For This Project

1. Identify which side owns the symptom: STM32 API/shadow state, FMC bus transaction, FPGA register decode, FPGA signal logic, or analog I/O.
2. For FMC issues, read both STM32 macros/pointers and FPGA address cases. Confirm the same address, width, byte lane behavior, and high/low word ordering.
3. For DA issues, confirm `DA_Apply_Settings()` ran after setters, then probe FMC writes to `0x0002..0x000F`, then scope DA clock/data/output.
4. For ADC issues, verify sampling control writes, FIFO enables in `CTRL_DATA`, FIFO flag semantics, bit reversal in `AD_DATA_DEAL.v`, and voltage conversion constants in `ad_measure.c`.
5. For frequency issues, distinguish FPGA counter reads (`AD_FREQ_MEASURE.v`) from STM32 sample-estimated frequency (`updateAdcFrequencyResults()`).
6. For button/UART workflows, trust `key_app.c` current callbacks over older docs. Capture UART1 logs at 115200 and correlate each button press with code path.
7. For clocks, trace 25 MHz oscillator to FPGA PLL and STM32 `SystemClock_Config()`, then compare with constants like `FPGA_BASE_CLK`, `DA_MAX_CLK`, and frequency-word formulas.
