/*
    DspFxRack.cpp
    -------------
    Implementation of the real-time DSP Sound Sculpting Rack:
      - ResonantFilter (Andy Simper SVF)
      - StereoDelay (Tempo-synced Ping-Pong with feedback HF damping)
      - AlgorithmicReverb (Schroeder / Freeverb core)
      - DspFxRack (Master / Lane FX Chain)
*/

#include "DspFxRack.h"

// ===========================================================================
// 1. ResonantFilter (State Variable Filter)
// ===========================================================================
ResonantFilter::ResonantFilter() = default;

void ResonantFilter::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    reset();
}

void ResonantFilter::reset()
{
    s1[0] = s1[1] = 0.0f;
    s2[0] = s2[1] = 0.0f;
}

void ResonantFilter::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (! enabled.load(std::memory_order_relaxed))
        return;

    const auto currentMode = getMode();
    const float fc = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.49), getCutoff());
    const float q = getResonance();

    // Andy Simper Linear SVF coefficients
    const float g = std::tan(juce::MathConstants<float>::pi * fc / static_cast<float>(currentSampleRate));
    const float k = 1.0f / q;
    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;
    const float a3 = g * a2;

    const int numChannels = juce::jmin(2, buffer.getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch, startSample);
        float s1_val = s1[ch];
        float s2_val = s2[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = channelData[i];
            const float v3 = x - s2_val;
            const float v1 = a1 * s1_val + a2 * v3;
            const float v2 = s2_val + a2 * s1_val + a3 * v3;

            s1_val = 2.0f * v1 - s1_val;
            s2_val = 2.0f * v2 - s2_val;

            // Denormal prevention
            if (std::abs(s1_val) < 1.0e-15f) s1_val = 0.0f;
            if (std::abs(s2_val) < 1.0e-15f) s2_val = 0.0f;

            switch (currentMode)
            {
                case FilterMode::LowPass:
                    channelData[i] = v2;
                    break;
                case FilterMode::BandPass:
                    channelData[i] = v1;
                    break;
                case FilterMode::HighPass:
                    channelData[i] = x - k * v1 - v2;
                    break;
            }
        }

        s1[ch] = s1_val;
        s2[ch] = s2_val;
    }
}


// ===========================================================================
// 2. StereoDelay (Tempo-Synced Stereo Ping-Pong Delay)
// ===========================================================================
StereoDelay::StereoDelay() = default;

void StereoDelay::prepare(double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    bufferSize = static_cast<int>(currentSampleRate * maxDelaySeconds);

    delayBufferLeft.assign(static_cast<size_t>(bufferSize), 0.0f);
    delayBufferRight.assign(static_cast<size_t>(bufferSize), 0.0f);
    reset();
}

void StereoDelay::reset()
{
    if (bufferSize > 0)
    {
        std::fill(delayBufferLeft.begin(), delayBufferLeft.end(), 0.0f);
        std::fill(delayBufferRight.begin(), delayBufferRight.end(), 0.0f);
    }
    writeIndex = 0;
    lastFilteredFeedback[0] = 0.0f;
    lastFilteredFeedback[1] = 0.0f;
}

