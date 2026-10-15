/*
    ParametricEQ.h
    --------------
    Studio-grade 3-Band Parametric Equalizer for K3N Armoni Composer:
      - Low Shelf (default 100 Hz, +/-24 dB)
      - Mid Peaking / Bell (default 1000 Hz, Q 0.1..10.0, +/-24 dB)
      - High Shelf (default 8000 Hz, +/-24 dB)
    Uses Robert Bristow-Johnson (RBJ) Audio EQ Cookbook biquad filters in
    Direct Form II Transposed for optimal numerical stability and minimal delay.
    Real-time safe: Zero heap allocation in processBlock().
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

class BiquadFilter
{
public:
    BiquadFilter();

    void reset();
    void setCoefficients(float b0, float b1, float b2, float a0, float a1, float a2);

    float processSample(float in, int channel)
    {
        // Direct Form II Transposed
        const float out = b0_n * in + s1[channel];
        s1[channel] = b1_n * in - a1_n * out + s2[channel];
        s2[channel] = b2_n * in - a2_n * out;
        return out;
    }

private:
    float b0_n = 1.0f;
    float b1_n = 0.0f;
    float b2_n = 0.0f;
    float a1_n = 0.0f;
    float a2_n = 0.0f;

    float s1[2] = { 0.0f, 0.0f };
    float s2[2] = { 0.0f, 0.0f };
};

class ParametricEQ
{
public:
    ParametricEQ();

    void prepare(double sampleRate);
    void reset();

    // Low Shelf controls
    void setLowShelf(float freqHz, float gainDb);
    float getLowShelfFreq() const { return lowFreq.load(std::memory_order_relaxed); }
    float getLowShelfGain() const { return lowGain.load(std::memory_order_relaxed); }

    // Mid Peaking Bell controls
    void setMidBand(float freqHz, float q, float gainDb);
    float getMidFreq() const { return midFreq.load(std::memory_order_relaxed); }
    float getMidQ() const    { return midQ.load(std::memory_order_relaxed); }
    float getMidGain() const { return midGain.load(std::memory_order_relaxed); }

    // High Shelf controls
    void setHighShelf(float freqHz, float gainDb);
    float getHighShelfFreq() const { return highFreq.load(std::memory_order_relaxed); }
    float getHighShelfGain() const { return highGain.load(std::memory_order_relaxed); }

    void setEnabled(bool isEnabled) { enabled.store(isEnabled, std::memory_order_relaxed); }
    bool isEnabled() const          { return enabled.load(std::memory_order_relaxed); }

    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    void updateLowShelf();
    void updateMidBand();
    void updateHighShelf();

    double currentSampleRate = 44100.0;

    std::atomic<bool> enabled { true };

    // Parameters
    std::atomic<float> lowFreq  { 100.0f };
    std::atomic<float> lowGain  { 0.0f };

    std::atomic<float> midFreq  { 1000.0f };
    std::atomic<float> midQ     { 0.707f };
    std::atomic<float> midGain  { 0.0f };

    std::atomic<float> highFreq { 8000.0f };
    std::atomic<float> highGain { 0.0f };

    std::atomic<bool> needsCoeffUpdate { true };

    // Filters per band
    BiquadFilter lowFilter;
    BiquadFilter midFilter;
    BiquadFilter highFilter;
};
