# Armoni Composer
### by EtherK3N (K3N Solver)

<div align="center">

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/JUCE-7%20%2F%208-FF6600?style=for-the-badge&logoColor=white)](https://juce.com/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://microsoft.com/windows)
[![License](https://img.shields.io/badge/License-AGPL%20v3.0-blue?style=for-the-badge)](LICENSE)
[![Status](https://img.shields.io/badge/Status-v0.1.0%20Preview-a855f7?style=for-the-badge)](ROADMAP.md)

**Turn any cheap USB keyboard into a live loop station, drum machine, and synth rig.**  
No expensive hardware. No subscriptions. No cloud. Just your creativity.

[The Idea](#-the-idea) · [Live Demo](#-try-the-interactive-demo-now) · [How It Works](#-how-it-works) · [Build from Source](#-build-from-source) · [Roadmap](#-roadmap) · [Contribute](#-contribute) · [Author](#-author)

</div>

---

## The Idea

Dedicated drum pads, MIDI controllers, and loop stations cost €80–€500.  
A USB membrane keyboard at a flea market costs €2.

**Armoni Composer** (by EtherK3N / K3N Solver) makes every cheap keyboard a performance instrument. Plug in one keyboard or four — the system tracks each device independently at the OS kernel level via Win32 Raw Input, routes each to its own instrument role (Drums / Bass / Synth / FX), and lets you build layered live loops in real time, just like a Boss RC-505 but with zero specialized hardware.

> Built for bedroom producers, street performers, students in underfunded schools, and anyone who believes music should not cost money to make.

---

## Try the Interactive Demo Now

No build required. Open directly in your browser:

**[`preview/index.html`](preview/index.html)** — double-click or open with any browser.

The simulator includes:
- Full visual keyboard rig with color-coded instrument zones
- Audible synthesized metronome (40–240 BPM, adjustable)
- 3-track live overdub loop station (Record → Play → Overdub)
- Per-layer modular FX routing (Delay, Reverb, Low-Pass Filter)
- Custom audio file loader (MP3, WAV, FLAC, OGG) — load any stem or track onto a key and remix it live
- Multi-deck switcher to simulate 1–3 separate physical keyboards
- Realtime canvas oscilloscope

---

## How It Works

```
USB Keyboard #1 (€3) ─┐
USB Keyboard #2 (€4) ─┤──► Win32 Raw Input WM_INPUT
USB Keyboard #3 (€2) ─┘         │
                                  ▼
                        RawInputHandler
                        (device path disambiguation)
                                  │
                    ┌─────────────┴──────────────┐
                    ▼                            ▼
            DeviceManager                 MappingEngine
          (role assignment)            (per-key sound routing)
                    │
                    ▼
            AudioEngine (C++ / JUCE)
            32-voice polyphonic sampler
            ASIO / WASAPI / DirectSound
                    │
                    ▼
            LoopStation (multi-track overdub)
            sample-accurate recording & playback
```

The critical technical challenge this solves: Windows normally merges all USB keyboard input into a single global key event stream. This app intercepts input **before** the OS merges it, at the Raw Input device level, allowing true per-physical-keyboard instrument assignment even with 4+ keyboards connected simultaneously.

---

## Key Features

| Feature | Detail |
|---|---|
| Multi-keyboard disambiguation | Up to 16 concurrent USB keyboards via Win32 Raw Input (`WM_INPUT`), each mapped to an independent instrument role |
| Event-based looper (`LoopTrack`) | Sample-accurate timestamped event recording with sorted timeline cursor scheduling ($O(1)$ block processing) |
| 32-voice polyphonic sampler | Pre-allocated voice pool with seamless voice-stealing on capacity |
| Built-in procedural starter kit | Synthesized 808 Kick, Snare, Hi-Hat, Sub-Bass, Acid 303, Moog Saw, Rhodes for immediate playability |
| Audio sample import | Lossless decoding of WAV, MP3, FLAC, OGG into pre-allocated memory buffers |
| Modular FX architecture | Delay / Reverb / Low-pass filter routing per loop layer (implemented in preview simulator; native C++ DSP planned for v0.3.0) |
| Audio backend drivers | ASIO / WASAPI / DirectSound device management via JUCE |
| 100% offline | Zero cloud, zero telemetry, zero accounts |
| Prototype latency target | Sub-10ms target on low-buffer ASIO/WASAPI exclusive configurations |

---

## Build from Source

> **Note — v0.1.0 Preview**: The native C++ executable is currently being finalized. The interactive browser simulator in `preview/index.html` is fully functional and demonstrates the complete UX. Native build instructions are tracked in [ROADMAP.md](ROADMAP.md).

### Prerequisites

- Windows 10 / 11 (64-bit)
- CMake 3.22+
- Visual Studio 2019/2022 with C++ workload, or Clang/LLVM
- JUCE 7 or 8 (fetched automatically as a submodule)

```bash
# Clone with submodules
git clone --recursive https://github.com/EtherK3N/ArmoniComposer.git
cd ArmoniComposer

# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

Binary output: `build/ArmoniComposer_artefacts/Release/ArmoniComposer.exe`

For Docker-based isolated testing:

```bat
DOCKER_BUILD_AND_TEST.bat
```

See [TESTING.md](TESTING.md) for the full test suite guide.

---

## Roadmap

This is a community-driven project. The core architecture is in place; the roadmap defines what we build next together.

| Phase | Version | Focus | Status |
|---|---|---|---|
| Core Engine & Hardware Layer | v0.1.0 | Win32 Raw Input, sampler, loop station, procedural synth | Preview |
| Metronome & BPM Quantize | v0.2.0 | Sample-accurate clock, snap-to-grid, swing/humanize | Planned |
| DSP FX Engine | v0.3.0 | Per-track EQ, resonant filter, algorithmic reverb | Planned |
| Studio Integration | v0.4.0 | Multitrack WAV export, MIDI output, Ableton Link sync | Planned |
| Cross-Platform | v0.5.0+ | macOS IOKit, Linux libevdev/ALSA/JACK, VST3/CLAP plugin | Planned |

Full detail: [ROADMAP.md](ROADMAP.md)

---

## Contribute

All skill levels welcome — musicians, audio engineers, C++ developers, UI designers, documentation writers.

- Browse open issues for `good first issue` tags
- Read [CONTRIBUTING.md](CONTRIBUTING.md) for PR guidelines
- Open a Discussion if you have a feature idea, use case story, or want to test with unusual hardware

If you use this tool, make something with it, or just find the concept interesting — **a star on the repo helps other people find it.** That is how open source grows.

---

## Author

Built by **[@EtherK3N](https://github.com/EtherK3N)** (K3N Solver) — part of the **K3N** creative and engineering brand.

High-performance C++, real-time audio systems, accessible technology, modern web and application development.

Open for collaboration, feedback, and contributions via GitHub Issues and Discussions.

---

## Security & Compliance

We adhere to modern cybersecurity resilience standards (including the EU Cyber Resilience Act / NIS2). For our complete Software Bill of Materials (SBOM), threat model, and vulnerability reporting process, see **[SECURITY.md](SECURITY.md)**.

---

## License

This project is licensed under the terms of the [GNU Affero General Public License v3.0 (AGPL-3.0)](LICENSE).  
Commercial / closed-source licensing inquiries can be directed to the author.
