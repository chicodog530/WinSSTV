# WinSSTV Developer Guide

WinSSTV's core builds upon the heavily established QSSTV DSP pipeline, but with significant Windows-specific modifications utilizing the MSYS2 MinGW64 toolchain and Qt5.

## Environment Architecture
To build WinSSTV from source, you cannot utilize MSVC. The project relies extensively on POSIX-like capabilities supplied via `mingw64`.
*   **Platform**: MSYS2 (mingw64 terminal)
*   **Compiler**: `g++` (v15.2 or equivalent) via MinGW64
*   **Framework**: Qt5 (Strictly statically compiled, specifically `/qt5-static/` branch)
*   **Core Dependencies**: FFTW3, Hamlib (Compiled statically), DirectX9/DXVA2/MFplat multimedia tools inside mingw-w64.

## Getting the Build Alive
1. Fire up the `MSYS2 MinGW 64-bit` shell.
2. Verify you have the proper tools installed: `pacman -S mingw-w64-x86_64-qt5 mingw-w64-x86_64-fftw mingw-w64-x86_64-hamlib`.
3. Navigate to the `WinSSTV` root source directory.
4. Execute `qmake` (specifically, path it to your `qt5-static/bin/qmake.exe` installation if overriding the global PATH).
5. Run `mingw32-make release`.

## Technical Layout
- **The Core Engine** (`src/core/`): Heavily isolated mathematically from the front-end Qt UI threads. Contains the 12kHz/48kHz filter cascades spanning `syncProcessor` (for mode locking), `sstvrx` (the actual processing loop), and `visfskid` (reading the raw FSK data bits).
- **Audio Overhaul** (`src/core/sound/`): The old POSIX ALSA dependencies are eradicated. Audio is funneled through native `soundwindows.cpp` WASAPI hooks, keeping latencies absolutely locked via Qt Audio abstractions downsampled mathematically.
- **Deep Decode Mechanism** (`src/gui/mainwindow.cpp`): Instead of capturing a real microphone, `processDeepDecode()` completely circumvents the WASAPI classes, pipes an active 12kHz raw `.wav` payload stream through `wavIO`, artificially pumps its amplitude via multiplier matrices, forces the Core Engine `sstvRx` internal clock to mimic a real 48kHz session, and blitzes the buffer sequence synchronous to a customized UI rendering timer.

> [!WARNING]
> Due to the tight linkage to `libhamlib.a` being a notoriously difficult dependency on Windows, `qsstv.pro` specifies direct object linking `-lhamlib` dynamically inside Mingw64 `/lib/` prior to static compression. If you face linkage crashes on a fresh Mingw build, check `LIBS+=` in `WinSSTV.pro`.
