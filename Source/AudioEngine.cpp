#include "AudioEngine.h"

//==============================================================================
// LoopTrack

LoopTrack::LoopTrack() = default;

void LoopTrack::startRecording()
{
    recordedEvents.clear();
    recording = true;
    loopLengthSamples = 0;
    playbackPositionSamples = 0;
    nextEventIndex = 0;
}

void LoopTrack::sortEvents()
{
    std::sort(recordedEvents.begin(), recordedEvents.end(),
              [](const TriggerEvent& a, const TriggerEvent& b)
              {
                  return a.offsetSamples < b.offsetSamples;
              });
}

void LoopTrack::stopRecordingAndStartLoop()
{
    recording = false;
    sortEvents();
    playbackPositionSamples = 0;
    nextEventIndex = 0;
}

void LoopTrack::stopRecordingAndStartLoop(const BpmQuantizer& quantizer, int beatsPerBar)
{
    recording = false;
    if (loopLengthSamples > 0)
        loopLengthSamples = quantizer.snapLoopLengthToNearestBar(loopLengthSamples, beatsPerBar);
    sortEvents();
    playbackPositionSamples = 0;
    nextEventIndex = 0;
}

void LoopTrack::clear()
{
    recordedEvents.clear();
    recording = false;
    loopLengthSamples = 0;
    playbackPositionSamples = 0;
    nextEventIndex = 0;
}

void LoopTrack::recordTrigger(int sampleHandle, juce::int64 offsetInLoopSamples,
                               const BpmQuantizer* quantizer)
{
    if (! recording)
        return;

    TriggerEvent ev;
    ev.sampleHandle  = sampleHandle;
    ev.offsetSamples = (quantizer != nullptr)
                       ? quantizer->quantize(offsetInLoopSamples, loopLengthSamples)
                       : offsetInLoopSamples;

    recordedEvents.add(ev);
    loopLengthSamples = juce::jmax(loopLengthSamples, ev.offsetSamples + 1);
}

void LoopTrack::moveEvent(int eventIndex, juce::int64 newOffsetSamples)
{
    if (eventIndex < 0 || eventIndex >= recordedEvents.size())
        return;

    const juce::int64 maxOffset = loopLengthSamples > 0 ? loopLengthSamples - 1 : newOffsetSamples;
    recordedEvents.getReference(eventIndex).offsetSamples = juce::jlimit<juce::int64>(0, maxOffset, newOffsetSamples);
    sortEvents();
}

void LoopTrack::setEventDuration(int eventIndex, juce::int64 durationSamples)
{
    if (eventIndex < 0 || eventIndex >= recordedEvents.size())
        return;

    recordedEvents.getReference(eventIndex).durationSamples = juce::jmax<juce::int64>(0, durationSamples);
}

void LoopTrack::setEventParams(int eventIndex, float gain, float pitchSemitones,
                              float reverbSend, float delaySend, float filterCutoff)
{
    if (eventIndex < 0 || eventIndex >= recordedEvents.size())
        return;

    auto& ev = recordedEvents.getReference(eventIndex);
    ev.gain = juce::jlimit(0.0f, 2.0f, gain);
    ev.pitchSemitones = juce::jlimit(-24.0f, 24.0f, pitchSemitones);
    ev.reverbSend = juce::jlimit(0.0f, 1.0f, reverbSend);
    ev.delaySend = juce::jlimit(0.0f, 1.0f, delaySend);
    ev.filterCutoff = juce::jlimit(0.0f, 1.0f, filterCutoff);
}

void LoopTrack::removeEvent(int eventIndex)
{
    if (eventIndex >= 0 && eventIndex < recordedEvents.size())
    {
        recordedEvents.remove(eventIndex);
        if (nextEventIndex >= recordedEvents.size())
            nextEventIndex = 0;
    }
}

void LoopTrack::duplicateEvent(int eventIndex, juce::int64 duplicateOffsetSamples)
{
    if (eventIndex < 0 || eventIndex >= recordedEvents.size())
        return;

    TriggerEvent copy = recordedEvents[eventIndex];
    const juce::int64 shift = duplicateOffsetSamples > 0 ? duplicateOffsetSamples : 4410; // ~100ms default
    if (loopLengthSamples > 0)
        copy.offsetSamples = (copy.offsetSamples + shift) % loopLengthSamples;
    else
        copy.offsetSamples += shift;

    recordedEvents.add(copy);
    sortEvents();
}