void StereoDelay::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double currentBpm)
{
    const float wetMix = getMix();
    if (! enabled.load(std::memory_order_relaxed) || wetMix <= 0.0001f || bufferSize <= 0)
        return;

    const double bpm = (currentBpm >= 40.0 && currentBpm <= 300.0) ? currentBpm : 120.0;
    const double beatDurationSec = 60.0 / bpm;

    double divisionBeats = 0.5; // default eighth
    switch (getDivision())
    {
        case DelayDivision::Quarter:          divisionBeats = 1.0; break;
        case DelayDivision::Eighth:           divisionBeats = 0.5; break;
        case DelayDivision::EighthDotted:     divisionBeats = 0.75; break;
        case DelayDivision::Sixteenth:        divisionBeats = 0.25; break;
        case DelayDivision::SixteenthTriplet: divisionBeats = 0.25 * (2.0 / 3.0); break;
    }

    const int delaySamples = juce::jlimit(1, bufferSize - 1, static_cast<int>(divisionBeats * beatDurationSec * currentSampleRate));
    const float fb = getFeedback();
    const float damp = getDamping();
    const bool isPP = isPingPong();
    const float dryMix = 1.0f - wetMix;

    auto* leftChannel = buffer.getWritePointer(0, startSample);
    auto* rightChannel = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1, startSample) : leftChannel;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = leftChannel[i];
        const float inR = rightChannel[i];

        // Read delayed signals from circular buffer
        int readIdx = writeIndex - delaySamples;
        if (readIdx < 0) readIdx += bufferSize;

        const float delayedL = delayBufferLeft[static_cast<size_t>(readIdx)];
        const float delayedR = delayBufferRight[static_cast<size_t>(readIdx)];

        // Feedback with one-pole high-frequency damping
        float fbL = isPP ? delayedR * fb : delayedL * fb;
        float fbR = isPP ? delayedL * fb : delayedR * fb;

        lastFilteredFeedback[0] = (1.0f - damp) * fbL + damp * lastFilteredFeedback[0];
        lastFilteredFeedback[1] = (1.0f - damp) * fbR + damp * lastFilteredFeedback[1];

        // Soft saturation in feedback loop to prevent explosive runaway
        const float feedbackInL = std::tanh(inL + lastFilteredFeedback[0]);
        const float feedbackInR = std::tanh(inR + lastFilteredFeedback[1]);

        delayBufferLeft[static_cast<size_t>(writeIndex)] = feedbackInL;
        delayBufferRight[static_cast<size_t>(writeIndex)] = feedbackInR;

        // Output mix
        leftChannel[i] = dryMix * inL + wetMix * delayedL;
        rightChannel[i] = dryMix * inR + wetMix * delayedR;

        writeIndex = (writeIndex + 1) % bufferSize;
    }
}


// ===========================================================================
// 3. AlgorithmicReverb (Schroeder / Freeverb Core)
// ===========================================================================
namespace
{
    // Tuning at 44.1kHz
    constexpr int defaultCombTunings[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    constexpr int defaultAllPassTunings[4] = { 556, 441, 341, 225 };
    constexpr int stereoSpread = 23;
}

AlgorithmicReverb::AlgorithmicReverb() = default;

void AlgorithmicReverb::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    const double srScale = currentSampleRate / 44100.0;

    for (int i = 0; i < numCombs; ++i)
    {
        const int lenL = static_cast<int>(defaultCombTunings[i] * srScale);
        const int lenR = static_cast<int>((defaultCombTunings[i] + stereoSpread) * srScale);

        combsL[static_cast<size_t>(i)].buffer.assign(static_cast<size_t>(lenL), 0.0f);
        combsL[static_cast<size_t>(i)].bufIndex = 0;
        combsL[static_cast<size_t>(i)].filterStore = 0.0f;

        combsR[static_cast<size_t>(i)].buffer.assign(static_cast<size_t>(lenR), 0.0f);
        combsR[static_cast<size_t>(i)].bufIndex = 0;
        combsR[static_cast<size_t>(i)].filterStore = 0.0f;
    }

    for (int i = 0; i < numAllPasses; ++i)
    {
        const int lenL = static_cast<int>(defaultAllPassTunings[i] * srScale);
        const int lenR = static_cast<int>((defaultAllPassTunings[i] + stereoSpread) * srScale);

        allPassesL[static_cast<size_t>(i)].buffer.assign(static_cast<size_t>(lenL), 0.0f);
        allPassesL[static_cast<size_t>(i)].bufIndex = 0;

        allPassesR[static_cast<size_t>(i)].buffer.assign(static_cast<size_t>(lenR), 0.0f);
        allPassesR[static_cast<size_t>(i)].bufIndex = 0;
    }
}

void AlgorithmicReverb::reset()
{
    for (auto& c : combsL)
    {
        std::fill(c.buffer.begin(), c.buffer.end(), 0.0f);
        c.bufIndex = 0;
        c.filterStore = 0.0f;
    }
    for (auto& c : combsR)
    {
        std::fill(c.buffer.begin(), c.buffer.end(), 0.0f);
        c.bufIndex = 0;
        c.filterStore = 0.0f;
    }
    for (auto& a : allPassesL)
    {
        std::fill(a.buffer.begin(), a.buffer.end(), 0.0f);
        a.bufIndex = 0;
    }
    for (auto& a : allPassesR)
    {
        std::fill(a.buffer.begin(), a.buffer.end(), 0.0f);
        a.bufIndex = 0;
    }
}

