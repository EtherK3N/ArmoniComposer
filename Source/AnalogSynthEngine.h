/*
    AnalogSynthEngine.h
    -------------------
    Studio-Grade Virtual Analog Synthesizer for K3N Armoni Composer:
      - Band-Limited PolyBLEP Oscillators (Saw, Pulse/PWM, Triangle, Sine)
      - 4-Pole 24 dB/oct Non-Linear Moog Ladder Resonant Filter with soft saturation
      - True Exponential ADSR Envelope Generator
      - 8-Voice Polyphonic Voice Allocator with voice stealing
      - Real-Time Safe: Zero heap allocations during audio block processing
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>

enum class OscillatorWaveform
{
    Saw = 0,
    Pulse,
    Triangle,
    Sine
};

// ===========================================================================
// 1. PolyBlepOscillator (Band-Limited Virtual Analog Oscillator)
// ===========================================================================
class PolyBlepOscillator
{
public:
    PolyBlepOscillator() = default;

    void prepare(double sampleRate);
    void reset();

    void setFrequency(float freqHz);
    void setWaveform(OscillatorWaveform wave) { waveform = wave; }
    void setPulseWidth(float pw)             { pulseWidth = juce::jlimit(0.05f, 0.95f, pw); }

    float getNextSample();

private:
    static float polyBlep(float t, float dt);

    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float frequencyHz = 440.0f;
    float pulseWidth = 0.5f;
    OscillatorWaveform waveform = OscillatorWaveform::Saw;
};


// ===========================================================================
// 2. MoogLadderFilter (4-Pole 24 dB/oct Resonant Ladder Filter)
// ===========================================================================
class MoogLadderFilter
{
public:
    MoogLadderFilter() = default;

    void prepare(double sampleRate);
    void reset();

    void setCutoff(float hz);
    float getCutoff() const { return cutoffHz; }

    void setResonance(float res);
    float getResonance() const { return resonance; }

    float processSample(float in, int channel);

private:
    double currentSampleRate = 44100.0;
    float cutoffHz = 20000.0f;
    float resonance = 0.0f; // 0.0 to 1.0 (self-oscillation around 0.98+)

    // 4 cascaded one-pole integrator stages per stereo channel
    float stage[2][4] = { {0.0f} };
};


// ===========================================================================
// 3. AdsrEnvelope (Exponential Analog Envelope)
// ===========================================================================
enum class AdsrState
{
    Idle,
    Attack,
    Decay,
    Sustain,
    Release
};

class AdsrEnvelope
{
public:
    AdsrEnvelope() = default;

    void prepare(double sampleRate);
    void setParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec);

    void noteOn();
    void noteOff();
    void reset();

    float getNextSample();
    bool isActive() const { return state != AdsrState::Idle; }
    AdsrState getState() const { return state; }

private:
    double currentSampleRate = 44100.0;
    AdsrState state = AdsrState::Idle;

    float currentLevel = 0.0f;

    float attackRate = 0.001f;
    float decayRate = 0.001f;
    float sustain = 0.7f;
    float releaseRate = 0.001f;
};


// ===========================================================================
// 4. SynthVoice (Single Polyphonic Synth Voice)
// ===========================================================================
class SynthVoice
{
public:
    SynthVoice() = default;

    void prepare(double sampleRate);
    void reset();

    void noteOn(int midiNoteNumber, float velocity);
    void noteOff();

    bool isActive() const { return envelope.isActive(); }
    int getNoteNumber() const { return activeNote; }
    juce::uint32 getAge() const { return noteAge; }
    void incrementAge() { if (isActive()) ++noteAge; }

    void setWaveform(OscillatorWaveform wave) { osc.setWaveform(wave); }
    void setPulseWidth(float pw)             { osc.setPulseWidth(pw); }
    void setFilterCutoff(float hz)           { filter.setCutoff(hz); }
    void setFilterResonance(float res)       { filter.setResonance(res); }
    void setAdsr(float a, float d, float s, float r) { envelope.setParameters(a, d, s, r); }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples, float filterEnvAmount);

private:
    PolyBlepOscillator osc;
    MoogLadderFilter   filter;
    AdsrEnvelope       envelope;

    int activeNote = -1;
    float noteVelocity = 0.0f;
    juce::uint32 noteAge = 0;
    double currentSampleRate = 44100.0;
};


// ===========================================================================
// 5. AnalogSynthEngine (8-Voice Polyphonic Synthesizer)
// ===========================================================================
class AnalogSynthEngine
{
public:
    AnalogSynthEngine();

    void prepare(double sampleRate);
    void reset();

    void noteOn(int midiNoteNumber, float velocity = 1.0f);
    void noteOff(int midiNoteNumber);
    void allNotesOff();

    // Sound Design Parameters (Thread-Safe)
    void setWaveform(OscillatorWaveform wave);
    OscillatorWaveform getWaveform() const { return static_cast<OscillatorWaveform>(waveform.load(std::memory_order_relaxed)); }

    void setPulseWidth(float pw)         { pulseWidth.store(juce::jlimit(0.05f, 0.95f, pw), std::memory_order_relaxed); }
    float getPulseWidth() const          { return pulseWidth.load(std::memory_order_relaxed); }

    void setFilterCutoff(float hz)       { filterCutoff.store(juce::jlimit(20.0f, 20000.0f, hz), std::memory_order_relaxed); }
    float getFilterCutoff() const        { return filterCutoff.load(std::memory_order_relaxed); }

    void setFilterResonance(float res)   { filterResonance.store(juce::jlimit(0.0f, 1.0f, res), std::memory_order_relaxed); }
    float getFilterResonance() const     { return filterResonance.load(std::memory_order_relaxed); }

    void setFilterEnvAmount(float amt)   { filterEnvAmount.store(juce::jlimit(-1.0f, 1.0f, amt), std::memory_order_relaxed); }
    float getFilterEnvAmount() const     { return filterEnvAmount.load(std::memory_order_relaxed); }

    void setAdsr(float a, float d, float s, float r);
    float getAttack() const  { return attackSec.load(std::memory_order_relaxed); }
    float getDecay() const   { return decaySec.load(std::memory_order_relaxed); }
    float getSustain() const { return sustainLvl.load(std::memory_order_relaxed); }
    float getRelease() const { return releaseSec.load(std::memory_order_relaxed); }

    void renderBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    static constexpr int numVoices = 8;

private:
    int findFreeVoice(int midiNoteNumber);

    double currentSampleRate = 44100.0;
    std::array<SynthVoice, numVoices> voices;

    std::atomic<int>   waveform { static_cast<int>(OscillatorWaveform::Saw) };
    std::atomic<float> pulseWidth { 0.5f };
    std::atomic<float> filterCutoff { 2500.0f };
    std::atomic<float> filterResonance { 0.4f };
    std::atomic<float> filterEnvAmount { 0.5f };

    std::atomic<float> attackSec { 0.01f };
    std::atomic<float> decaySec  { 0.25f };
    std::atomic<float> sustainLvl { 0.6f };
    std::atomic<float> releaseSec { 0.2f };
};
