/*
    AudioEngine.h
    -------------
    Multichannel real-time JUCE audio engine:
      - 32-voice polyphony with low-overhead voice stealing
      - Pre-cached RAM sample pool (zero memory allocations or disk I/O in the audio thread)
      - Synchronous sample-accurate multi-track Looper (LoopTrack)
      - Built-in Procedural Drum Synthesizer (Instant Out-Of-The-Box Playability with zero external files)
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <unordered_map>
#include <string>
#include <algorithm>
#include "MetronomeClock.h"
#include "BpmQuantizer.h"
#include "DspFxRack.h"
#include "MidiSyncEngine.h"

class AudioEngine;

/** A single sampler voice (one-shot or looper trigger playback). */
struct SamplerVoice
{
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    int position = 0;
    float gain = 1.0f;
    bool isActive = false;
};

/** A discrete trigger event recorded in a looper lane with micro-timing, gate duration, and per-hit parameter locks. */
struct TriggerEvent
{
    int sampleHandle = -1;
    juce::int64 offsetSamples = 0;
    juce::int64 durationSamples = 0; // 0 = default one-shot
    float gain = 1.0f;               // Velocity / dynamic accent (0.0 to 2.0)
    float pitchSemitones = 0.0f;    // Per-hit transposition (-24 to +24)
    float reverbSend = 0.0f;         // Per-hit reverb burst (0.0 to 1.0)
    float delaySend = 0.0f;          // Per-hit delay throw (0.0 to 1.0)
    float filterCutoff = 1.0f;       // Per-hit filter cutoff (0.0 to 1.0)
};

/** Multi-track looper lane synchronized directly to audio samples. */
class LoopTrack
{
public:
    LoopTrack();

    void startRecording();
    /** Stops recording without quantization. */
    void stopRecordingAndStartLoop();
    /** Stops recording and snaps the loop length to the nearest bar boundary. */
    void stopRecordingAndStartLoop(const BpmQuantizer& quantizer, int beatsPerBar = 4);
    void clear();


    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    bool isMuted() const { return muted; }
    bool isEmpty() const { return recordedEvents.isEmpty(); }
    bool isRecording() const { return recording; }

    /** Records a trigger event, applying BPM quantization if a quantizer is provided. */
    void recordTrigger(int sampleHandle, juce::int64 offsetInLoopSamples,
                       const BpmQuantizer* quantizer = nullptr);

    /** Micro-timing: moves an event to a new offset, keeping events sorted. */
    void moveEvent(int eventIndex, juce::int64 newOffsetSamples);

    /** Sets the gate duration of a specific event. */
    void setEventDuration(int eventIndex, juce::int64 durationSamples);

    /** Updates per-hit parameter locks (gain, pitch, reverb, delay, filter). */
    void setEventParams(int eventIndex, float gain, float pitchSemitones = 0.0f,
                        float reverbSend = 0.0f, float delaySend = 0.0f, float filterCutoff = 1.0f);

    /** Removes a specific event by index. */
    void removeEvent(int eventIndex);

    /** Duplicates an event, placing the copy at offset + duplicateOffsetSamples (default 1/16). */
    void duplicateEvent(int eventIndex, juce::int64 duplicateOffsetSamples = 0);

    juce::int64 getLoopLengthSamples() const { return loopLengthSamples; }

    /** Advances playback position for the current audio block and emits triggers. */
    void processAudioBlock(int numSamples, AudioEngine& engine);

    juce::Array<TriggerEvent> recordedEvents;

private:
    void sortEvents();

    bool muted = false;
    bool recording = false;
    juce::int64 loopLengthSamples = 0;
    juce::int64 playbackPositionSamples = 0;
    int nextEventIndex = 0;
};

class AudioEngine : public juce::AudioSource
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // juce::AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    /** Loads an audio file into RAM cache. Returns cached handle if already loaded. */
    int loadSample(const juce::File& audioFile);

    /** Adds a pre-generated AudioBuffer directly into the memory cache. */
    int addMemoryBuffer(std::unique_ptr<juce::AudioBuffer<float>> buffer, const juce::String& identifier);

    /** Generates built-in procedural electronic drumkit (Kick, Snare, Hi-Hat, 808 Bass, Synth Lead). */
    void generateDefaultStarterKit();

    /** Finds cached sample handle for an absolute path or memory identifier. Returns -1 if not found. */
    int findSampleHandle(const juce::String& identifier) const;

    /** Returns pointer to cached sample buffer, or nullptr if invalid handle. */
    const juce::AudioBuffer<float>* getSampleBuffer(int handle) const;

    /** Returns cached identifier/path for sample handle. */
    juce::String getSampleIdentifier(int handle) const;

    /** Returns total number of cached sample buffers. */
    int getNumLoadedSamples() const;

    /** Instant real-time safe sample triggering. */
    void triggerSample(int sampleHandle, float gain = 1.0f);

    /** Registers a loop track for synchronous audio block processing. */
    void registerLoopTrack(LoopTrack* track);
    void unregisterLoopTrack(LoopTrack* track);

    double getSampleRate() const { return currentSampleRate; }

    // BPM & Clock access
    MetronomeClock& getMetronome() { return metronome; }
    BpmQuantizer&   getQuantizer() { return quantizer; }
    DspFxRack&      getMasterFxRack() { return masterFxRack; }
    MidiSyncEngine& getMidiSyncEngine() { return midiSync; }
    const MidiSyncEngine& getMidiSyncEngine() const { return midiSync; }
    void setTempo(double bpm);

    static constexpr int maxVoices = 32;


private:
    juce::AudioFormatManager formatManager;
    juce::OwnedArray<juce::AudioBuffer<float>> loadedSamples;
    std::unordered_map<std::string, int> samplePathToHandle;
    juce::StringArray sampleIdentifiers;

    std::array<SamplerVoice, maxVoices> voices;
    juce::Array<LoopTrack*> activeLoopTracks;

    MetronomeClock metronome;
    BpmQuantizer   quantizer;
    DspFxRack      masterFxRack;
    MidiSyncEngine midiSync;

    // Procedural click buffers for the metronome (generated once)
    std::unique_ptr<juce::AudioBuffer<float>> clickDownbeatBuffer;
    std::unique_ptr<juce::AudioBuffer<float>> clickBeatBuffer;
    int clickDownbeatVoice = -1; // voice index reserved for click

    double currentSampleRate = 44100.0;
    juce::CriticalSection audioLock;

    void generateClickBuffers();


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
