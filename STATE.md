# Workspace State: Armoni Composer

Last Updated: **September 2026**

---

## 🎵 Project Vision & Manifesto
- High-performance standalone C++17 / JUCE digital audio workstation and live loop station.
- **Mission & Frugal Innovation**: Democratizing music production through hardware upcycling (turning discarded €2-€5 USB computer keyboards into an expressive multi-instrument workstation via Win32 Raw Input hardware disambiguation).
- **Interactive Simulation**: preview/index.html featuring full multi-layer overdub loop station, per-layer modulable FX chains (Delay, Reverb, Low-Pass filter), 30+ synthesized instruments, session save/load (.armoni), and live WebM audio export.
- **License**: GNU Affero General Public License v3.0 (AGPL-3.0) with attribution & commercial licensing requirements.

---

## 🏛 Architecture & Component Status

1. **Source/**:
   - RawInputHandler.h/.cpp: Win32 Raw Input WM_INPUT hardware disambiguation up to 16 keyboards.
   - DeviceManager.h/.cpp: Per-hardware role assignment and %APPDATA% JSON persistence.
   - MappingEngine.h/.cpp: Per-key sound mapping with bank-aware bindings (Drums, Bass, Synth, FX), fallback lookup, and presets.
   - AudioEngine.h/.cpp: 32-voice polyphonic sampler, lock-free voice stealing, procedural starter kit, click buffer generator, and LoopTrack with hit manipulation (moveEvent, setEventDuration, setEventParams, duplicateEvent, removeEvent).
   - MetronomeClock.h/.cpp: Sample-accurate BPM clock (40-300 BPM), time signatures, lock-free atomic phase sync.
   - BpmQuantizer.h/.cpp: Snap-to-grid input quantizer (1/4 to 1/32 triplets), humanize factor, auto bar-snap.
   - ShiftLayerSystem.h/.cpp: Modal layer controller (Shift momentary, CapsLock latched, Tab+1..5 octave transpose, Numpad 3x4 MPC grid).
   - MainComponent.h/.cpp: JUCE UI router & AudioDeviceManager setup.
   - MappingEditorComponent.h/.cpp & PerformanceViewComponent.h/.cpp: GUI views with real-time Layer HUD (Bank, Octave, Numpad mode).
2. **CMakeLists.txt**: JUCE 7/8 build setup including MetronomeClock, BpmQuantizer, and ShiftLayerSystem targets.
3. **Tests/**: Automated Unit Test Suite (Tests/TestRunner.cpp) covering all 8 core subsystems including LoopTrack hit micro-timing, gate duration, and parameter locks.
4. **preview/**: Standalone browser workstation simulator (preview/index.html) with full 4-bank Shift Layer support and interactive Hit Inspector / Parameter Locks Drawer (drag micro-timing, gate length tails, reverb splash, delay throw, pitch transpose).
5. **CI/CD**: .github/workflows/build.yml with native MSVC runner, parallel build, always-on build.log artifact capture, and executable release upload.
6. **Documentation**:
   - README.md, ARCHITECTURE.md, ROADMAP.md, AUDIT.md, CONTRIBUTING.md, TESTING.md, SECURITY.md (SBOM & CRA compliance), LICENSE (AGPL-3.0), RELEASE_NOTES_v0.1.0.md.

---

## 🎯 Current Milestone: Phase 2 (Musical Performance & Hardware Feel) - COMPLETED [100%]

- [x] **Metronome & Clock Engine**: Sample-accurate MetronomeClock (40-300 BPM, odd meters, click buffer).
- [x] **Input Quantization**: BpmQuantizer snap-to-grid (1/4 to 1/32 triplets, humanize factor, bar snapping).
- [x] **Single-Keyboard Shift Layer System**:
  - [x] Shift held: switch active key bank from Drums to Bass.
  - [x] Caps Lock toggle: switch to Synth/FX bank.
  - [x] Tab + number keys: octave transpose per bank (-2 to +2).
  - [x] Numpad layout mode: optimized 3x4 grid for separate USB numeric keypads.
- [x] **Interactive Hit/Step Editor & Parameter Locks**:
  - [x] Micro-timing drag & nudge: drag hit along timeline with mouse or nudge by -10ms/-1ms/+1ms/+10ms with 1/16 grid snap.
  - [x] Gate duration stretching: adjust length of individual hits with visual duration bars/tails.
  - [x] Per-hit parameter locks: isolated reverb splash, delay throw, pitch transpose, and velocity accent/ghost-note on individual hits.
  - [x] Hit lifecycle: duplicate, delete, and audition hits with real-time parameter locks.

---

## 🎯 Current Milestone: Phase 3: DSP Sound Sculpting (v0.3.0) - COMPLETED [100%]
- [x] **ResonantFilter**: Zero-delay feedback State Variable Filter (SVF) with Low-Pass, High-Pass, Band-Pass modes and resonance Q control (0.5 - 10.0).
- [x] **StereoDelay**: Tempo-synced ping-pong delay connected to `MetronomeClock` (1/4 to 1/16 triplets) with analog HF feedback damping.
- [x] **AlgorithmicReverb**: 8-comb / 4-allpass Schroeder core with pre-allocated static buffers and zero audio-thread memory allocation.
- [x] **DspFxRack**: Master and per-lane processor integrated into `AudioEngine::getNextAudioBlock()`.
- [x] **Automated Tests**: Added `testDspFxRackProcessing` covering filter frequency attenuation, delay echo arrival timing, and reverb tail stability (9/9 suites passing).

---

## 🚀 Next Milestone: Phase 4: Studio Integration & Multitrack Audio Export (v0.4.0)
- Multitrack WAV stem export for DAW import (Ableton, FL Studio, Reaper).
- MIDI Clock input/output synchronization.
- Ableton Link support for wireless jam sessions.

---

## 📋 Completed Remediation Tasks (Technical Audit & Hygiene)
Detailed post-mortem and audit documented in `TECHNICAL_AUDIT_ACTION_PLAN.md`:

1. [x] **Fixed MSVC CI Failure (`MainComponent.cpp`)**:
   - Resolved `error C3861: 'getApplicationVersion': identifier not found` at line 119 using `juce::JUCEApplication::getInstance()->getApplicationVersion()`.
   - Updated release URL placeholder (`YOUR_GITHUB` -> `EtherK3N/ArmoniComposer`).
2. [x] **LoopTrack Event Scheduling Optimization**:
   - Replaced $O(N)$ linear loop `for (const auto& ev : recordedEvents)` per audio block with a sorted timeline cursor ($O(1)$ block check) and modulo wrap-around handling.
3. [x] **De-buzzwording & Realism in Documentation**:
   - Stripped AI-generated marketing hyperboles from `README.md` and `ARCHITECTURE.md`.
   - Documented real concurrency status (audioLock ScopedLock vs future lock-free SPSC FIFO queue).
   - Framed truthfully as an open-source v0.1.0 prototype / hardware-upcycling project.
4. [x] **Git Cleanup & History Hygiene**:
   - Consolidated trial-and-error commits into clean Conventional Commits.
5. [x] **Modern Modular JUCE Refactoring (Zero JuceHeader.h)**:
   - Removed monolithic auto-generated `JuceHeader.h` from all sources and tests.
   - Replaced with fine-grained direct module includes (`<juce_core/juce_core.h>`, `<juce_gui_basics/juce_gui_basics.h>`, `<juce_audio_basics/juce_audio_basics.h>`, etc.).
   - Removed deprecated `NEEDS_JUCE_HEADER` and explicitly linked required JUCE modules in `CMakeLists.txt`.
   - Translated `DOCKER_BUILD_AND_TEST.bat` to English and purged legacy `AVVIA_SIMULATORE.bat`.
6. [x] **Product Rebranding & Hyphenless SEO Optimization (Option 1)**:
   - Tool Name: `Armoni Composer` (clean, zero hyphens).
   - Copyright & Rights: `Copyright (C) 2026 EtherK3N (K3N Solver) - Armoni Composer` in `LICENSE`.
   - Author signature: `by EtherK3N (K3N Solver)`.
   - Repository slug: `EtherK3N/ArmoniComposer`.
   - Updated CMake targets (`ArmoniComposer`, `ArmoniComposer_Tests`), CI workflow (`.github/workflows/build.yml`), JUCE App class (`ArmoniComposerApplication`), and Web simulator SEO meta keywords/descriptions.