void LoopTrack::processAudioBlock(int numSamples, AudioEngine& engine)
{
    if (recording)
    {
        loopLengthSamples += numSamples;
        return;
    }

    if (muted || recordedEvents.isEmpty() || loopLengthSamples <= 0)
        return;

    const juce::int64 startPos = playbackPositionSamples;
    const juce::int64 endPos = startPos + numSamples;
    const int totalEvents = recordedEvents.size();

    // Re-synchronize event cursor if desynchronized or seeking occurred
    if (nextEventIndex < 0
        || (nextEventIndex < totalEvents && recordedEvents[nextEventIndex].offsetSamples < startPos)
        || (nextEventIndex > 0 && recordedEvents[nextEventIndex - 1].offsetSamples >= startPos))
    {
        auto it = std::lower_bound(recordedEvents.begin(), recordedEvents.end(), startPos,
                                   [](const TriggerEvent& ev, juce::int64 pos) {
                                       return ev.offsetSamples < pos;
                                   });
        nextEventIndex = static_cast<int>(std::distance(recordedEvents.begin(), it));
    }

    if (endPos <= loopLengthSamples)
    {
        while (nextEventIndex < totalEvents && recordedEvents[nextEventIndex].offsetSamples < endPos)
        {
            const auto& ev = recordedEvents[nextEventIndex++];
            if (ev.offsetSamples >= startPos && ev.sampleHandle >= 0)
                engine.triggerSample(ev.sampleHandle, ev.gain);
        }
    }
    else
    {
        while (nextEventIndex < totalEvents)
        {
            const auto& ev = recordedEvents[nextEventIndex++];
            if (ev.offsetSamples >= startPos && ev.sampleHandle >= 0)
                engine.triggerSample(ev.sampleHandle, ev.gain);
        }

        nextEventIndex = 0;
        const juce::int64 wrappedEnd = endPos % loopLengthSamples;
        while (nextEventIndex < totalEvents && recordedEvents[nextEventIndex].offsetSamples < wrappedEnd)
        {
            const auto& ev = recordedEvents[nextEventIndex++];
            if (ev.sampleHandle >= 0)
                engine.triggerSample(ev.sampleHandle, ev.gain);
        }
    }

    playbackPositionSamples = (playbackPositionSamples + numSamples) % loopLengthSamples;
}

//==============================================================================
// AudioEngine

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    generateDefaultStarterKit();
    generateClickBuffers();
}


AudioEngine::~AudioEngine() = default;

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);
    currentSampleRate = sampleRate;
    metronome.prepareToPlay(sampleRate, samplesPerBlockExpected);
    quantizer.setSampleRate(sampleRate);
    masterFxRack.prepare(sampleRate, samplesPerBlockExpected);

    const juce::ScopedLock sl(audioLock);
    for (auto& v : voices)
    {
        v.isActive = false;
        v.sourceBuffer = nullptr;
        v.position = 0;
    }
}


void AudioEngine::releaseResources()
{
    metronome.releaseResources();
    masterFxRack.reset();
    const juce::ScopedLock sl(audioLock);
    for (auto& v : voices)
    {
        v.isActive = false;
        v.sourceBuffer = nullptr;
    }
}


void AudioEngine::registerLoopTrack(LoopTrack* track)
{
    const juce::ScopedLock sl(audioLock);
    if (track != nullptr && ! activeLoopTracks.contains(track))
        activeLoopTracks.add(track);
}

void AudioEngine::unregisterLoopTrack(LoopTrack* track)
{
    const juce::ScopedLock sl(audioLock);
    activeLoopTracks.removeFirstMatchingValue(track);
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    const juce::ScopedLock sl(audioLock);

    // 0. Advance the metronome clock — emits beat events synchronously
    metronome.processBlock(bufferToFill.numSamples, [this](const BeatEvent& ev)
    {
        // Generate a click sample trigger on beat/downbeat
        if (! metronome.isClickEnabled())
            return;

        const bool isDown = (ev.type == BeatType::Downbeat);
        auto* clickBuf = isDown ? clickDownbeatBuffer.get() : clickBeatBuffer.get();
        if (clickBuf == nullptr)
            return;

        // Find a free voice for the click (skip voice stealing to protect musical voices)
        for (auto& voice : voices)
        {
            if (! voice.isActive)
            {
                voice.sourceBuffer = clickBuf;
                voice.position = 0;
                voice.gain = isDown ? 0.22f : 0.08f;
                voice.isActive = true;
                break;
            }
        }
    });

    // 1. Process active looper lanes
    for (auto* track : activeLoopTracks)
    {
        if (track != nullptr)
            track->processAudioBlock(bufferToFill.numSamples, *this);
    }

    // 2. Mix active voices into the destination buffer
    auto* outBuffer = bufferToFill.buffer;
    const int numOutChannels = outBuffer->getNumChannels();

    for (auto& voice : voices)
    {
        if (! voice.isActive || voice.sourceBuffer == nullptr)
            continue;

        const int samplesRemaining = voice.sourceBuffer->getNumSamples() - voice.position;
        const int samplesToCopy = juce::jmin(samplesRemaining, bufferToFill.numSamples);

        if (samplesToCopy <= 0)
        {
            voice.isActive = false;
            continue;
        }

        const int numSrcChannels = voice.sourceBuffer->getNumChannels();

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const int srcCh = juce::jmin(ch, numSrcChannels - 1);
            outBuffer->addFrom(ch, bufferToFill.startSample,
                               *voice.sourceBuffer, srcCh, voice.position,
                               samplesToCopy, voice.gain);
        }

        voice.position += samplesToCopy;
        if (voice.position >= voice.sourceBuffer->getNumSamples())
            voice.isActive = false;
    }

    // 3. Master FX Rack: Filter -> Delay -> Reverb
    masterFxRack.processBlock(*outBuffer, bufferToFill.startSample, bufferToFill.numSamples, metronome.getBpm());
}

