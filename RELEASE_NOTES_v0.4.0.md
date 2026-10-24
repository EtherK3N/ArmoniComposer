# Armoni Composer — Release v0.4.0 (October 2026)

**Virtual Analog Synthesis, Studio DSP Rack, DAW Export & Dynamic Velocity Emulation Release**  
Author: **[@EtherK3N](https://github.com/EtherK3N)** (K3N Solver)  
License: **GNU Affero General Public License v3.0 (AGPL-3.0)**

---

## 🚀 Overview

**Armoni Composer v0.4.0** marks a major architectural milestone. What began as a frugal hardware-upcycling prototype has evolved into a complete, studio-grade digital audio workstation and live performance instrument. 

v0.4.0 bridges the gap between binary computer keyboards and expressive acoustic performance by introducing the **Velocity Emulation Engine**, a complete **Virtual Analog PolyBLEP Synthesizer**, a **Studio DSP FX Rack**, and full **Multitrack Stem & MIDI Export** capabilities.

---

## ⚡ What is New in v0.4.0

### 1. Dynamic Keystroke Velocity Emulation Engine (`Source/VelocityEmulationEngine.*`)
- **Acoustic Velocity from Membrane Keyboards:** Derives expressive MIDI velocity ($1–127$) from strike cadence intervals ($\Delta t_{inter}$) and key dwell hold duration ($\Delta t_{dwell}$).
- **Five Musical Response Curves:**
  - `Linear`: Proportional energy mapping for general performance.
  - `Exponential`: Quadratic power curve matching human auditory loudness perception (fine control on subtle ghost notes, sharp punch on hard hits).
  - `Logarithmic`: Sensitive light-touch response for rapid synth arpeggios.
  - `Compressed`: Elevated dynamic floor ($85–127$) designed specifically for punchy hip-hop/electronic drums.
  - `Fixed`: Deterministic velocity ($100$ / gain $1.0$) for classic step sequencing.
- **Sample-Accurate Gate Duration:** Converts key release dwell time directly into sample lengths for the looper and sequencer.
- **Zero-Allocation Audio Thread Safety:** Pre-allocated state tracking per keyboard hardware ID and virtual key code.

### 2. PolyBLEP Virtual Analog Synthesizer (`Source/AnalogSynthEngine.*`)
- **Band-Limited Oscillators:** PolyBLEP (Polynomial Band-Limited Step) algorithm eliminating digital aliasing across Saw, Pulse (with variable PWM), Triangle, and Sine waveforms.
- **4-Pole 24 dB/oct Moog Ladder Filter:** Non-linear analog modeling with soft hyperbolic tangent (`tanh`) saturation, resonance Q control up to self-oscillation, and envelope modulation.
- **Exponential ADSR Envelope:** Analog-style curvature on decay and release stages.
- **8-Voice Polyphony:** Intelligent voice allocator with note stealing, glide/portamento, and integrated audio thread rendering.

### 3. Studio Parametric EQ & DSP FX Rack (`Source/ParametricEQ.*`, `Source/DspFxRack.*`)
- **3-Band Parametric Equalizer:** Low Shelf, Mid Peaking Bell, and High Shelf filters based on Robert Bristow-Johnson (RBJ) Audio EQ Cookbook equations implemented in Direct Form II Transposed structure.
- **Resonant State Variable Filter (SVF):** Zero-delay feedback multi-mode filter (Low-Pass, High-Pass, Band-Pass) with resonance Q up to $10.0$.
- **Tempo-Synced Ping-Pong Delay:** Rhythmic subdivisions ($1/4$ to $1/16$ triplets) synchronized with `MetronomeClock` and analog HF feedback damping.
- **Schroeder Algorithmic Reverb:** 8 parallel comb filters and 4 cascaded all-pass diffusers with static pre-allocated memory buffers (zero heap allocations during playback).

### 4. Multitrack Audio & MIDI Studio Export (`Source/StemExporter.*`, `Source/MidiExporter.*`, `Source/MidiSyncEngine.*`)
- **24-bit WAV Stem Export (`StemExporter`):** Offline bounce of isolated, synchronized broadcast-ready WAV stems for every loop lane plus a summed master mix with peak normalization.
- **Type 1 Standard MIDI Sequence Export (`MidiExporter`):** High-resolution $960$ PPQN standard MIDI file generation (`.mid`) with micro-timing offsets, gate duration, and GM drum/chromatic mapping.
- **Real-Time 24 PPQN MIDI Clock Sync (`MidiSyncEngine`):** Emits standard MIDI Clock (`0xF8`), Start (`0xFA`), Stop (`0xFC`), and Continue (`0xFB`) pulses to synchronize external hardware drum machines, grooveboxes, and host DAWs.

### 5. Persistent Hardware VID/PID Fingerprinting (`Source/RawInputHandler.*`, `Source/DeviceManager.*`)
- Replaces volatile Windows OS device handles with persistent `VID:PID:MI` hardware fingerprints.
- Keyboards unplugged and reconnected to different USB ports automatically retain their assigned musical role (Drums, Bass, Chords, FX) and custom friendly names.

### 6. Interactive Zero-Install Web Simulator on GitHub Pages
- Full browser DAW accessible live with zero installation via **GitHub Pages** (`https://etherk3n.github.io/ArmoniComposer/`).
- Includes real-time canvas oscilloscope, 3-track overdub looper, parameter locks drawer, and WebM export.

### 7. Automated Test Suite (20 Suites — 100% Passing)
- Extended `Tests/TestRunner.cpp` to **20 automated test suites** verifying every DSP filter, oscillator harmonic stability, hardware fingerprint matching, MIDI file binary syntax, and velocity cadence calculations.

---

## 🛠 Architectural Verification Matrix

| Component | Language / Spec | Concurrency / Memory Model | Status |
|---|---|---|---|
| `RawInputHandler` | C++17 / Win32 | Persistent VID:PID fingerprinting | Verified |
| `VelocityEmulationEngine` | C++17 / Lock-free | Dynamic strike cadence & dwell scaling | Verified |
| `AnalogSynthEngine` | C++17 / PolyBLEP | 8-voice polyphonic, Moog ladder filter | Verified |
| `ParametricEQ` | C++17 / RBJ Biquads | Direct Form II Transposed | Verified |
| `DspFxRack` | C++17 / DSP | Zero heap allocation in audio thread | Verified |
| `StemExporter` | C++17 / 24-bit WAV | Sample-accurate offline stem bounce | Verified |
| `MidiExporter` | C++17 / SMF Type 1 | 960 PPQN micro-timing resolution | Verified |
| `MidiSyncEngine` | C++17 / MIDI 1.0 | 24 PPQN real-time master clock | Verified |
| `Automated Test Suite` | C++17 / JUCE Console | **20/20 Test Suites Passing** | Verified |
