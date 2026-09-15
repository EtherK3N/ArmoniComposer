# Armoni Composer — Release v0.1.0 (September 6, 2026)

**Public Preview & Open Hardware Orchestration Release**  
Author: **[@EtherK3N](https://github.com/EtherK3N)** (K3N Solver)  
License: **GNU Affero General Public License v3.0 (AGPL-3.0)**

---

## 🚀 Overview

**K3N Armoni Composer** is an open-source, low-latency live loop station and audio workstation built on native C++17, JUCE, and the Windows Raw Input API (`WM_INPUT`).

It solves a fundamental limitation of consumer operating systems: Windows normally merges keystrokes from all connected USB keyboards into a single generic input stream. By intercepting raw device handles at the hardware driver level, K3N Armoni Composer identifies up to **16 distinct USB keyboards concurrently**, turning ordinary €2 membrane computer keyboards into an orchestra of independent musical controllers:
- **Keyboard #1**: Electronic Drum Kit & Percussion Pad
- **Keyboard #2**: 808 Sub-Bass & Moog Lead
- **Keyboard #3**: Polyphonic Chords & Ambient Pads
- **Keyboard #4**: One-Shot FX, Live Stems & Vocal Drops

---

## ⚡ What is Included in v0.1.0

### 1. Low-Level Input Subsystem (`Source/RawInputHandler.*`, `Source/DeviceManager.*`)
- Direct Win32 `WM_INPUT` hook with `RIDEV_INPUTSINK` (captures input even when backgrounded).
- Hardware path disambiguation via `GetRawInputDeviceInfoW(RIDI_DEVICENAME)`.
- Per-keyboard role assignment (Drums, Bass, FX, Melody) with zero-cloud `%APPDATA%` local JSON persistence.

### 2. High-Performance Audio Engine (`Source/AudioEngine.*`)
- **32-voice polyphonic sampler** with real-time voice-stealing.
- Zero heap allocations and zero disk I/O in the high-priority audio thread (`getNextAudioBlock`).
- Built-in procedural synthesis engine: punchy 808 Kick, Snare, Closed/Open Hats, Sub-Bass (A1), and Lead Stab (440Hz).
- Pre-decoded in-memory RAM sample cache (WAV, MP3, FLAC, AIFF, OGG).

### 3. Precision Timing & Quantization (`Source/MetronomeClock.*`, `Source/BpmQuantizer.*`)
- **`MetronomeClock`**: Sample-accurate variable tempo engine (40–300 BPM), multiple time signatures (4/4, 3/4, 6/8, odd meters), and procedural woodblock click generator (1400Hz downbeat / 900Hz subdivisions).
- **`BpmQuantizer`**: Snap-to-grid engine with resolutions from 1/4 to 1/32 triplets, adjustable humanize/swing blend, and automatic loop length snapping to bar boundaries (`snapLoopLengthToNearestBar`).

### 4. Interactive Zero-Install Web Simulator (`preview/index.html`)
- Immediate in-browser demonstration of the entire workstation workflow via Web Audio API.
- Live 3-track overdub looper with real-time waveform playheads.
- Modular per-layer FX chains (individual Delay, Algorithmic Reverb, and Low-Pass Filter with editable parameters).
- Custom audio stem drag-and-drop loader (MP3, WAV, FLAC, OGG).
- 30+ synthesized instrument vault entries and live WebM audio export.

### 5. Robust Verification & CI
- 5 comprehensive automated test suites (`Tests/TestRunner.cpp`).
- Cross-platform Docker build & test harness (`Dockerfile`, `DOCKER_BUILD_AND_TEST.bat`).
- Automated GitHub Actions build pipeline (`.github/workflows/build.yml`).

---

## 📦 How to Get Started

- **Instant Browser Simulator**: Double-click `AVVIA_SIMULATORE.bat` or open `preview/index.html` in Chrome/Edge/Firefox.
- **Native C++ Build**: Follow instructions in [README.md](README.md) and [TESTING.md](TESTING.md).

---

## 🤝 Community & Next Steps

This milestone lays the technical foundation for Phase 2 (modifier shift layers, Ableton Link synchronization, and native VST3/CLAP wrappers). Feedback, bug reports, and contributions are welcomed via [GitHub Issues](https://github.com/EtherK3N/ArmoniComposer/issues).
