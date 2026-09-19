/*
    MidiSyncEngine.cpp
    ------------------
    Implementation of real-time 24 PPQN MIDI Clock generator.
*/

#include "MidiSyncEngine.h"

MidiSyncEngine::MidiSyncEngine() = default;

MidiSyncEngine::~MidiSyncEngine()
{
    closeMidiOutput();
}

void MidiSyncEngine::prepare(double sampleRate, double bpm)
{
    currentSampleRate.store(sampleRate > 0.0 ? sampleRate : 44100.0, std::memory_order_relaxed);
    currentBpm.store(bpm > 0.0 ? bpm : 120.0, std::memory_order_relaxed);
    sampleCounter = 0.0;
    clockPulseCounter.store(0, std::memory_order_relaxed);
    recalculateSamplesPerClock();
}

void MidiSyncEngine::setTempo(double bpm)
{
    if (bpm > 0.0)
    {
        currentBpm.store(bpm, std::memory_order_relaxed);
        recalculateSamplesPerClock();
    }
}

void MidiSyncEngine::recalculateSamplesPerClock()
{
    const double sRate = currentSampleRate.load(std::memory_order_relaxed);
    const double bpm = currentBpm.load(std::memory_order_relaxed);
    const double spc = (sRate * 60.0) / (bpm * 24.0);
    samplesPerClock.store(spc, std::memory_order_relaxed);
}

void MidiSyncEngine::sendStart()
{
    sampleCounter = 0.0;
    clockPulseCounter.store(0, std::memory_order_relaxed);
    isRunning.store(true, std::memory_order_relaxed);

    if (clockEnabled.load(std::memory_order_relaxed))
        sendMessage(juce::MidiMessage(0xFA)); // MIDI Start
}

void MidiSyncEngine::sendStop()
{
    isRunning.store(false, std::memory_order_relaxed);

    if (clockEnabled.load(std::memory_order_relaxed))
        sendMessage(juce::MidiMessage(0xFC)); // MIDI Stop
}

void MidiSyncEngine::sendContinue()
{
    isRunning.store(true, std::memory_order_relaxed);

    if (clockEnabled.load(std::memory_order_relaxed))
        sendMessage(juce::MidiMessage(0xFB)); // MIDI Continue
}

bool MidiSyncEngine::openMidiOutput(int deviceIndex)
{
    const juce::ScopedLock sl(midiLock);
    closeMidiOutput();

    const auto devices = juce::MidiOutput::getAvailableDevices();
    if (deviceIndex >= 0 && deviceIndex < devices.size())
    {
        midiOutput = juce::MidiOutput::openDevice(devices[deviceIndex].identifier);
        return midiOutput != nullptr;
    }
    return false;
}

void MidiSyncEngine::closeMidiOutput()
{
    const juce::ScopedLock sl(midiLock);
    if (midiOutput != nullptr)
    {
        if (isRunning.load(std::memory_order_relaxed))
            midiOutput->sendMessageNow(juce::MidiMessage(0xFC));
        midiOutput.reset();
    }
}

juce::StringArray MidiSyncEngine::getAvailableMidiOutputs() const
{
    juce::StringArray names;
    const auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& dev : devices)
        names.add(dev.name);
    return names;
}

juce::String MidiSyncEngine::getActiveOutputDeviceName() const
{
    const juce::ScopedLock sl(midiLock);
    if (midiOutput != nullptr)
        return midiOutput->getName();
    return {};
}

void MidiSyncEngine::sendMessage(const juce::MidiMessage& message)
{
    const juce::ScopedLock sl(midiLock);
    if (midiOutput != nullptr)
        midiOutput->sendMessageNow(message);
}
