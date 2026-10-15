/*
    AnalogSynthEngine.cpp
    ---------------------
    Implementation of Virtual Analog Synthesis for K3N Armoni Composer.
*/

#include "AnalogSynthEngine.h"
#include <algorithm>

// ===========================================================================
// PolyBlepOscillator
// ===========================================================================
void PolyBlepOscillator::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    reset();
}

void PolyBlepOscillator::reset()
{
    phase = 0.0f;
    phaseIncrement = frequencyHz / static_cast<float>(currentSampleRate);
}

void PolyBlepOscillator::setFrequency(float freqHz)
{
    frequencyHz = juce::jlimit(10.0f, static_cast<float>(currentSampleRate * 0.45), freqHz);
    phaseIncrement = frequencyHz / static_cast<float>(currentSampleRate);
}

float PolyBlepOscillator::polyBlep(float t, float dt)
{
    if (dt <= 0.0f)
        return 0.0f;

    // 0 <= t < 1
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float PolyBlepOscillator::getNextSample()
{
    float sample = 0.0f;
    const float dt = phaseIncrement;

    switch (waveform)
    {
        case OscillatorWaveform::Saw:
        {
            // Naive Saw: 2*phase - 1
            sample = (2.0f * phase) - 1.0f;
            // Subtract PolyBLEP correction at phase transition (t = 0)
            sample -= polyBlep(phase, dt);
            break;
        }

        case OscillatorWaveform::Pulse:
        {
            // Naive Pulse
            sample = (phase < pulseWidth) ? 1.0f : -1.0f;
            // Add correction at phase = 0, subtract at phase = pulseWidth
            sample += polyBlep(phase, dt);
            float phasePw = phase - pulseWidth;
            if (phasePw < 0.0f) phasePw += 1.0f;
            sample -= polyBlep(phasePw, dt);
            break;
        }

        case OscillatorWaveform::Triangle:
        {
            // Derived from naive triangle with integration or smoothed fold
            sample = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
        }

        case OscillatorWaveform::Sine:
        default:
        {
            sample = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            break;
        }
    }

    phase += phaseIncrement;
    while (phase >= 1.0f)
        phase -= 1.0f;

    return sample;
}


// ===========================================================================
// MoogLadderFilter (Huovilainen 4-Pole Non-Linear Saturated Model)
// ===========================================================================
void MoogLadderFilter::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    reset();
}

void MoogLadderFilter::reset()
{
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4; ++i)
            stage[ch][i] = 0.0f;
}

void MoogLadderFilter::setCutoff(float hz)
{
    cutoffHz = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), hz);
}

void MoogLadderFilter::setResonance(float res)
{
    resonance = juce::jlimit(0.0f, 1.0f, res);
}

float MoogLadderFilter::processSample(float in, int channel)
{
    const int ch = std::min(channel, 1);

    // Normalized frequency tuned for 44.1k / 48k / 96k
    const float fc = cutoffHz / static_cast<float>(currentSampleRate);
    const float f = juce::jlimit(0.001f, 0.49f, fc);
    const float w = 2.0f * juce::MathConstants<float>::pi * f;
    const float g = 1.0f - std::exp(-w);
    const float k = resonance * 3.96f; // Self-oscillates near 1.0

    // Non-linear feedback loop with soft saturation
    const float feedback = k * stage[ch][3];
    const float x = in - feedback;
    const float satX = std::tanh(x * 0.65f) * 1.538f;

    stage[ch][0] += g * (std::tanh(satX) - stage[ch][0]);
    stage[ch][1] += g * (stage[ch][0] - stage[ch][1]);
    stage[ch][2] += g * (stage[ch][1] - stage[ch][2]);
    stage[ch][3] += g * (stage[ch][2] - stage[ch][3]);

    return stage[ch][3];
}


// ===========================================================================
// AdsrEnvelope
// ===========================================================================
void AdsrEnvelope::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    reset();
}

void AdsrEnvelope::setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec)
{
    const float safeAttack  = std::max(0.001f, attackSec);
    const float safeDecay   = std::max(0.001f, decaySec);
    const float safeRelease = std::max(0.001f, releaseSec);

    attackRate  = 1.0f / (safeAttack * static_cast<float>(currentSampleRate));
    decayRate   = 1.0f / (safeDecay * static_cast<float>(currentSampleRate));
    sustain     = juce::jlimit(0.0f, 1.0f, sustainLevel);
    releaseRate = 1.0f / (safeRelease * static_cast<float>(currentSampleRate));
}

void AdsrEnvelope::noteOn()
{
    state = AdsrState::Attack;
}

void AdsrEnvelope::noteOff()
{
    if (state != AdsrState::Idle)
        state = AdsrState::Release;
}

void AdsrEnvelope::reset()
{
    state = AdsrState::Idle;
    currentLevel = 0.0f;
}

float AdsrEnvelope::getNextSample()
{
    switch (state)
    {
        case AdsrState::Attack:
            currentLevel += attackRate;
            if (currentLevel >= 1.0f)
            {
                currentLevel = 1.0f;
                state = AdsrState::Decay;
            }
            break;

        case AdsrState::Decay:
            currentLevel -= decayRate;
            if (currentLevel <= sustain)
            {
                currentLevel = sustain;
                state = AdsrState::Sustain;
            }
            break;

        case AdsrState::Sustain:
            currentLevel = sustain;
            break;

        case AdsrState::Release:
            currentLevel -= releaseRate;
            if (currentLevel <= 0.0f)
            {
                currentLevel = 0.0f;
                state = AdsrState::Idle;
            }
            break;

        case AdsrState::Idle:
        default:
            currentLevel = 0.0f;
            break;
    }

    return currentLevel;
}


