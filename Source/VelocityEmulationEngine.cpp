/*
    VelocityEmulationEngine.cpp
    ---------------------------
    Implementation of the real-time keystroke velocity & dynamic expression engine.
*/

#include "VelocityEmulationEngine.h"

VelocityEmulationEngine::VelocityEmulationEngine()
{
    // Reserve capacity to prevent rehashing during performance
    keyStates.reserve(64);
}

void VelocityEmulationEngine::setCurve(VelocityCurve newCurve) noexcept
{
    currentCurve = newCurve;
}

void VelocityEmulationEngine::setSensitivity(float factor) noexcept
{
    sensitivity = juce::jlimit(0.1f, 5.0f, factor);
}

void VelocityEmulationEngine::setVelocityRange(int minVel, int maxVel) noexcept
{
    minVelocity = juce::jlimit(1, 126, minVel);
    maxVelocity = juce::jlimit(minVelocity + 1, 127, maxVel);
}

void VelocityEmulationEngine::setFixedVelocity(int vel) noexcept
{
    fixedVelocity = juce::jlimit(1, 127, vel);
}

std::string VelocityEmulationEngine::makeKey(const juce::String& deviceId, int keyCode)
{
    return deviceId.toStdString() + "_" + std::to_string(keyCode);
}

int VelocityEmulationEngine::applyCurve(float normalizedEnergy) const noexcept
{
    // Clamp energy to [0.0, 1.0]
    const float x = juce::jlimit(0.0f, 1.0f, normalizedEnergy);

    if (currentCurve == VelocityCurve::Fixed)
        return fixedVelocity;

    float curved = x;

    switch (currentCurve)
    {
        case VelocityCurve::Linear:
            curved = x;
            break;

        case VelocityCurve::Exponential:
            // Quadratic response: quiet at bottom, sharp accents on fast strikes
            curved = x * x;
            break;

        case VelocityCurve::Logarithmic:
            // Square root response: rapid rise for light touch playing
            curved = std::sqrt(x);
            break;

        case VelocityCurve::Compressed:
            // Raised floor: minimum 0.5f base energy, compressed top end
            curved = 0.5f + (0.5f * (x * (2.0f - x)));
            break;

        case VelocityCurve::Fixed:
        default:
            return fixedVelocity;
    }

    const float scaled = static_cast<float>(minVelocity) + (static_cast<float>(maxVelocity - minVelocity) * curved);
    return juce::jlimit(minVelocity, maxVelocity, static_cast<int>(std::round(scaled)));
}

int VelocityEmulationEngine::registerKeyDown(const juce::String& deviceId, int virtualKeyCode, juce::int64 timestampMs, bool shiftHeld)
{
    const std::string keyId = makeKey(deviceId, virtualKeyCode);
    auto& state = keyStates[keyId];

    // Compute intervals
    const juce::int64 deltaKeyMs = (state.lastKeyDownMs > 0 && timestampMs > state.lastKeyDownMs)
                                       ? (timestampMs - state.lastKeyDownMs)
                                       : 600;

    const juce::int64 deltaGlobalMs = (lastGlobalStrikeTimestampMs > 0 && timestampMs > lastGlobalStrikeTimestampMs)
                                          ? (timestampMs - lastGlobalStrikeTimestampMs)
                                          : 600;

    // Fast strike cadence: shorter delta implies higher kinetic energy / momentum
    // Typical human drumming cadences: 60ms (fast roll / accent) to 500ms (slow spaced hit)
    const float keyCadenceEnergy = juce::jlimit(0.0f, 1.0f, 1.0f - (static_cast<float>(deltaKeyMs - 50) / 450.0f));
    const float globalCadenceEnergy = juce::jlimit(0.0f, 1.0f, 1.0f - (static_cast<float>(deltaGlobalMs - 30) / 350.0f));

    // Weighted combination of per-key repetition and rhythmic groove velocity
    float rawEnergy = (0.6f * globalCadenceEnergy) + (0.4f * keyCadenceEnergy);

    // Apply sensitivity factor
    rawEnergy *= sensitivity;

    // Shift modifier acts as an accent / ghost-note modifier (+30% velocity boost)
    if (shiftHeld)
        rawEnergy = juce::jmin(1.0f, rawEnergy + 0.30f);

    const int calculatedVelocity = applyCurve(rawEnergy);

    // Update state
    state.keyDownTimestampMs = timestampMs;
    state.lastKeyDownMs = timestampMs;
    state.lastVelocity = calculatedVelocity;
    state.isPressed = true;
    lastGlobalStrikeTimestampMs = timestampMs;

    return calculatedVelocity;
}

juce::int64 VelocityEmulationEngine::registerKeyUp(const juce::String& deviceId, int virtualKeyCode, juce::int64 timestampMs, int* outReleaseVelocity)
{
    const std::string keyId = makeKey(deviceId, virtualKeyCode);
    auto it = keyStates.find(keyId);
    if (it == keyStates.end() || ! it->second.isPressed)
    {
        if (outReleaseVelocity != nullptr)
            *outReleaseVelocity = 64;
        return 0;
    }

    auto& state = it->second;
    state.isPressed = false;

    const juce::int64 dwellMs = (timestampMs >= state.keyDownTimestampMs)
                                    ? (timestampMs - state.keyDownTimestampMs)
                                    : 10;

    if (outReleaseVelocity != nullptr)
    {
        // Release velocity: fast release (short dwell) gives higher release snap
        const float snapEnergy = juce::jlimit(0.0f, 1.0f, 1.0f - (static_cast<float>(dwellMs - 15) / 200.0f));
        *outReleaseVelocity = juce::jlimit(1, 127, static_cast<int>(std::round(40.0f + 87.0f * snapEnergy)));
    }

    return dwellMs;
}

float VelocityEmulationEngine::velocityToGain(int midiVelocity) noexcept
{
    const int vel = juce::jlimit(1, 127, midiVelocity);
    const float norm = static_cast<float>(vel) / 127.0f;
    // Power curve for natural perceptual volume scaling (0.05 min audible gain, up to 1.50 for hard accents)
    return 0.05f + 1.45f * (norm * norm);
}

juce::int64 VelocityEmulationEngine::dwellTimeToSamples(juce::int64 dwellMs, double sampleRate) noexcept
{
    if (dwellMs <= 0 || sampleRate <= 0.0)
        return 0;

    return static_cast<juce::int64>((static_cast<double>(dwellMs) / 1000.0) * sampleRate);
}

void VelocityEmulationEngine::reset()
{
    keyStates.clear();
    lastGlobalStrikeTimestampMs = 0;
}
