# Armoni Composer
### by EtherK3N (K3N Solver)

<div align="center">

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/JUCE-7%20%2F%208-FF6600?style=for-the-badge&logoColor=white)](https://juce.com/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://microsoft.com/windows)
[![License](https://img.shields.io/badge/License-AGPL%20v3.0-blue?style=for-the-badge)](LICENSE)
[![Status](https://img.shields.io/badge/Status-v0.4.0%20Studio-10b981?style=for-the-badge)](RELEASE_NOTES_v0.4.0.md)
[![Live Demo](https://img.shields.io/badge/Live_Demo-Try_in_Browser-9333ea?style=for-the-badge&logo=googlechrome&logoColor=white)](https://etherk3n.github.io/ArmoniComposer/)

**Turn any cheap USB keyboard into a live loop station, drum machine, and virtual analog synth rig.**  
No expensive hardware. No subscriptions. No cloud. Just raw musical performance.

[The Idea](#-the-idea) · [Live Browser Demo](#-try-the-interactive-demo-now) · [How It Works](#-how-it-works) · [Features](#-key-features) · [Build from Source](#-build-from-source) · [Roadmap](#-roadmap) · [Contribute](#-contribute) · [Author](#-author)

</div>

---

## 💡 The Idea

Dedicated drum pads, MIDI controllers, and hardware loop stations cost €80–€500.  
A discarded USB membrane keyboard at a flea market costs €2.

**Armoni Composer** makes every computer keyboard an expressive musical instrument. Plug in one keyboard or four — the system tracks each physical device independently at the OS kernel level via Win32 Raw Input, routes each to its own instrument role (Drums / Bass / Synth / FX), and lets you build layered live loops in real time, just like a Boss RC-505 or Akai MPC, with zero specialized hardware.

> Built for bedroom producers, street performers, students in underfunded schools, and anyone who believes music production should not cost a fortune.

---

## 🌐 Try the Interactive Demo Now

**Zero install required.** Play the full DAW simulation right now in your browser:

👉 **[Launch Armoni Composer Live on GitHub Pages](https://etherk3n.github.io/ArmoniComposer/)**

The interactive workstation includes:
- **Visual Keyboard Rig**: Color-coded instrument zones for 1–4 connected keyboards.
- **Audible Synthesized Metronome**: 40–240 BPM, odd meters, and visual LED.
- **3-Track Live Overdub Looper**: Record → Play → Overdub with real-time waveform playheads.
- **Modular FX Chains**: Per-layer Delay, Algorithmic Reverb, and Low-Pass Filter.
- **Parameter Locks & Hit Inspector**: Nudge micro-timing, gate tails, and pitch transposition.
- **Custom Sample Import**: Load MP3, WAV, FLAC, OGG files directly onto any key.
- **Realtime Canvas Oscilloscope & WebM Audio Export**.

---

## ⚙️ How It Works

```
USB Keyboard #1 (€2) ─┐
USB Keyboard #2 (€3) ─┤──► Win32 Raw Input WM_INPUT (Device Fingerprinting VID/PID)
USB Keyboard #3 (€2) ─┘         │
                                  ▼
                        RawInputHandler
                   (per-keyboard hardware routing)
                                  │
         ┌────────────────────────┼────────────────────────┐
         ▼                        ▼                        ▼
  DeviceManager          VelocityEmulationEngine    ShiftLayerSystem
 (role assignment)       (strike cadence & dwell)   (Drums/Bass/Synth)
         │                        │                        │
         └────────────────────────┼────────────────────────┘
                                  ▼
                       AudioEngine (C++17 / JUCE)
          ┌───────────────────────┴───────────────────────┐
          ▼                                               ▼
  AnalogSynthEngine                               32-Voice Sampler
  PolyBLEP Oscillators                            Pre-allocated RAM Pool
  4-pole Moog Ladder Filter                       Procedural 808 Kit
          │                                               │
          └───────────────────────┬───────────────────────┘
                                  ▼
                             DspFxRack
               3-Band RBJ Parametric EQ (Biquad DF-II)
               Zero-Delay State Variable Filter (SVF)
               Tempo-Synced Stereo Ping-Pong Delay
               Schroeder Algorithmic Reverb Core
                                  │
                                  ▼
                             LoopStation
                Sample-Accurate Overdub Multi-Track
                Sorted Timeline Cursor Scheduling O(1)
                                  │
         ┌────────────────────────┴────────────────────────┐
         ▼                                                 ▼
   StemExporter                                       MidiExporter
  24-bit Multitrack WAV                             Type 1 Standard MIDI
  Peak-Normalized Mixdown                           24 PPQN Realtime Sync
```

The critical engineering challenge this solves: Windows merges all USB keyboard inputs into a single global key event stream. Armoni Composer intercepts input **before** OS merging, capturing raw hardware handles and persistent `VID:PID:MI` fingerprints so your devices retain their musical roles across USB ports.

---

## ⚡ Key Features

| Subsystem | Architectural Implementation | Production Detail |
|---|---|---|
| **Multi-Keyboard Disambiguation** | Win32 Raw Input (`WM_INPUT`) | Up to 16 concurrent USB keyboards with persistent `VID:PID` hardware fingerprinting |
| **Keystroke Velocity Emulation** | `VelocityEmulationEngine` | Generates acoustic MIDI velocity (1–127) from strike cadence and dwell duration; 5 perceptual curves |
| **Virtual Analog Synthesizer** | `AnalogSynthEngine` | Anti-aliased PolyBLEP oscillators (Saw/Pulse-PWM/Tri/Sine), 4-pole Moog ladder resonant filter, ADSR |
| **Studio Parametric EQ** | `ParametricEQ` | 3-band Low Shelf, Mid Bell, High Shelf using RBJ Audio EQ Cookbook biquads (Direct Form II Transposed) |
| **Modular DSP FX Rack** | `DspFxRack` | Resonant SVF filter, tempo-synced ping-pong delay, Schroeder 8-comb/4-allpass algorithmic reverb |
| **Sample-Accurate Looper** | `LoopTrack` | Sorted timeline cursor scheduling ($O(1)$ block processing), micro-timing nudge, parameter locks |
| **Precision Metronome & Clock** | `MetronomeClock` | Sample-accurate 40–300 BPM clock, odd meters (3/4, 4/4, 6/8), procedural woodblock click generator |
| **Grid Quantizer & Swing** | `BpmQuantizer` | 1/4 to 1/32 triplet snap-to-grid, configurable humanize/swing factor, auto bar-snap on record stop |
| **Multitrack Stem Export** | `StemExporter` | Offline bounce of synchronized 24-bit broadcast WAV stems per lane + summed master mix |
| **Standard MIDI File Export** | `MidiExporter` | High-resolution 960 PPQN Type 1 `.mid` sequence export with micro-timing and GM drum mapping |
| **Realtime MIDI Clock Sync** | `MidiSyncEngine` | 24 PPQN standard MIDI clock (`0xF8`), Start (`0xFA`), Stop (`0xFC`) for external hardware synths & DAWs |
| **Audio Drivers & Latency** | JUCE Audio Engine | ASIO, WASAPI Exclusive, WASAPI Shared, DirectSound with sub-10ms buffer performance |
| **Automated Verification** | `TestRunner` | **20 Automated Test Suites (100% Passing)** covering DSP, oscillators, timing, and export |

---

## 🏗️ Build from Source

### Prerequisites

- Windows 10 / 11 (64-bit)
- CMake 3.22+
- Visual Studio 2019/2022 with C++ workload (MSVC) or Clang
- JUCE 7 or 8 (fetched automatically if not present locally)

```bash
# Clone the repository
git clone https://github.com/EtherK3N/ArmoniComposer.git
cd ArmoniComposer

# Configure CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build GUI Application and Automated Test Suite
cmake --build build --config Release --parallel
```

### Running the Automated Test Suite

```powershell
.\build\ArmoniComposer_Tests_artefacts\Release\ArmoniComposer_Tests.exe
```

Expected output:
```
=======================================================
  ARMONI COMPOSER - AUTOMATED TEST SUITE
=======================================================

[ RUN      ] testAudioEngineProceduralKitAndVoices...
[       OK ] testAudioEngineProceduralKitAndVoices PASSED
...
[ RUN      ] testVelocityEmulationCadenceAndCurves...
[       OK ] testVelocityEmulationCadenceAndCurves PASSED
[ RUN      ] testVelocityEmulationDwellAndGain...
[       OK ] testVelocityEmulationDwellAndGain PASSED

=======================================================
  ALL 20 TEST SUITES PASSED!
=======================================================
```

---

## 🗺️ Roadmap & Milestones

| Phase | Target | Core Innovations | Status |
|---|---|---|---|
| **Phase 1** | v0.1.0 | Win32 Raw Input kernel hook, 32-voice sampler, procedural 808 kit, looper | **Completed [100%]** |
| **Phase 2** | v0.2.0 | MetronomeClock (40-300 BPM), BpmQuantizer, ShiftLayerSystem, Hit Inspector | **Completed [100%]** |
| **Phase 3** | v0.3.0 | PolyBLEP Analog Synth, Moog ladder filter, 3-Band Parametric EQ, SVF, Reverb, Delay | **Completed [100%]** |
| **Phase 4** | v0.4.0 | 24-bit WAV Stem Export, 960 PPQN MIDI Export, 24 PPQN MIDI Clock Sync, Velocity Engine | **Completed [100%]** |
| **Phase 5** | v0.5.0+ | macOS Apple IOKit HID, Linux libevdev/ALSA/JACK, VST3/CLAP plugin format | In Planning |

Full technical detail: [ROADMAP.md](ROADMAP.md)

---

## 🤝 Contributing

We welcome contributions from audio engineers, DSP programmers, C++ developers, and musicians:

- Check out open issues or propose new features via GitHub Discussions.
- Review [CONTRIBUTING.md](CONTRIBUTING.md) for branch strategy and commit standards.
- Follow our [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

---

## 👤 Author & Studio

Designed and engineered by **[@EtherK3N](https://github.com/EtherK3N)** (K3N Solver).  
Part of the **K3N** independent engineering brand — developing low-latency audio software, high-performance web systems, and open-source tooling.

---

## 🔒 Security & Compliance

Armoni Composer adheres to modern cybersecurity resilience standards (including the EU Cyber Resilience Act / NIS2). For our Software Bill of Materials (SBOM), threat modeling, and vulnerability disclosure policy, see **[SECURITY.md](SECURITY.md)**.

---

## 📄 License

This software is licensed under the [GNU Affero General Public License v3.0 (AGPL-3.0)](LICENSE).  
Commercial and proprietary licensing inquiries: `k3n.solver@gmail.com` / `etherk3n@icloud.com`.