// ===========================================================================
// SynthVoice
// ===========================================================================
void SynthVoice::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    osc.prepare(sampleRate);
    filter.prepare(sampleRate);
    envelope.prepare(sampleRate);
    reset();
}

void SynthVoice::reset()
{
    activeNote = -1;
    noteVelocity = 0.0f;
    noteAge = 0;
    osc.reset();
    filter.reset();
    envelope.reset();
}

void SynthVoice::noteOn(int midiNoteNumber, float velocity)
{
    activeNote = midiNoteNumber;
    noteVelocity = juce::jlimit(0.0f, 1.0f, velocity);
    noteAge = 0;

    // Convert MIDI note number to frequency (A4 = 440 Hz = note 69)
    const float freq = 440.0f * std::pow(2.0f, (midiNoteNumber - 69.0f) / 12.0f);
    osc.setFrequency(freq);

    envelope.noteOn();
}

void SynthVoice::noteOff()
{
    envelope.noteOff();
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, float filterEnvAmount)
{
    if (!envelope.isActive())
    {
        activeNote = -1;
        return;
    }

    const int numChannels = std::min(outputBuffer.getNumChannels(), 2);
    const float baseCutoff = filter.getCutoff();

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = envelope.getNextSample();
        if (envelope.getState() == AdsrState::Idle && env <= 0.0f)
        {
            activeNote = -1;
            break;
        }

        // Modulate filter cutoff with envelope
        const float modCutoff = juce::jlimit(20.0f, 20000.0f, baseCutoff + (filterEnvAmount * 8000.0f * env));
        filter.setCutoff(modCutoff);

        // Generate oscillator sample and filter through 4-pole Moog ladder
        const float rawSample = osc.getNextSample();
        const float filteredL = filter.processSample(rawSample, 0);
        const float filteredR = (numChannels > 1) ? filter.processSample(rawSample, 1) : filteredL;

        const float gain = env * noteVelocity * 0.4f;

        outputBuffer.addSample(0, startSample + i, filteredL * gain);
        if (numChannels > 1)
            outputBuffer.addSample(1, startSample + i, filteredR * gain);
    }
}


// ===========================================================================
// AnalogSynthEngine
// ===========================================================================
AnalogSynthEngine::AnalogSynthEngine()
{
    setAdsr(0.01f, 0.25f, 0.6f, 0.2f);
}

void AnalogSynthEngine::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    for (auto& voice : voices)
        voice.prepare(sampleRate);
}

void AnalogSynthEngine::reset()
{
    for (auto& voice : voices)
        voice.reset();
}

void AnalogSynthEngine::setWaveform(OscillatorWaveform wave)
{
    waveform.store(static_cast<int>(wave), std::memory_order_relaxed);
    for (auto& voice : voices)
        voice.setWaveform(wave);
}

void AnalogSynthEngine::setAdsr(float a, float d, float s, float r)
{
    attackSec.store(a, std::memory_order_relaxed);
    decaySec.store(d, std::memory_order_relaxed);
    sustainLvl.store(s, std::memory_order_relaxed);
    releaseSec.store(r, std::memory_order_relaxed);

    for (auto& voice : voices)
        voice.setAdsr(a, d, s, r);
}

int AnalogSynthEngine::findFreeVoice(int midiNoteNumber)
{
    // 1. If voice already playing this exact note, retrigger it
    for (int i = 0; i < numVoices; ++i)
    {
        if (voices[static_cast<size_t>(i)].getNoteNumber() == midiNoteNumber)
            return i;
    }

    // 2. Find an idle voice
    for (int i = 0; i < numVoices; ++i)
    {
        if (!voices[static_cast<size_t>(i)].isActive())
            return i;
    }

    // 3. Voice stealing: steal the oldest voice
    int oldestIndex = 0;
    juce::uint32 maxAge = 0;
    for (int i = 0; i < numVoices; ++i)
    {
        if (voices[static_cast<size_t>(i)].getAge() >= maxAge)
        {
            maxAge = voices[static_cast<size_t>(i)].getAge();
            oldestIndex = i;
        }
    }
    return oldestIndex;
}

void AnalogSynthEngine::noteOn(int midiNoteNumber, float velocity)
{
    const int voiceIdx = findFreeVoice(midiNoteNumber);
    auto& voice = voices[static_cast<size_t>(voiceIdx)];

    voice.setWaveform(getWaveform());
    voice.setPulseWidth(getPulseWidth());
    voice.setFilterCutoff(getFilterCutoff());
    voice.setFilterResonance(getFilterResonance());
    voice.setAdsr(getAttack(), getDecay(), getSustain(), getRelease());

    voice.noteOn(midiNoteNumber, velocity);
}

void AnalogSynthEngine::noteOff(int midiNoteNumber)
{
    for (auto& voice : voices)
    {
        if (voice.getNoteNumber() == midiNoteNumber)
            voice.noteOff();
    }
}

void AnalogSynthEngine::allNotesOff()
{
    for (auto& voice : voices)
        voice.noteOff();
}

void AnalogSynthEngine::renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float envAmt = filterEnvAmount.load(std::memory_order_relaxed);
    const float curCutoff = filterCutoff.load(std::memory_order_relaxed);
    const float curRes = filterResonance.load(std::memory_order_relaxed);
    const float curPw = pulseWidth.load(std::memory_order_relaxed);

    for (auto& voice : voices)
    {
        if (voice.isActive())
        {
            voice.setFilterCutoff(curCutoff);
            voice.setFilterResonance(curRes);
            voice.setPulseWidth(curPw);
            voice.renderNextBlock(buffer, startSample, numSamples, envAmt);
            voice.incrementAge();
        }
    }
}
