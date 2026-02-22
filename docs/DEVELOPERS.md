# WinSSTV Developer Guide

WinSSTV is a high-performance SSTV decoding engine for Windows, built from the ground up utilizing the robust DSP logic patterns found in MMSSTV, rather than the older QSSTV pipelines.

## Core Technical Philosophy
The primary goal of WinSSTV is **Robust Decoding**. While many decoders rely on simple frequency estimation or zero-crossing, WinSSTV implements a parallel resonant filter architecture that excels in high-noise environments (QRM) and low-signal-to-noise ratios.

## Engine Architecture

### 1. Resonant Filter Pipeline (`src/core/dsp/`)
The heart of the decoder is a set of parallel 2-pole IIR Resonant Filters (often called "Tanks"). These were ported from MMSSTV's `CIIRTANK` implementation.

*   **Frequencies**: 1100Hz, 1200Hz, 1300Hz, and 1900Hz.
*   **Purpose**: These filters provide a real-time energy metric for specific SSTV tones, allowing the decoder to "hear" the signal through the static.

### 2. VIS Decoding (`visDecoder`)
Unlike standard decoders, WinSSTV does not estimate the frequency of the VIS bit. Instead, it performs a **Comparative Energy Analysis**.
*   Bit 1 (1100Hz) vs Bit 0 (1300Hz).
*   The bit is determined by which resonant filter reports higher energy at that specific sample index. This makes VIS auto-mode detection incredibly stable.

### 3. Sync Detection (`syncProcessor`)
WinSSTV employs a **Differential Sync Metric**.
*   A sync pulse (1200Hz) is only validated if its energy significantly exceeds the 1900Hz energy.
*   This prevents false triggers from white noise or high-frequency image content, maintaining a steady "lock" on the image slant.

## Environment & Build
The project is built for Windows using the **MSYS2 MinGW64** toolchain. 

*   **Framework**: Qt5 (Static build recommended)
*   **Compiler**: g++ (v15.2+)
*   **Build System**: `qmake` (WinSSTV.pro)

### Dependencies
- **FFTW3**: Used for Waterfall FFT rendering.
- **Hamlib**: Linked statically for rig control.
- **WASAPI**: Native Windows audio backend via Qt Multimedia.

## Project Layout
- `src/core/`: The math-heavy DSP engine.
- `src/gui/`: The Qt-based modern interface.
- `tools/`: Python-based test signal generators.
- `Portable/`: A bundled version of the app for zero-install distribution.

## Contributing
When adding new SSTV modes, ensure you utilize the `resonantFilter` energy arrays for consistency with the robust decoding chain. Any additions to the DSP layer should avoid absolute frequency measurements in favor of relative energy comparisons.