int AudioEngine::findSampleHandle(const juce::String& identifier) const
{
    auto it = samplePathToHandle.find(identifier.toStdString());
    if (it != samplePathToHandle.end())
        return it->second;
    return -1;
}

int AudioEngine::addMemoryBuffer(std::unique_ptr<juce::AudioBuffer<float>> buffer, const juce::String& identifier)
{
    const juce::ScopedLock sl(audioLock);
    loadedSamples.add(buffer.release());
    const int handle = loadedSamples.size() - 1;
    samplePathToHandle[identifier.toStdString()] = handle;
    return handle;
}

int AudioEngine::loadSample(const juce::File& audioFile)
{
    if (! audioFile.existsAsFile())
        return -1;

    const auto pathStr = audioFile.getFullPathName().toStdString();
    auto it = samplePathToHandle.find(pathStr);
    if (it != samplePathToHandle.end())
        return it->second;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    if (reader == nullptr)
        return -1;

    auto buffer = std::make_unique<juce::AudioBuffer<float>>(
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples));

    reader->read(buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    const juce::ScopedLock sl(audioLock);
    loadedSamples.add(buffer.release());
    const int handle = loadedSamples.size() - 1;
    samplePathToHandle[pathStr] = handle;
    return handle;
}

void AudioEngine::generateDefaultStarterKit()
{
    const double sRate = 44100.0;

    // 1. Procedural Kick Drum (0.3s pitch sweep sine)
    {
        const int numSamples = static_cast<int>(sRate * 0.3);
        auto kickBuf = std::make_unique<juce::AudioBuffer<float>>(2, numSamples);
        kickBuf->clear();
        float phase = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float progress = static_cast<float>(i) / numSamples;
            const float freq = 140.0f * std::exp(-progress * 9.0f) + 40.0f;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * freq / sRate);
            const float env = std::exp(-progress * 6.5f);
            const float sample = std::sin(phase) * env * 0.9f;
            kickBuf->setSample(0, i, sample);
            kickBuf->setSample(1, i, sample);
        }
        addMemoryBuffer(std::move(kickBuf), "starter://kick");
    }

    // 2. Procedural Snare Drum (0.2s body + noise)
    {
        const int numSamples = static_cast<int>(sRate * 0.22);
        auto snareBuf = std::make_unique<juce::AudioBuffer<float>>(2, numSamples);
        snareBuf->clear();
        juce::Random rnd(12345);
        float phase = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float progress = static_cast<float>(i) / numSamples;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * 185.0 / sRate);
            const float tone = std::sin(phase) * std::exp(-progress * 15.0f) * 0.5f;
            const float noise = (rnd.nextFloat() * 2.0f - 1.0f) * std::exp(-progress * 9.0f) * 0.6f;
            const float sample = (tone + noise) * 0.85f;
            snareBuf->setSample(0, i, sample);
            snareBuf->setSample(1, i, sample);
        }
        addMemoryBuffer(std::move(snareBuf), "starter://snare");
    }

    // 3. Procedural Closed Hi-Hat (0.05s filtered noise)
    {
        const int numSamples = static_cast<int>(sRate * 0.06);
        auto hatBuf = std::make_unique<juce::AudioBuffer<float>>(2, numSamples);
        hatBuf->clear();
        juce::Random rnd(54321);
        float lastNoise = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float progress = static_cast<float>(i) / numSamples;
            const float rawNoise = rnd.nextFloat() * 2.0f - 1.0f;
            const float hpNoise = rawNoise - lastNoise; // High pass filter
            lastNoise = rawNoise;
            const float sample = hpNoise * std::exp(-progress * 28.0f) * 0.7f;
            hatBuf->setSample(0, i, sample);
            hatBuf->setSample(1, i, sample);
        }
        addMemoryBuffer(std::move(hatBuf), "starter://hihat");
    }

    // 4. Procedural 808 Sub Bass (0.6s warm sine)
    {
        const int numSamples = static_cast<int>(sRate * 0.65);
        auto bassBuf = std::make_unique<juce::AudioBuffer<float>>(2, numSamples);
        bassBuf->clear();
        float phase = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float progress = static_cast<float>(i) / numSamples;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * 55.0 / sRate); // A1 note
            float val = std::sin(phase) * std::exp(-progress * 2.5f);
            val = std::tanh(val * 1.5f); // Soft saturation warmth
            bassBuf->setSample(0, i, val * 0.8f);
            bassBuf->setSample(1, i, val * 0.8f);
        }
        addMemoryBuffer(std::move(bassBuf), "starter://bass808");
    }

    // 5. Procedural Synth Blip (0.3s melodic stab)
    {
        const int numSamples = static_cast<int>(sRate * 0.35);
        auto synthBuf = std::make_unique<juce::AudioBuffer<float>>(2, numSamples);
        synthBuf->clear();
        float phase = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float progress = static_cast<float>(i) / numSamples;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * 440.0 / sRate); // A4 note
            const float tri = (std::asin(std::sin(phase)) * (2.0f / 3.14159f)) * std::exp(-progress * 5.0f);
            synthBuf->setSample(0, i, tri * 0.75f);
            synthBuf->setSample(1, i, tri * 0.75f);
        }
        addMemoryBuffer(std::move(synthBuf), "starter://synth");
    }
}

