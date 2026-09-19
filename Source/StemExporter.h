/*
    StemExporter.h
    --------------
    DAW-ready multitrack WAV stem exporter for K3N Armoni Composer:
      - Synchronized broadcast-quality 24-bit / 16-bit / 32-bit float WAV stems
      - Individual lane bounces (Drums, Bass, Synth, FX) + Master Stereo Mix
      - Sample-accurate loop alignment and optional peak normalization
      - Embeds loop length, sample rate, and channel metadata
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "AudioEngine.h"

struct StemExportOptions
{
    juce::File outputDirectory;
    double sampleRate = 44100.0;
    int bitDepth = 24;                 // 16, 24, or 32
    bool exportIndividualStems = true;  // Stem_01_Drums.wav, etc.
    bool exportMasterMix = true;        // Stem_Master_Mix.wav
    bool normalize = false;             // Peak normalize to -0.2 dBFS
};

struct StemExportResult
{
    bool success = false;
    juce::String errorMessage;
    juce::Array<juce::File> exportedFiles;
    juce::int64 totalSamplesRendered = 0;
    double durationSeconds = 0.0;
};

class StemExporter
{
public:
    StemExporter() = default;

    /** Renders the provided loop tracks into synchronized WAV stems and master mix. */
    static StemExportResult renderStems(const AudioEngine& engine,
                                        const juce::Array<const LoopTrack*>& tracks,
                                        const juce::StringArray& trackNames,
                                        const StemExportOptions& options);

private:
    static bool writeBufferToWav(const juce::AudioBuffer<float>& buffer,
                                 const juce::File& destinationFile,
                                 double sampleRate,
                                 int bitDepth);
};
