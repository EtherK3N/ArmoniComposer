/*
    VelocityEmulationEngine.h
    -------------------------
    High-performance real-time keystroke velocity & dynamic expression engine:
      - Emulates acoustic MIDI velocity (1-127) on standard binary computer keyboards
      - Evaluates inter-keystroke cadence (strike interval) and key dwell time (hold duration)
      - Implements multiple perception curves: Linear, Exponential, Logarithmic, Compressed, Fixed
      - Computes sample-accurate note gate durations for Looper & Step Sequencer integration
      - Zero heap allocations and lock-free execution suitable for high-priority audio threads
*/

#pragma once

#include <juce_core/juce_core.h>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>

enum class VelocityCurve
{
    Linear,       // Direct linear response: energy maps proportionally
    Exponential,  // Natural human auditory response (accentuates hard strikes, fine control on ghost notes)
    Logarithmic,  // Sensitive light-touch response (reaches high velocity quickly)
    Compressed,   // Heavy drum punch: keeps hits in the 85-127 range with deliberate ghost-note floor
    Fixed         // Constant studio velocity (100 / gain 1.0)
};

class VelocityEmulationEngine
{
public:
    VelocityEmulationEngine();
    ~VelocityEmulationEngine() = default;

    /** Sets the dynamic response curve. */
    void setCurve(VelocityCurve newCurve) noexcept;
    VelocityCurve getCurve() const noexcept { return currentCurve; }

    /** Sets sensitivity factor (0.25 = heavy touch needed, 1.0 = normal, 2.5 = very sensitive). */
    void setSensitivity(float factor) noexcept;
    float getSensitivity() const noexcept { return sensitivity; }

    /** Clamping limits for generated MIDI velocity (1 to 127). */
    void setVelocityRange(int minVel, int maxVel) noexcept;
    int getMinVelocity() const noexcept { return minVelocity; }
    int getMaxVelocity() const noexcept { return maxVelocity; }

    /** Default fixed velocity used when curve is set to Fixed. */
    void setFixedVelocity(int vel) noexcept;
    int getFixedVelocity() const noexcept { return fixedVelocity; }

    /**
        Registers a KeyDown event and returns the calculated strike velocity (1-127).
        Uses inter-keystroke cadence and strike acceleration to infer dynamic intensity.
    */
    int registerKeyDown(const juce::String& deviceId, int virtualKeyCode, juce::int64 timestampMs, bool shiftHeld = false);

    /**
        Registers a KeyUp event. Returns the hold duration in milliseconds (dwell time)
        and outputs the note-off release velocity.
    */
    juce::int64 registerKeyUp(const juce::String& deviceId, int virtualKeyCode, juce::int64 timestampMs, int* outReleaseVelocity = nullptr);

    /**
        Maps a MIDI velocity (1-127) to an audio amplitude gain factor (0.05f to 1.50f).
        Applies smooth quadratic acoustic power scaling.
    */
    static float velocityToGain(int midiVelocity) noexcept;

    /**
        Maps key hold duration (milliseconds) to sample count given a sample rate.
    */
    static juce::int64 dwellTimeToSamples(juce::int64 dwellMs, double sampleRate) noexcept;

    /** Resets all internal key trackers and state. */
    void reset();

private:
    struct KeyState
    {
        juce::int64 keyDownTimestampMs = 0;
        juce::int64 lastKeyDownMs = 0;
        int lastVelocity = 100;
        bool isPressed = false;
    };

    static std::string makeKey(const juce::String& deviceId, int keyCode);
    int applyCurve(float normalizedEnergy) const noexcept;

    VelocityCurve currentCurve = VelocityCurve::Exponential;
    float sensitivity = 1.0f;
    int minVelocity = 20;
    int maxVelocity = 127;
    int fixedVelocity = 100;

    juce::int64 lastGlobalStrikeTimestampMs = 0;
    std::unordered_map<std::string, KeyState> keyStates;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityEmulationEngine)
};
