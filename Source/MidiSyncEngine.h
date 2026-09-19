/*
    MidiSyncEngine.h
    ----------------
    Real-time MIDI Clock synchronization engine for K3N Armoni Composer:
      - 24 PPQN (Pulses Per Quarter Note) standard MIDI 1.0 clock generator
      - Start (0xFA), Continue (0xFB), Stop (0xFC), and Timing Clock (0xF8) support
      - Synchronized to MetronomeClock audio-sample timeline
      - Hardware MIDI Output & Virtual Port integration (Ableton, FL Studio, Reaper, hardware synths)
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>
#include <atomic>

class MidiSyncEngine
{
public:
    MidiSyncEngine();
    ~MidiSyncEngine();

    /** Prepares sync engine with current audio sample rate and tempo. */
    void prepare(double sampleRate, double bpm);

    /** Updates tempo in beats per minute. */
    void setTempo(double bpm);

    /** Enables or disables MIDI clock transmission. */
    void setClockEnabled(bool shouldBeEnabled) { clockEnabled.store(shouldBeEnabled, std::memory_order_relaxed); }
    bool isClockEnabled() const { return clockEnabled.load(std::memory_order_relaxed); }

    /** Sends MIDI Start (0xFA) and resets clock pulse phase. */
    void sendStart();

    /** Sends MIDI Stop (0xFC). */
    void sendStop();

    /** Sends MIDI Continue (0xFB). */
    void sendContinue();

    /** Processes an audio block and generates MIDI clock messages if needed. */
    template <typename Callback>
    void processBlock(int numSamples, Callback&& clockCallback)
    {
        if (! clockEnabled.load(std::memory_order_relaxed) || ! isRunning.load(std::memory_order_relaxed))
            return;

        const double spc = samplesPerClock.load(std::memory_order_relaxed);
        if (spc <= 0.0)
            return;

        sampleCounter += numSamples;
        while (sampleCounter >= spc)
        {
            sampleCounter -= spc;
            clockPulseCounter++;
            clockCallback(juce::MidiMessage(0xF8)); // MIDI Timing Clock
        }
    }

    /** Opens a specific MIDI output device for direct transmission. */
    bool openMidiOutput(int deviceIndex);
    void closeMidiOutput();
    juce::StringArray getAvailableMidiOutputs() const;
    juce::String getActiveOutputDeviceName() const;

    /** Sends a message to the active MIDI output device, if opened. */
    void sendMessage(const juce::MidiMessage& message);

    juce::int64 getClockPulseCount() const { return clockPulseCounter.load(std::memory_order_relaxed); }
    void resetClockPulseCount() { clockPulseCounter.store(0, std::memory_order_relaxed); }

private:
    void recalculateSamplesPerClock();

    std::atomic<bool> clockEnabled { true };
    std::atomic<bool> isRunning { false };
    std::atomic<double> currentBpm { 120.0 };
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<double> samplesPerClock { 44100.0 * 60.0 / (120.0 * 24.0) };
    std::atomic<juce::int64> clockPulseCounter { 0 };

    double sampleCounter = 0.0;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::CriticalSection midiLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSyncEngine)
};
