/*
    ParametricEQ.cpp
    ----------------
    Implementation of 3-Band Parametric Equalizer for K3N Armoni Composer.
*/

#include "ParametricEQ.h"
#include <algorithm>

// ===========================================================================
// BiquadFilter
// ===========================================================================
BiquadFilter::BiquadFilter()
{
    reset();
}

void BiquadFilter::reset()
{
    s1[0] = 0.0f;
    s1[1] = 0.0f;
    s2[0] = 0.0f;
    s2[1] = 0.0f;
}

void BiquadFilter::setCoefficients(float b0, float b1, float b2, float a0, float a1, float a2)
{
    if (std::abs(a0) < 1.0e-8f)
        return;

    const float invA0 = 1.0f / a0;
    b0_n = b0 * invA0;
    b1_n = b1 * invA0;
    b2_n = b2 * invA0;
    a1_n = a1 * invA0;
    a2_n = a2 * invA0;
}

// ===========================================================================
// ParametricEQ
// ===========================================================================
ParametricEQ::ParametricEQ()
{
    reset();
}

void ParametricEQ::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    reset();
    updateLowShelf();
    updateMidBand();
    updateHighShelf();
    needsCoeffUpdate.store(false, std::memory_order_relaxed);
}

void ParametricEQ::reset()
{
    lowFilter.reset();
    midFilter.reset();
    highFilter.reset();
}

void ParametricEQ::setLowShelf(float freqHz, float gainDb)
{
    lowFreq.store(juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), freqHz), std::memory_order_relaxed);
    lowGain.store(juce::jlimit(-24.0f, 24.0f, gainDb), std::memory_order_relaxed);
    needsCoeffUpdate.store(true, std::memory_order_relaxed);
}

void ParametricEQ::setMidBand(float freqHz, float q, float gainDb)
{
    midFreq.store(juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), freqHz), std::memory_order_relaxed);
    midQ.store(juce::jlimit(0.1f, 10.0f, q), std::memory_order_relaxed);
    midGain.store(juce::jlimit(-24.0f, 24.0f, gainDb), std::memory_order_relaxed);
    needsCoeffUpdate.store(true, std::memory_order_relaxed);
}

void ParametricEQ::setHighShelf(float freqHz, float gainDb)
{
    highFreq.store(juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), freqHz), std::memory_order_relaxed);
    highGain.store(juce::jlimit(-24.0f, 24.0f, gainDb), std::memory_order_relaxed);
    needsCoeffUpdate.store(true, std::memory_order_relaxed);
}

void ParametricEQ::updateLowShelf()
{
    const float f0 = lowFreq.load(std::memory_order_relaxed);
    const float dB = lowGain.load(std::memory_order_relaxed);

    if (std::abs(dB) < 0.001f)
    {
        lowFilter.setCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        return;
    }

    const float A = std::pow(10.0f, dB / 40.0f);
    const float w0 = 2.0f * juce::MathConstants<float>::pi * (f0 / static_cast<float>(currentSampleRate));
    const float cosW = std::cos(w0);
    const float sinW = std::sin(w0);
    const float beta = std::sqrt(2.0f * A) * sinW;

    const float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosW + beta);
    const float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW);
    const float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - beta);
    const float a0 = (A + 1.0f) + (A - 1.0f) * cosW + beta;
    const float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW);
    const float a2 = (A + 1.0f) + (A - 1.0f) * cosW - beta;

    lowFilter.setCoefficients(b0, b1, b2, a0, a1, a2);
}

void ParametricEQ::updateMidBand()
{
    const float f0 = midFreq.load(std::memory_order_relaxed);
    const float q  = midQ.load(std::memory_order_relaxed);
    const float dB = midGain.load(std::memory_order_relaxed);

    if (std::abs(dB) < 0.001f)
    {
        midFilter.setCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        return;
    }

    const float A = std::pow(10.0f, dB / 40.0f);
    const float w0 = 2.0f * juce::MathConstants<float>::pi * (f0 / static_cast<float>(currentSampleRate));
    const float cosW = std::cos(w0);
    const float sinW = std::sin(w0);
    const float alpha = sinW / (2.0f * q);

    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosW;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosW;
    const float a2 = 1.0f - alpha / A;

    midFilter.setCoefficients(b0, b1, b2, a0, a1, a2);
}

void ParametricEQ::updateHighShelf()
{
    const float f0 = highFreq.load(std::memory_order_relaxed);
    const float dB = highGain.load(std::memory_order_relaxed);

    if (std::abs(dB) < 0.001f)
    {
        highFilter.setCoefficients(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        return;
    }

    const float A = std::pow(10.0f, dB / 40.0f);
    const float w0 = 2.0f * juce::MathConstants<float>::pi * (f0 / static_cast<float>(currentSampleRate));
    const float cosW = std::cos(w0);
    const float sinW = std::sin(w0);
    const float beta = std::sqrt(2.0f * A) * sinW;

    const float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosW + beta);
    const float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW);
    const float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosW - beta);
    const float a0 = (A + 1.0f) - (A - 1.0f) * cosW + beta;
    const float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW);
    const float a2 = (A + 1.0f) - (A - 1.0f) * cosW - beta;

    highFilter.setCoefficients(b0, b1, b2, a0, a1, a2);
}

void ParametricEQ::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!enabled.load(std::memory_order_relaxed))
        return;

    if (needsCoeffUpdate.load(std::memory_order_relaxed))
    {
        updateLowShelf();
        updateMidBand();
        updateHighShelf();
        needsCoeffUpdate.store(false, std::memory_order_relaxed);
    }

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    if (numChannels <= 0)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch, startSample);
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = data[i];
            sample = lowFilter.processSample(sample, ch);
            sample = midFilter.processSample(sample, ch);
            sample = highFilter.processSample(sample, ch);
            data[i] = sample;
        }
    }
}
