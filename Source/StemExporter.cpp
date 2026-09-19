/*
    StemExporter.cpp
    ----------------
    Implementation of multitrack WAV stem exporter.
*/

#include "StemExporter.h"
#include <algorithm>

bool StemExporter::writeBufferToWav(const juce::AudioBuffer<float>& buffer,
                                    const juce::File& destinationFile,
                                    double sampleRate,
                                    int bitDepth)
{
    if (destinationFile.exists())
        destinationFile.deleteFile();

    if (! destinationFile.getParentDirectory().exists())
        destinationFile.getParentDirectory().createDirectory();

    auto outStream = std::make_unique<juce::FileOutputStream>(destinationFile);
    if (! outStream->openedOk())
        return false;

    juce::WavAudioFormat wavFormat;
    juce::StringPairArray metadataValues;
    metadataValues.set("Software", "Armoni Composer by EtherK3N");

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.get(),
                                  sampleRate,
                                  static_cast<unsigned int>(buffer.getNumChannels()),
                                  bitDepth,
                                  metadataValues,
                                  0));

    if (writer != nullptr)
    {
        outStream.release(); // Writer took ownership
        return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }

    return false;
}

StemExportResult StemExporter::renderStems(const AudioEngine& engine,
                                          const juce::Array<const LoopTrack*>& tracks,
                                          const juce::StringArray& trackNames,
                                          const StemExportOptions& options)
{
    StemExportResult result;

    if (tracks.isEmpty())
    {
        result.errorMessage = "No tracks provided for stem export.";
        return result;
    }

    // Determine the master loop length in samples
    juce::int64 maxLoopLength = 0;
    for (const auto* t : tracks)
    {
        if (t != nullptr)
            maxLoopLength = std::max(maxLoopLength, t->getLoopLengthSamples());
    }

    // If loop lengths are not set, scan events for maximum duration
    if (maxLoopLength <= 0)
    {
        for (const auto* t : tracks)
        {
            if (t != nullptr)
            {
                for (const auto& ev : t->recordedEvents)
                {
                    const auto* sBuf = engine.getSampleBuffer(ev.sampleHandle);
                    const juce::int64 sampleDur = (sBuf != nullptr && sBuf->getNumSamples() > 0)
                                                  ? sBuf->getNumSamples()
                                                  : static_cast<juce::int64>(options.sampleRate * 0.25);
                    const juce::int64 eventDur = (ev.durationSamples > 0) ? ev.durationSamples : sampleDur;
                    maxLoopLength = std::max(maxLoopLength, ev.offsetSamples + eventDur);
                }
            }
        }
    }

    if (maxLoopLength <= 0)
    {
        result.errorMessage = "Tracks are empty. Nothing to export.";
        return result;
    }

    const double sampleRate = options.sampleRate > 0.0 ? options.sampleRate : 44100.0;
    const int numChannels = 2;
    const int totalSamples = static_cast<int>(maxLoopLength);

    juce::AudioBuffer<float> masterBuffer(numChannels, totalSamples);
    masterBuffer.clear();

    const juce::File outDir = options.outputDirectory;
    if (! outDir.exists())
        outDir.createDirectory();

    int trackIndex = 1;
    for (int t = 0; t < tracks.size(); ++t)
    {
        const auto* track = tracks[t];
        if (track == nullptr)
            continue;

        const juce::String baseName = (t < trackNames.size() && trackNames[t].isNotEmpty())
                                      ? trackNames[t]
                                      : "Track_" + juce::String(trackIndex);

        // Sanitize file name
        const juce::String safeName = baseName.replaceCharacter(' ', '_').replaceCharacter('/', '_').replaceCharacter('\\', '_');

        juce::AudioBuffer<float> trackBuffer(numChannels, totalSamples);
        trackBuffer.clear();

        for (const auto& ev : track->recordedEvents)
        {
            const auto* sBuf = engine.getSampleBuffer(ev.sampleHandle);
            if (sBuf == nullptr || sBuf->getNumSamples() == 0)
                continue;

            const int offset = static_cast<int>(ev.offsetSamples);
            if (offset >= totalSamples)
                continue;

            const int copyLen = std::min(sBuf->getNumSamples(), totalSamples - offset);
            const float gain = ev.gain;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const int srcCh = std::min(ch, sBuf->getNumChannels() - 1);
                trackBuffer.addFrom(ch, offset, *sBuf, srcCh, 0, copyLen, gain);
            }
        }

        // Sum into master mix
        for (int ch = 0; ch < numChannels; ++ch)
            masterBuffer.addFrom(ch, 0, trackBuffer, ch, 0, totalSamples);

        // Export individual stem if enabled
        if (options.exportIndividualStems)
        {
            if (options.normalize)
            {
                const float peak = trackBuffer.getMagnitude(0, totalSamples);
                if (peak > 0.0001f)
                    trackBuffer.applyGain(0.95f / peak);
            }

            const juce::String filename = "Stem_" + juce::String::formatted("%02d", trackIndex) + "_" + safeName + ".wav";
            const juce::File stemFile = outDir.getChildFile(filename);

            if (writeBufferToWav(trackBuffer, stemFile, sampleRate, options.bitDepth))
                result.exportedFiles.add(stemFile);
        }

        trackIndex++;
    }

    // Export Master Mix if enabled
    if (options.exportMasterMix)
    {
        if (options.normalize)
        {
            const float peak = masterBuffer.getMagnitude(0, totalSamples);
            if (peak > 0.0001f)
                masterBuffer.applyGain(0.95f / peak);
        }

        const juce::File masterFile = outDir.getChildFile("Stem_Master_Mix.wav");
        if (writeBufferToWav(masterBuffer, masterFile, sampleRate, options.bitDepth))
            result.exportedFiles.add(masterFile);
    }

    result.success = ! result.exportedFiles.isEmpty();
    result.totalSamplesRendered = maxLoopLength;
    result.durationSeconds = static_cast<double>(maxLoopLength) / sampleRate;

    return result;
}
