# Security Policy & Dependency Compliance (SBOM)

## 1. Overview & Commitment to Security

**K3N Armoni Composer** is designed with an **offline-first, zero-telemetry, privacy-by-design** architecture.
In compliance with modern software resilience frameworks (including the **EU Cyber Resilience Act (CRA)** and **NIS2 Directive**), this document defines our vulnerability disclosure process, threat model, and a transparent **Software Bill of Materials (SBOM)** tracking all third-party libraries and runtime components to ensure rapid vulnerability detection and remediation.

---

## 2. Software Bill of Materials (SBOM)

The following matrix documents all third-party technologies, frameworks, and system APIs utilized by K3N Armoni Composer:

| Component | Pinned Version / Tag | Provenance / Repository | Purpose | License | CVE / Vulnerability Scope |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **JUCE Framework** | `7.0.12` (pinned) | `https://github.com/juce-framework/JUCE` | Core audio device I/O, format decoders, cross-platform DSP utilities, and GUI rendering | AGPL-3.0 / Commercial | Memory safety in audio buffer copies, audio file header parsing (WAV/MP3/FLAC) |
| **Win32 Raw Input API** | Windows 10/11 SDK | Native OS Library (`user32.dll`, `hid.dll`, `setupapi.dll`) | Hardware-level USB keyboard interception (`WM_INPUT`) before OS merge | Microsoft Platform SDK | Buffer validation on `GetRawInputBuffer` and `RAWINPUT` payload parsing |
| **C++ Standard Library** | ISO C++17 | MSVC STL / GCC libstdc++ | Standard algorithms, memory containers, atomics | Compiler vendor | Safe bounds checking, integer arithmetic, lock-free atomics |
| **WebAudio Engine** *(Preview)* | W3C Standard | Native browser implementation | Interactive browser workstation simulation (`preview/index.html`) | Browser Standard | Client-side audio graph execution |
| **Google Fonts** *(Preview)* | HTTPS CDN | `fonts.googleapis.com` (Outfit & JetBrains Mono) | Modern typography for simulator interface | OFL-1.1 | External asset loading restricted strictly to fonts; no script execution |

---

## 3. Threat Model & Privacy Verification

### Privacy & Data Handling Guarantee
- **Zero Telemetry / Analytics**: The application collects no user metrics, tracking cookies, or behavioral data.
- **No Keystroke Logging**: Although the application intercepts raw USB keyboard messages (`WM_INPUT`), keystrokes are processed solely in volatile memory for local audio triggering. Key events are never logged to disk, persisted, or transmitted over any network socket.
- **Local-Only JSON Storage**: Device friendly names and custom key-sound mappings are stored exclusively in the local `%APPDATA%/LoopStation/` directory.
- **Zero Embedded Credentials**: No secret API keys, personal access tokens, private system paths, or developer credentials exist in the source code or test suites.

### Security Boundaries & Input Sanitization
- **Audio File Importer**: Decodes audio files using JUCE format readers with memory validation to protect against malicious audio headers or buffer overruns.
- **Release Checker**: The update checker queries `https://api.github.com` via read-only HTTPS GET requests with a strict timeout (3000ms). It parses release JSON objects without executing remote code.

---

## 4. Reporting a Vulnerability

We take the security of our users seriously. If you discover a security vulnerability or supply chain risk, please report it privately:

1. **Preferred Method**: Open a **Private Vulnerability Report** via GitHub Security Advisories at:  
   `https://github.com/EtherK3N/ArmoniComposer/security/advisories/new`
2. **Alternative Contact**: Email security disclosures to **`k3n.solver@gmail.com`** with the subject `[SECURITY ISSUE] Armoni Composer`.

### Response SLA & Remediation Policy
- **Initial Acknowledgement**: Within **48 hours**.
- **Assessment & Triage**: Within **5 business days**, detailing severity and an action plan.
- **Public Disclosure**: Coordinated disclosure following patch release, ensuring users have access to the fix before publication.