void AlgorithmicReverb::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float wetMix = getMix();
    if (! enabled.load(std::memory_order_relaxed) || wetMix <= 0.0001f)
        return;

    const float room = getRoomSize() * 0.28f + 0.70f;
    const float damp = getDamping() * 0.4f;
    const float w = getWidth();
    const float wet1 = wetMix * (w / 2.0f + 0.5f);
    const float wet2 = wetMix * ((1.0f - w) / 2.0f);
    const float dry = 1.0f - wetMix;

    auto* leftChannel = buffer.getWritePointer(0, startSample);
    auto* rightChannel = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1, startSample) : leftChannel;

    for (int i = 0; i < numSamples; ++i)
    {
        const float in = (leftChannel[i] + rightChannel[i]) * 0.015f; // gain trim
        float outL = 0.0f;
        float outR = 0.0f;

        // 8 Parallel Comb Filters
        for (int c = 0; c < numCombs; ++c)
        {
            auto& combL = combsL[static_cast<size_t>(c)];
            const float bufOutL = combL.buffer[static_cast<size_t>(combL.bufIndex)];
            combL.filterStore = bufOutL * (1.0f - damp) + combL.filterStore * damp;
            combL.buffer[static_cast<size_t>(combL.bufIndex)] = in + combL.filterStore * room;
            if (++combL.bufIndex >= static_cast<int>(combL.buffer.size())) combL.bufIndex = 0;
            outL += bufOutL;

            auto& combR = combsR[static_cast<size_t>(c)];
            const float bufOutR = combR.buffer[static_cast<size_t>(combR.bufIndex)];
            combR.filterStore = bufOutR * (1.0f - damp) + combR.filterStore * damp;
            combR.buffer[static_cast<size_t>(combR.bufIndex)] = in + combR.filterStore * room;
            if (++combR.bufIndex >= static_cast<int>(combR.buffer.size())) combR.bufIndex = 0;
            outR += bufOutR;
        }

        // 4 Series All-Pass Diffusers
        constexpr float allPassFeedback = 0.5f;
        for (int a = 0; a < numAllPasses; ++a)
        {
            auto& apL = allPassesL[static_cast<size_t>(a)];
            const float bufOutL = apL.buffer[static_cast<size_t>(apL.bufIndex)];
            apL.buffer[static_cast<size_t>(apL.bufIndex)] = outL + bufOutL * allPassFeedback;
            outL = -outL + bufOutL;
            if (++apL.bufIndex >= static_cast<int>(apL.buffer.size())) apL.bufIndex = 0;

            auto& apR = allPassesR[static_cast<size_t>(a)];
            const float bufOutR = apR.buffer[static_cast<size_t>(apR.bufIndex)];
            apR.buffer[static_cast<size_t>(apR.bufIndex)] = outR + bufOutR * allPassFeedback;
            outR = -outR + bufOutR;
            if (++apR.bufIndex >= static_cast<int>(apR.buffer.size())) apR.bufIndex = 0;
        }

        const float inL = leftChannel[i];
        const float inR = rightChannel[i];

        leftChannel[i]  = inL * dry + outL * wet1 + outR * wet2;
        rightChannel[i] = inR * dry + outR * wet1 + outL * wet2;
    }
}


// ===========================================================================
// 4. DspFxRack (Master / Lane FX Chain)
// ===========================================================================
DspFxRack::DspFxRack() = default;

void DspFxRack::prepare(double sampleRate, int maxBlockSize)
{
    filter.prepare(sampleRate);
    delay.prepare(sampleRate, maxBlockSize);
    reverb.prepare(sampleRate);
}

void DspFxRack::reset()
{
    filter.reset();
    delay.reset();
    reverb.reset();
}

void DspFxRack::processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, double currentBpm)
{
    if (masterBypass.load(std::memory_order_relaxed))
        return;

    // Signal Flow: Filter -> Delay -> Reverb
    filter.processBlock(buffer, startSample, numSamples);
    delay.processBlock(buffer, startSample, numSamples, currentBpm);
    reverb.processBlock(buffer, startSample, numSamples);
}