void AudioEngine::triggerSample(int sampleHandle, float gain)
{
    const juce::ScopedLock sl(audioLock);

    if (sampleHandle < 0 || sampleHandle >= loadedSamples.size())
        return;

    const auto* bufferPtr = loadedSamples[sampleHandle];
    if (bufferPtr == nullptr)
        return;

    // Search for a free voice
    for (auto& voice : voices)
    {
        if (! voice.isActive)
        {
            voice.sourceBuffer = bufferPtr;
            voice.position = 0;
            voice.gain = gain;
            voice.isActive = true;
            return;
        }
    }

    // Voice stealing: reallocate the voice furthest along in playback
    int maxPos = -1;
    int stealIndex = 0;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].position > maxPos)
        {
            maxPos = voices[i].position;
            stealIndex = i;
        }
    }

    voices[stealIndex].sourceBuffer = bufferPtr;
    voices[stealIndex].position = 0;
    voices[stealIndex].gain = gain;
    voices[stealIndex].isActive = true;
}

void AudioEngine::setTempo(double bpm)
{
    metronome.setTempo(bpm);
    quantizer.setTempo(bpm);
}

void AudioEngine::generateClickBuffers()
{
    // Downbeat click: higher-pitched woodblock at 1400Hz
    {
        const int len = static_cast<int>(44100.0 * 0.03);
        auto buf = std::make_unique<juce::AudioBuffer<float>>(2, len);
        buf->clear();
        float phase = 0.0f;
        for (int i = 0; i < len; ++i)
        {
            const float t = static_cast<float>(i) / len;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * 1400.0 / 44100.0);
            const float sample = std::sin(phase) * std::exp(-t * 60.0f) * 0.8f;
            buf->setSample(0, i, sample);
            buf->setSample(1, i, sample);
        }
        clickDownbeatBuffer = std::move(buf);
    }

    // Beat click: lower-pitched woodblock at 900Hz
    {
        const int len = static_cast<int>(44100.0 * 0.025);
        auto buf = std::make_unique<juce::AudioBuffer<float>>(2, len);
        buf->clear();
        float phase = 0.0f;
        for (int i = 0; i < len; ++i)
        {
            const float t = static_cast<float>(i) / len;
            phase += static_cast<float>(juce::MathConstants<double>::twoPi * 900.0 / 44100.0);
            const float sample = std::sin(phase) * std::exp(-t * 65.0f) * 0.4f;
            buf->setSample(0, i, sample);
            buf->setSample(1, i, sample);
        }
        clickBeatBuffer = std::move(buf);
    }
}
