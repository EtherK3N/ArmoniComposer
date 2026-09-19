/*
    MidiExporter.h
    --------------
    Standard Type 1 MIDI File (.mid) exporter for K3N Armoni Composer:
      - Converts LoopTrack recorded triggers, micro-timings, gate lengths, and parameter locks into MIDI sequences
      - 960 PPQN high-resolution timing
      - Tempo & Time Signature meta-events
      - Intelligent General MIDI drum and chromatic note mapping
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "AudioEngine.h"

struct MidiExportOptions
{
    double sampleRate = 44100.0;
    double bpm = 120.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    short ticksPerQuarterNote = 960;
};

struct MidiExportResult
{
    bool success = false;
    juce::String errorMessage;
    juce::File exportedFile;
    int totalTracksExported = 0;
    int totalNotesExported = 0;
};

class MidiExporter
{
public:
    MidiExporter() = default;

    /** Maps a sample handle/identifier and pitch offset to a standard General MIDI note number (0-127). */
    static int resolveMidiNote(int sampleHandle, const juce::String& identifier,
                               const juce::String& trackRole, float pitchSemitones = 0.0f);

    /** Exports an array of LoopTracks and their names into a Type 1 Standard MIDI File (.mid). */
    static MidiExportResult exportToFile(const juce::Array<const LoopTrack*>& tracks,
                                         const juce::StringArray& trackNames,
                                         const juce::File& outputFile,
                                         const MidiExportOptions& options,
                                         const AudioEngine* engine = nullptr);
};
