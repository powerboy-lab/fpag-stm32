# Local Toolchain Paths

Use these known local paths before searching `PATH`:

## STM32 / Keil MDK

- Keil uVision: `D:\Keil_v5\UV4\UV4.exe`
- Typical build command:

```powershell
& 'D:\Keil_v5\UV4\UV4.exe' -b '<project>.uvprojx' -j0 -o '<build-log>.log'
```

Use this for non-interactive compile checks. Do not flash unless the user has requested board programming or hardware testing.

## STM32CubeMX

- STM32CubeMX GUI: `D:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX\STM32CubeMX.exe`

Use this for `.ioc` inspection/regeneration only when needed. Because it opens a GUI and may rewrite generated files, ask before launching or regenerating.

## Intel Quartus Lite

- Quartus GUI: `C:\intelFPGA_lite\18.1\quartus\bin64\quartus.exe`
- Check the same directory for command-line tools such as `quartus_sh.exe`, `quartus_map.exe`, `quartus_fit.exe`, `quartus_asm.exe`, and `quartus_sta.exe`.

Prefer command-line Quartus tools for build checks. Use the GUI only when a BDF/IP edit requires it and the user agrees.

## Python

- Python environment: `D:\minicoda3\envs\newenvs\env\python.exe`

Use this for serial capture scripts, log analysis, file conversion, and deterministic project tooling.

## GitHub / Network Proxy

- Local proxy for Git HTTP/HTTPS: `http://127.0.0.1:7892`

If GitHub works in Chrome but `git clone`, `git pull`, or `git push` is slow or times out, verify:

```powershell
git config --global --get-regexp "^(http|https)\.proxy$"
```

Expected values:

```text
http.proxy http://127.0.0.1:7892
https.proxy http://127.0.0.1:7892
```

Use `git ls-remote <repo-url> HEAD` as a quick connectivity test before long clone or push operations.
