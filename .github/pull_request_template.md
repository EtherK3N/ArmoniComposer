## 📋 Pull Request Description

### Summary of Changes
Provide a clear and concise overview of what this PR introduces, fixes, or refactors.

---

### Type of Change
- [ ] 🎸 **feat**: New musical feature, instrument, or DSP processor
- [ ] 🐛 **fix**: Bug fix or hardware compatibility patch
- [ ] ⚡ **perf**: Real-time performance or latency optimization
- [ ] ♻️ **refactor**: Code restructuring without changing external behavior
- [ ] 🧪 **test**: Added or updated automated unit tests
- [ ] 📝 **docs**: Documentation, roadmap, or license updates
- [ ] 🔧 **chore**: Build scripts, CI pipeline, or dependency updates

---

### 🛡️ Quality & Real-Time Audio Checklist
Before submitting, verify each requirement:

- [ ] **Real-Time Safety**: Zero dynamic memory allocations (`new`, `malloc`, `std::vector::resize`) and zero disk/network I/O in `getNextAudioBlock()`.
- [ ] **Thread Safety**: State shared between UI and audio thread is lock-free (atomic or SPSC FIFO) or strictly protected with bounded locks.
- [ ] **Automated Tests**: Unit test suite passes cleanly (`ArmoniComposer_Tests.exe`).
- [ ] **Build Validation**: Compiles without errors on MSVC / Clang with `-Wall / W4`.
- [ ] **Git Hygiene**: Commits adhere to **Conventional Commits** (`feat(...)`, `fix(...)`, etc.). No uninformative or squash-worthy commits.
- [ ] **Attribution & Licensing**: Code is original or compatible with **AGPL-3.0**.
