# Contributing to Armoni Composer

Thank you for your interest in contributing to **Armoni Composer** (by EtherK3N / K3N Solver).  
Because this is an open-source audio workstation with real-time audio safety and hardware-level input handling, we enforce clear engineering and git collaboration standards to ensure that the codebase remains rock-solid, performant, and easy to maintain for everyone.

---

## 🏛️ Code of Conduct
All contributors, maintainers, and community members are expected to uphold our **[Code of Conduct](CODE_OF_CONDUCT.md)**. Harassment, trolling, or disrespectful behavior will not be tolerated.

---

## 🚀 Git Workflow & Contribution Rules

### 1. Branching Strategy
* **`main` is protected**: Direct pushes of experimental code or untested features to `main` are strictly prohibited.
* Always branch from `main` using descriptive prefixes:
  * `feat/<feature-name>` — e.g. `feat/resonant-lowpass-filter`
  * `fix/<bug-description>` — e.g. `fix/wasapi-buffer-underrun`
  * `perf/<optimization>` — e.g. `perf/lock-free-event-queue`
  * `docs/<topic>` — e.g. `docs/midi-clock-sync-guide`

### 2. Conventional Commits (Mandatory)
Every commit message must follow the [Conventional Commits v1.0.0](https://www.conventionalcommits.org/) specification:
`<type>(<optional scope>): <short description in present tense>`

| Type | When to use | Example |
|---|---|---|
| `feat` | Adding a new user-facing musical capability or DSP node | `feat(dsp): implement resonant stereo low-pass filter` |
| `fix` | Correcting a bug, buffer underrun, or audio glitch | `fix(scheduler): handle modulo wrap-around in loop events` |
| `perf` | Optimizing memory access or reducing audio callback latency | `perf(audio): replace mutex lock with lock-free SPSC FIFO` |
| `refactor` | Code restructuring that does not alter sound or functionality | `refactor(juce): migrate to fine-grained direct module includes` |
| `test` | Adding automated unit tests in `Tests/TestRunner.cpp` | `test(quantizer): add swing blend and triplet timing assertions` |
| `docs` | Modifying documentation, diagrams, or release notes | `docs(readme): update build and testing instructions` |
| `chore` | Build script, CI/CD, dependencies, or git configuration | `chore(ci): update artifact bundle to ArmoniComposer-Windows-x64` |

> ⚠️ **Note**: Commits with generic titles like "update", "fix bug", or "wip" will be rejected during PR review.

### 3. Step-by-Step Contribution Guide
1. **Fork** the repository on GitHub.
2. **Clone** your fork locally with submodules:
   ```bash
   git clone --recursive https://github.com/EtherK3N/ArmoniComposer.git
   cd ArmoniComposer
   ```
3. **Create your feature branch**:
   ```bash
   git checkout -b feat/my-new-feature
   ```
4. **Compile and run the Automated Test Suite**:
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   .\build\ArmoniComposer_Tests_artefacts\Release\ArmoniComposer_Tests.exe
   ```
5. **Commit your changes**:
   ```bash
   git commit -m "feat(synth): add 3-band parametric EQ module"
   ```
6. **Push to your fork and open a Pull Request**:
   * Complete the PR checklist provided in `.github/pull_request_template.md`.

---

## ⚡ Core Audio Engineering Guidelines

Because Armoni Composer operates in real-time at low audio buffer sizes (128–256 samples, sub-5ms latency), code running in the audio path must adhere to strict real-time constraints:

1. **Zero Allocations in the Audio Thread**:
   * Never call `new`, `malloc`, `std::vector::resize`, `std::string`, or any dynamic heap allocator inside `getNextAudioBlock()` or methods called from it.
   * Pre-allocate all buffers, voices, and wavetables during initialization or on the message thread.
2. **Zero Disk & Network I/O**:
   * Never read/write files or make network requests on the audio thread.
3. **Thread Synchronization**:
   * Prefer atomic operations (`std::atomic<float>`, `std::atomic<bool>`) and single-producer single-consumer lock-free ring buffers over mutexes.
4. **C++ Standard**: C++17. Use modern RAII and standard libraries.
5. **JUCE Architecture**: Avoid legacy monolithic `JuceHeader.h`. Include only specific modules needed (e.g. `<juce_audio_basics/juce_audio_basics.h>`).

---

## 💬 Discussions & Security
* **Questions & Proposals**: Open a [GitHub Discussion](https://github.com/EtherK3N/ArmoniComposer/discussions) or [Issue](https://github.com/EtherK3N/ArmoniComposer/issues).
* **Security Vulnerabilities**: Disclose responsibly via [SECURITY.md](SECURITY.md) or email `k3n.solver@gmail.com`.
