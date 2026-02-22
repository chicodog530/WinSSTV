# WinSSTV

WinSSTV is a modern, native Windows application built on the Qt5 framework designed for transmitting and receiving Slow Scan Television (SSTV) images. Originally adapted from the robust core DSP engine of Linux's prominent QSSTV software, WinSSTV brings powerful audio decoding, clean modern interfaces, and easy-to-use workflows natively to the Windows ecosystem.

## Core Features
- **High-Fidelity Audio DSP**: Built on a solid, battle-tested FSK and VIS audio processing pipeline capable of incredibly noisy signal recovery.
- **Deep Decode Engine**: Skip the radio. Process pre-recorded `.wav` files natively and watch them render to the screen at hyperspeed with real-time UI rendering and progress tracking. 
- **Eager Mode Fallback**: A completely revamped RX Mode logic allows listeners to pick a preferred mode (like Martin 1) that will completely skip the need for VIS transmission headers and lock onto the raw acoustic sync pulses, decoding signals that other platforms miss. Absolute VIS detection guarantees that if a completely *different* mode transmits over you, the application will instantly snap over to the newly detected VIS mode automatically.
- **Image Transmit Pipeline**: Native font rendering, image generation capabilities, image overlays, resize handling, and precision 48kHz audio synthesis.
- **Modern Interface**: Designed for rapid "Waterfall -> Image -> Gallery" workflow via the updated Qt UI.

## Getting Started

Check out the included `instructions.txt` for a quick breakdown on how to connect your audio interface, test generic transmit loopbacks, and load in `.wav` files for deep decoding.

For build and environment setup information, take a look at `DEVELOPERS.md`!

### Credits
WinSSTV was heavily adapted and built upon the core DSP and filtering implementations originally authored for Linux by the creators of QSSTV. Support for these features was rewritten exclusively for the Windows runtime.
