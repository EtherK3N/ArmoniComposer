# Roadmap: K3N Armoni Composer

This is a living document. It describes what exists, what is being built, and what the community can help build next.

---

## Current State: v0.1.0 Preview

The core architecture and low-level technical foundations are implemented. The interactive browser simulator at `preview/index.html` demonstrates the complete intended workflow. The native C++ executable is in active development.

### What exists today

- [x] Win32 Raw Input hardware disambiguation (up to 16 concurrent USB keyboards, each assigned a unique device path and instrument role)
- [x] `RawInputHandler` — WM_INPUT interception and per-device routing before OS merging
- [x] `DeviceManager` — role assignment and JSON persistence in `%APPDATA%/LoopStation`
- [x] `MappingEngine` — per-key sound assignment and preset save/load
- [x] `AudioEngine` — 32-voice polyphonic sampler with voice stealing, zero heap allocation in the audio thread
- [x] Built-in procedural sound synthesizer — 808 Kick, Snare, Hi-Hat, Sub-Bass, Acid 303, Moog Saw, Rhodes
- [x] Sample-accurate multi-track loop station — record, overdub, mute, clear per lane
- [x] ASIO / WASAPI Exclusive / WASAPI Shared / DirectSound driver support via JUCE
- [x] Custom sample import — WAV, MP3, FLAC, OGG decoded to RAM at full quality
- [x] Interactive browser simulator (`preview/index.html`) — full UX preview with Web Audio API
- [x] CMake build system, Docker test environment, automated GitHub Actions CI
- [x] Metronome (audible click, BPM 40–240, visual LED) — implemented in simulator, porting to native
- [x] Per-layer modular FX routing — Delay, Reverb, Low-Pass Filter

---

## Phase 2: Rhythmic Precision & Single-Keyboard Expressivity (v0.2.0)

**Goal**: Make the native app feel as tight and musical as a hardware loop station.

### Metronome & Clock Engine
- [x] Sample-accurate BPM clock in native C++ JUCE component (`MetronomeClock`)
- [x] Time signature support: 4/4, 3/4, 6/8, and odd meters
- [x] Procedural click synthesis: accentuated downbeat, subdivisions (1/8, 1/16, 1/32, triplets)

### Input Quantization
- [x] Snap-to-grid: round recorded trigger timestamps to nearest grid subdivision (`BpmQuantizer`)
- [x] Configurable humanize/swing factor (0% = robotic grid, 100% = natural feel)
- [x] Auto-loop-length detection: snap loop end point to nearest bar boundary on recording stop

### Single-Keyboard Shift Layer System
- [x] `Shift` held: switch active key bank from Drums to Bass
- [x] `Caps Lock` toggle: switch to Synth/FX bank
- [x] `Tab` + number keys: octave transpose per bank
- [x] Numpad layout mode: optimized 3x4 grid for separate USB numeric keypads

---

## Phase 3: DSP Sound Sculpting (v0.3.0)

**Goal**: Professional studio-quality sound shaping on every track.

### Per-Lane FX Rack (Native C++)
- [x] Multi-mode resonant filter — Low-Pass, High-Pass, Band-Pass with Q control per lane (`ResonantFilter`)
- [x] Stereo ping-pong delay — tempo-synced with feedback damping (`StereoDelay`)
- [x] Algorithmic reverb — Schroeder/Freeverb core with zero audio-thread allocation (`AlgorithmicReverb`)
- [ ] 3-band parametric EQ — Low Shelf, Mid Bell, High Shelf

### Pitch & Time
- [ ] Real-time sample resampler using `juce::LagrangeInterpolator`
- [ ] Granular pitch transpose — change pitch without changing playback speed
- [ ] Velocity emulation — derived from keystroke timing interval

---

## Phase 4: Studio Integration (v0.4.0)

**Goal**: Move from live performance into a full creative production workflow.

### Export
- [x] Multitrack stem export — bounce all loop lanes as synchronized 24-bit WAV stems (`StemExporter`)
- [x] MIDI file export (`.mid`) — recorded trigger patterns as standard MIDI sequences (`MidiExporter`)
- [x] Virtual MIDI output & MIDI Clock — send 24 PPQN real-time clock to external DAWs and synths (`MidiSyncEngine`)

### Sync
- [ ] Ableton Link integration — wireless tempo sync with other devices on local network

---

## Phase 5: Cross-Platform & Plugin Ecosystem (v0.5.0+)

**Goal**: Reach creators on every platform.

- [ ] macOS: Apple IOKit HID Manager for per-keyboard disambiguation
- [ ] Linux: libevdev + udev, ALSA and JACK/PipeWire support
- [ ] VST3 / CLAP / AU plugin edition — run inside any DAW host
- [ ] Community preset hub — `.armoni` pack format (JSON mapping + bundled samples)

---

## How to Contribute

Pick any unchecked item above. Open an issue to claim it, ask questions, or propose a different approach.

All contributions are welcome: C++ audio/DSP code, UI work, documentation, hardware testing reports (unusual keyboards, edge cases), use case stories.

See [CONTRIBUTING.md](CONTRIBUTING.md) for setup and PR guidelines.
