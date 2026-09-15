# 🧪 Testing & Verification Guide: Armoni Composer

This guide explains how to compile, verify, and test **Armoni Composer** — both through the **Automated Unit Test Suite** and via **Physical Hardware Verification** on Windows.

---

## 🐳 0. Isolated Testing with Docker (Recommended - Zero Host Toolchains!)

If you have **Docker Desktop** installed on your machine, you don't even need CMake or Visual Studio on your Windows host!

### 1-Click Docker Test:
Double-click: **[`DOCKER_BUILD_AND_TEST.bat`](DOCKER_BUILD_AND_TEST.bat)**

### Or via Terminal:
```bash
# 1. Build the isolated testing container:
docker build -t k3n-armoni-tester .

# 2. Run automated tests inside the container:
docker run --rm k3n-armoni-tester
```

---

## 🛠️ 1. Compiling on Host (Windows Native)

### Prerequisites
- **Windows 10 or 11 (64-bit)**
- **Visual Studio 2019 / 2022** (with *Desktop development with C++*) or **Clang**
- **CMake 3.22+**
- **JUCE 7 or 8** (as a git submodule in `./JUCE` or installed on your system)

### Step-by-Step Compilation Commands:
Open **PowerShell** or **Command Prompt** in the project directory:

```bash
# 1. Clone / Initialize JUCE submodule if not already present:
git submodule update --init --recursive
# Or if cloning fresh:
# git submodule add https://github.com/juce-framework/JUCE.git JUCE

# 2. Generate CMake build files:
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Compile both the main GUI app and the Test Suite:
cmake --build build --config Release
```

---

## 🤖 2. Running Automated Unit Tests

We have built a dedicated automated test suite in [`Tests/TestRunner.cpp`](Tests/TestRunner.cpp) that validates core functionality without needing physical keyboards or audio output hardware.

### To Run the Test Suite:
```bash
# Execute the compiled test binary:
.\build\ArmoniComposer_Tests_artefacts\Release\ArmoniComposer_Tests.exe
```

### What It Tests:
- **`testAudioEngineProceduralKitAndVoices`**:
  - Validates procedural drumkit synthesis in memory (`starter://kick`, `starter://snare`, `starter://bass808`).
  - Verifies polyphonic voice mixing and non-clipping audio buffer rendering.
- **`testLoopTrackWrapAroundTriggerMath`**:
  - Tests sample-accurate discrete trigger registration and modulo wrap-around loop playback.
- **`testMappingEngineJsonSerialization`**:
  - Tests key assignment, renaming, removal, and complete JSON persistence round-trip integrity.

---

## 🎹 3. Physical Hardware & Live Testing

### Step 1: Launch the Main App
```bash
.\build\ArmoniComposer_artefacts\Release\ArmoniComposer.exe
```

### Step 2: Audio Device Setup
1. Click **`Audio Settings...`** in the top-right corner.
2. Under **Audio device type**, select **ASIO** (if you have an audio interface or *ASIO4ALL* / *FL Studio ASIO*) or **Windows Audio (WASAPI Exclusive)**.
3. Select a buffer size of **128 samples** or **256 samples** for instant low-latency response.

### Step 3: Test Multi-Keyboard Disambiguation
1. Plug in **2 or more USB keyboards** (or a laptop keyboard + a USB keyboard / numeric numpad).
2. Go to **`1. Mapping Editor`**.
3. Open the **`Keyboard / Device`** dropdown at the top: you will see each physical keyboard listed separately with its unique USB port identifier.
4. Select a sound from the **`Sample Library`** (e.g. `Built-in Kick (808)`).
5. Click **`Learn Key`** $\to$ Press `Spacebar` on **Keyboard #1**.
6. Select `Built-in Snare Drum` $\to$ Click **`Learn Key`** $\to$ Press `Spacebar` on **Keyboard #2**.
7. Click **`Save Preset`**.
8. Press `Spacebar` on Keyboard #1 (triggers Kick) and `Spacebar` on Keyboard #2 (triggers Snare) simultaneously. **Notice that the identical physical key produces two completely different instruments without conflict.**

### Step 4: Test Multi-Track Looping
1. Switch to **`2. Live Performance`**.
2. Click **`Rec`** on the **Drums Track**.
3. Play a 4-beat rhythm on Keyboard #1.
4. Click **`Stop Rec`**: the drum pattern immediately begins looping seamlessly.
5. Click **`Rec`** on the **Bass Track**: play a bassline on Keyboard #2 over the drum loop.
6. Click **`Stop Rec`**: both lanes now loop together synchronously!
7. Use **`Mute`** / **`Unmute`** to drop instruments in and out live.
