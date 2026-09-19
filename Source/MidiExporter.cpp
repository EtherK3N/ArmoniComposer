/*
    MidiExporter.cpp
    ----------------
    Implementation of Type 1 Standard MIDI File exporter.
*/

#include "MidiExporter.h"
#include <cmath>
#include <algorithm>

int MidiExporter::resolveMidiNote(int sampleHandle, const juce::String& identifier,
                                  const juce::String& trackRole, float pitchSemitones)
{
    const juce::String idLower = identifier.toLowerCase();
    const juce::String roleLower = trackRole.toLowerCase();

    // General MIDI Drum mapping (Channel 10 convention)
    if (roleLower.contains("drum") || idLower.contains("drum")
        || idLower.contains("kick") || idLower.contains("snare")
        || idLower.contains("hat") || idLower.contains("clap"))
    {
        if (idLower.contains("kick") || idLower.contains("808kick") || idLower.contains("bd"))
            return 36; // C1: Bass Drum 1
        if (idLower.contains("snare") || idLower.contains("sd"))
            return 38; // D1: Acoustic Snare
        if (idLower.contains("clap") || idLower.contains("cp"))
            return 39; // D#1: Hand Clap
        if (idLower.contains("hat") || idLower.contains("hihat") || idLower.contains("hh"))
        {
            if (idLower.contains("open") || idLower.contains("oh"))
                return 46; // A#1: Open Hi-Hat
            return 42;     // F#1: Closed Hi-Hat
        }
        if (idLower.contains("tom"))
            return 45; // A1: Low Tom
        if (idLower.contains("crash") || idLower.contains("cymbal"))
            return 49; // C#2: Crash Cymbal 1
        if (idLower.contains("ride"))
            return 51; // D#2: Ride Cymbal 1

        // Default drum note
        return 36;
    }

    // Melodic Bass mapping (Root C1 = 36 or C2 = 48)
    if (roleLower.contains("bass") || idLower.contains("bass"))
    {
        const int baseNote = 36; // C1
        const int note = baseNote + static_cast<int>(std::round(pitchSemitones));
        return juce::jlimit<int>(12, 72, note);
    }

    // Melodic Synth / Lead / Rhodes / Chord mapping (Root C3 = 60)
    const int baseNote = 60; // C3
    const int note = baseNote + static_cast<int>(std::round(pitchSemitones));
    return juce::jlimit<int>(12, 127, note);
}

MidiExportResult MidiExporter::exportToFile(const juce::Array<const LoopTrack*>& tracks,
                                           const juce::StringArray& trackNames,
                                           const juce::File& outputFile,
                                           const MidiExportOptions& options,
                                           const AudioEngine* engine)
{
    MidiExportResult result;
    result.exportedFile = outputFile;

    if (tracks.isEmpty())
    {
        result.errorMessage = "No tracks provided for MIDI export.";
        return result;
    }

    const double sampleRate = options.sampleRate > 0.0 ? options.sampleRate : 44100.0;
    const double bpm = options.bpm > 0.0 ? options.bpm : 120.0;
    const short ppqn = options.ticksPerQuarterNote > 0 ? options.ticksPerQuarterNote : 960;

    // Convert samples to MIDI ticks:
    // samples -> seconds = samples / sampleRate
    // seconds -> quarter notes = seconds * (bpm / 60.0)
    // quarter notes -> ticks = quarterNotes * ppqn
    const double samplesToTicks = (bpm / 60.0) * static_cast<double>(ppqn) / sampleRate;

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(ppqn);

    // Track 0: Conductor Track (Tempo, Time Signature)
    juce::MidiMessageSequence conductorTrack;
    const int microsecondsPerQuarterNote = static_cast<int>(std::round(60000000.0 / bpm));
    conductorTrack.addEvent(juce::MidiMessage::tempoMetaEvent(microsecondsPerQuarterNote), 0.0);
    conductorTrack.addEvent(juce::MidiMessage::timeSignatureMetaEvent(options.timeSignatureNumerator,
                                                                     options.timeSignatureDenominator), 0.0);
    conductorTrack.addEvent(juce::MidiMessage::textMetaEvent(3, "Conductor Track"), 0.0);
    conductorTrack.addEvent(juce::MidiMessage::endOfTrack(), 0.0);
    midiFile.addTrack(conductorTrack);

    int totalNotes = 0;
    int exportedTrackCount = 0;

    for (int t = 0; t < tracks.size(); ++t)
    {
        const auto* track = tracks[t];
        if (track == nullptr)
            continue;

        const juce::String trackName = (t < trackNames.size() && trackNames[t].isNotEmpty())
                                       ? trackNames[t]
                                       : "Track " + juce::String(t + 1);

        juce::MidiMessageSequence trackSeq;
        trackSeq.addEvent(juce::MidiMessage::textMetaEvent(3, trackName), 0.0);

        const int midiChannel = trackName.toLowerCase().contains("drum") ? 10 : juce::jlimit(1, 16, t + 1);
        double maxTick = 0.0;

        for (const auto& ev : track->recordedEvents)
        {
            juce::String sampleId;
            if (engine != nullptr)
                sampleId = engine->getSampleIdentifier(ev.sampleHandle);

            const int noteNumber = resolveMidiNote(ev.sampleHandle, sampleId, trackName, ev.pitchSemitones);
            const float velocity = juce::jlimit<float>(0.05f, 1.0f, ev.gain * 0.8f);

            const double startTick = static_cast<double>(ev.offsetSamples) * samplesToTicks;

            // Default gate length to 1/16th note if duration is 0
            double durationTicks = (ev.durationSamples > 0)
                                   ? static_cast<double>(ev.durationSamples) * samplesToTicks
                                   : static_cast<double>(ppqn) / 4.0; // 1/16th note

            if (durationTicks < 10.0)
                durationTicks = 10.0;

            const double endTick = startTick + durationTicks;
            maxTick = std::max(maxTick, endTick);

            trackSeq.addEvent(juce::MidiMessage::noteOn(midiChannel, noteNumber, velocity), startTick);
            trackSeq.addEvent(juce::MidiMessage::noteOff(midiChannel, noteNumber, 0.0f), endTick);

            totalNotes++;
        }

        // Snap loop end tick if track has a defined loop length
        if (track->getLoopLengthSamples() > 0)
        {
            const double loopEndTick = static_cast<double>(track->getLoopLengthSamples()) * samplesToTicks;
            maxTick = std::max(maxTick, loopEndTick);
        }

        trackSeq.addEvent(juce::MidiMessage::endOfTrack(), maxTick + 1.0);
        trackSeq.updateMatchedPairs();
        midiFile.addTrack(trackSeq);
        exportedTrackCount++;
    }

    // Ensure destination directory exists
    if (! outputFile.getParentDirectory().exists())
        outputFile.getParentDirectory().createDirectory();

    outputFile.deleteFile();
    std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());
    if (outStream == nullptr || ! outStream->openedOk())
    {
        result.errorMessage = "Could not create output stream for file: " + outputFile.getFullPathName();
        return result;
    }

    if (! midiFile.writeTo(*outStream))
    {
        result.errorMessage = "Failed to write MIDI data to stream.";
        return result;
    }

    result.success = true;
    result.totalTracksExported = exportedTrackCount;
    result.totalNotesExported = totalNotes;
    return result;
}
