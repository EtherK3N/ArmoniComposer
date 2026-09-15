/*
    DspFxRack.h
    -----------
    Real-time high-performance DSP Sound Sculpting Rack:
      - ResonantFilter: Zero-delay feedback State Variable Filter (SVF)
        [Low-Pass, High-Pass, Band-Pass with Q up to 10.0]
      - StereoDelay: Tempo-synced stereo ping-pong delay with analog HF damping
      - AlgorithmicReverb: Schroeder/Freeverb core with pre-allocated static buffers
      - DspFxRack: Modular master/lane audio processing rack with zero audio-thread heap allocations
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

// ===========================================================================
// 1. ResonantFilter: Zero-Delay Feedback State Variable Filter (SVF)
// ===========================================================================
enum class FilterMode
{
    LowPass = 0,
    HighPass,
    BandPass
};

class ResonantFilter
{
public:
    ResonantFilter();

    void prepare(double sampleRate);
    void reset();

    void setMode(FilterMode newMode) { mode.store(static_cast<int>(newMode), std::memory_order_relaxed); }
    FilterMode getMode() const       { return static_cast<FilterMode>(mode.load(std::memory_order_relaxed)); }

    void setCutoff(float hz)        { cutoffHz.store(juce::jlimit(20.0f, 20000.0f, hz), std::memory_order_relaxed); }
    float getCutoff() const         { return cutoffHz.load(std::memory_order_relaxed); }

    void setResonance(float q)      { resonanceQ.store(juce::jlimit(0.5f, 10.0f, q), std::memory_order_relaxed); }
    float getResonance() const      { return resonanceQ.load(std::memory_order_relaxed); }

    void setEnabled(bool isEnabled) { enabled.store(isEnabled, std::memory_order_relaxed); }
    bool isEnabled() const          { return enabled.load(std::memory_order_relaxed); }

    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    double currentSampleRate = 44100.0;
    std::atomic<int> mode { static_cast<int>(FilterMode::LowPass) };
    std::atomic<float> cutoffHz { 20000.0f };
    std::atomic<float> resonanceQ { 0.707f };
    std::atomic<bool> enabled { true };

    // Filter internal state variables per channel (max 2 channels)
    float s1[2] = { 0.0f, 0.0f };
    float s2[2] = { 0.0f, 0.0f };
};


// ===========================================================================
// 2. StereoDelay: Tempo-Synced Ping-Pong Delay with Analog HF Damping
// ===========================================================================
enum class DelayDivision
{
    Quarter = 0,        // 1/4 note
    Eighth,             // 1/8 note
    EighthDotted,       // 1/8 dotted
    Sixteenth,          // 1/16 note
    SixteenthTriplet    // 1/16 triplet
};

class StereoDelay
{
public:
    StereoDelay();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setDivision(DelayDivision div) { division.store(static_cast<int>(div), std::memory_order_relaxed); }
    DelayDivision getDivision() const   { return static_cast<DelayDivision>(division.load(std::memory_order_relaxed)); }

    void setFeedback(float fb)          { feedback.store(juce::jlimit(0.0f, 0.95f, fb), std::memory_order_relaxed); }
    float getFeedback() const           { return feedback.load(std::memory_order_relaxed); }

    void setDamping(float damp)         { damping.store(juce::jlimit(0.0f, 0.9f, damp), std::memory_order_relaxed); }
    float getDamping() const            { return damping.load(std::memory_order_relaxed); }

    void setPingPong(bool isPingPong)   { pingPong.store(isPingPong, std::memory_order_relaxed); }
    bool isPingPong() const             { return pingPong.load(std::memory_order_relaxed); }

    void setMix(float wetDry)           { mix.store(juce::jlimit(0.0f, 1.0f, wetDry), std::memory_order_relaxed); }
    float getMix() const                { return mix.load(std::memory_order_relaxed); }

    void setEnabled(bool isEnabled)     { enabled.store(isEnabled, std::memory_order_relaxed); }
    bool isEnabled() const              { return enabled.load(std::memory_order_relaxed); }

    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double currentBpm);

private:
    double currentSampleRate = 44100.0;
    static constexpr int maxDelaySeconds = 2;

    std::vector<float> delayBufferLeft;
    std::vector<float> delayBufferRight;
    int bufferSize = 0;
    int writeIndex = 0;

    float lastFilteredFeedback[2] = { 0.0f, 0.0f };

    std::atomic<int>   division { static_cast<int>(DelayDivision::Eighth) };
    std::atomic<float> feedback { 0.40f };
    std::atomic<float> damping  { 0.30f };
    std::atomic<bool>  pingPong { true };
    std::atomic<float> mix      { 0.0f }; // default bypassed (0% wet)
    std::atomic<bool>  enabled  { true };
};


// ===========================================================================
// 3. AlgorithmicReverb: Schroeder / Freeverb Core
// ===========================================================================
class AlgorithmicReverb
{
public:
    AlgorithmicReverb();

    void prepare(double sampleRate);
    void reset();

    void setRoomSize(float size)        { roomSize.store(juce::jlimit(0.0f, 1.0f, size), std::memory_order_relaxed); }
    float getRoomSize() const           { return roomSize.load(std::memory_order_relaxed); }

    void setDamping(float damp)         { damping.store(juce::jlimit(0.0f, 1.0f, damp), std::memory_order_relaxed); }
    float getDamping() const            { return damping.load(std::memory_order_relaxed); }

    void setWidth(float w)              { width.store(juce::jlimit(0.0f, 1.0f, w), std::memory_order_relaxed); }
    float getWidth() const              { return width.load(std::memory_order_relaxed); }

    void setMix(float wetDry)           { mix.store(juce::jlimit(0.0f, 1.0f, wetDry), std::memory_order_relaxed); }
    float getMix() const                { return mix.load(std::memory_order_relaxed); }

    void setEnabled(bool isEnabled)     { enabled.store(isEnabled, std::memory_order_relaxed); }
    bool isEnabled() const              { return enabled.load(std::memory_order_relaxed); }

    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    struct CombFilter
    {
        std::vector<float> buffer;
        int bufIndex = 0;
        float filterStore = 0.0f;
    };

    struct AllPassFilter
    {
        std::vector<float> buffer;
        int bufIndex = 0;
    };

    static constexpr int numCombs = 8;
    static constexpr int numAllPasses = 4;

    std::array<CombFilter, numCombs> combsL;
    std::array<CombFilter, numCombs> combsR;
    std::array<AllPassFilter, numAllPasses> allPassesL;
    std::array<AllPassFilter, numAllPasses> allPassesR;

    double currentSampleRate = 44100.0;
    std::atomic<float> roomSize { 0.5f };
    std::atomic<float> damping  { 0.5f };
    std::atomic<float> width    { 1.0f };
    std::atomic<float> mix      { 0.0f }; // default dry
    std::atomic<bool>  enabled  { true };
};


// ===========================================================================
// 4. DspFxRack: Master / Lane FX Processor
// ===========================================================================
class DspFxRack
{
public:
    DspFxRack();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double currentBpm);

    ResonantFilter&   getFilter() { return filter; }
    StereoDelay&      getDelay()  { return delay; }
    AlgorithmicReverb& getReverb() { return reverb; }

    void setMasterBypass(bool bypass) { masterBypass.store(bypass, std::memory_order_relaxed); }
    bool isMasterBypassed() const     { return masterBypass.load(std::memory_order_relaxed); }

private:
    ResonantFilter    filter;
    StereoDelay       delay;
    AlgorithmicReverb reverb;

    std::atomic<bool> masterBypass { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspFxRack)
};
