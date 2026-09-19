#include "PerformanceViewComponent.h"
#include "StemExporter.h"
#include "MidiExporter.h"

namespace
{
    juce::String roleToLabel(KeyboardRole role)
    {
        switch (role)
        {
            case KeyboardRole::Drums:    return "Drums Track";
            case KeyboardRole::Bass:     return "Bass Track";
            case KeyboardRole::FX:       return "FX Track";
            case KeyboardRole::Melody:   return "Melody Track";
            case KeyboardRole::Adaptive: return "Adaptive Track";
            case KeyboardRole::Unassigned:
            default: return "Free Track";
        }
    }
}

PerformanceViewComponent::PerformanceViewComponent(AudioEngine& audioEngine, MappingEngine& mappingEngine, DeviceManager& deviceManager)
    : audio(audioEngine), mapping(mappingEngine), devices(deviceManager)
{
    juce::ignoreUnused(devices);

    const KeyboardRole roles[kNumTracks] = {
        KeyboardRole::Drums, KeyboardRole::Bass, KeyboardRole::FX,
        KeyboardRole::Melody, KeyboardRole::Adaptive
    };

    for (auto role : roles)
    {
        auto* row = new TrackRow();
        row->role = role;
        row->label = roleToLabel(role);

        addAndMakeVisible(row->statusLabel);
        addAndMakeVisible(row->recordButton);
        addAndMakeVisible(row->muteButton);
        addAndMakeVisible(row->clearButton);

        row->recordButton.addListener(this);
        row->muteButton.addListener(this);
        row->clearButton.addListener(this);

        // Register loop track with the audio engine
        audio.registerLoopTrack(&row->track);

        tracks.add(row);
    }

    hudBankLabel.setJustificationType(juce::Justification::centred);
    hudOctaveLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(hudBankLabel);
    addAndMakeVisible(hudOctaveLabel);
    addAndMakeVisible(numpadModeToggle);

    addAndMakeVisible(exportStemsButton);
    addAndMakeVisible(exportMidiButton);
    addAndMakeVisible(exportStatusLabel);
    exportStemsButton.addListener(this);
    exportMidiButton.addListener(this);
    exportStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);

    numpadModeToggle.onClick = [this]
    {
        shiftLayers.setNumpadModeEnabled(numpadModeToggle.getToggleState());
    };

    shiftLayers.onStateChanged = [this](KeyBank, int)
    {
        updateStatusLabels();
    };

    updateStatusLabels();
    startTimerHz(15);
}

PerformanceViewComponent::~PerformanceViewComponent()
{
    stopTimer();
    for (auto* row : tracks)
    {
        if (row != nullptr)
            audio.unregisterLoopTrack(&row->track);
    }
}

void PerformanceViewComponent::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Layer System HUD Bar
    auto hudArea = area.removeFromTop(34);
    hudBankLabel.setBounds(hudArea.removeFromLeft(220).reduced(2));
    hudOctaveLabel.setBounds(hudArea.removeFromLeft(180).reduced(2));
    numpadModeToggle.setBounds(hudArea.removeFromLeft(180).reduced(2));
    area.removeFromTop(10);

    const int rowHeight = 44;

    for (auto* row : tracks)
    {
        auto rowArea = area.removeFromTop(rowHeight);
        row->statusLabel.setBounds(rowArea.removeFromLeft(rowArea.getWidth() - 3 * 95));
        row->recordButton.setBounds(rowArea.removeFromLeft(95).reduced(4));
        row->muteButton.setBounds(rowArea.removeFromLeft(95).reduced(4));
        row->clearButton.setBounds(rowArea.removeFromLeft(95).reduced(4));
        area.removeFromTop(8);
    }

    area.removeFromTop(8);
    auto exportArea = area.removeFromTop(32);
    exportStemsButton.setBounds(exportArea.removeFromLeft(170).reduced(2));
    exportMidiButton.setBounds(exportArea.removeFromLeft(170).reduced(2));
    exportStatusLabel.setBounds(exportArea.reduced(2));
}

PerformanceViewComponent::TrackRow* PerformanceViewComponent::findRow(KeyboardRole role)
{
    for (auto* row : tracks)
        if (row->role == role)
            return row;
    return nullptr;
}

void PerformanceViewComponent::buttonClicked(juce::Button* b)
{
    if (b == &exportStemsButton)
    {
        exportStems();
        return;
    }
    if (b == &exportMidiButton)
    {
        exportMidi();
        return;
    }

    for (auto* row : tracks)
    {
        if (b == &row->recordButton)
        {
            if (row->track.isRecording())
            {
                row->track.stopRecordingAndStartLoop();
            }
            else
            {
                row->track.startRecording();
                row->recordingStartMs = juce::Time::currentTimeMillis();
            }
            updateStatusLabels();
            return;
        }
        if (b == &row->muteButton)
        {
            row->track.setMuted(! row->track.isMuted());
            updateStatusLabels();
            return;
        }
        if (b == &row->clearButton)
        {
            row->track.clear();
            updateStatusLabels();
            return;
        }
    }
}

void PerformanceViewComponent::timerCallback()
{
    updateStatusLabels();
}

void PerformanceViewComponent::updateStatusLabels()
{
    // Update Layer System HUD
    const auto activeBank = shiftLayers.getActiveBank();
    juce::String bankText = "BANK: " + getBankName(activeBank);
    if (shiftLayers.isShiftHeld())
        bankText << " [SHIFT]";
    if (shiftLayers.isCapsLockOn())
        bankText << " [CAPS]";

    hudBankLabel.setText(bankText, juce::dontSendNotification);

    juce::Colour bankCol = juce::Colour(0xff00d2ff); // Drums (cyan)
    if (activeBank == KeyBank::Bass)       bankCol = juce::Colour(0xffff9900); // Bass (orange)
    else if (activeBank == KeyBank::Synth) bankCol = juce::Colour(0xffd050ff); // Synth (magenta)
    else if (activeBank == KeyBank::FX)    bankCol = juce::Colour(0xffffdd00); // FX (yellow)

    hudBankLabel.setColour(juce::Label::textColourId, bankCol);

    const int oct = shiftLayers.getActiveOctaveOffset();
    juce::String octText = "OCT: " + (oct > 0 ? "+" : "") + juce::String(oct) + " (Tab+1..5)";
    hudOctaveLabel.setText(octText, juce::dontSendNotification);

    numpadModeToggle.setToggleState(shiftLayers.isNumpadModeEnabled(), juce::dontSendNotification);

    for (auto* row : tracks)
    {
        juce::String status = row->label + ": ";

        if (row->track.isRecording())
        {
            auto elapsedMs = juce::Time::currentTimeMillis() - row->recordingStartMs;
            status << "[RECORDING (" << juce::String(elapsedMs / 1000.0, 1) << "s)]";
            row->recordButton.setButtonText("Stop Rec");
        }
        else if (row->track.isEmpty())
        {
            status << "[Empty Track]";
            row->recordButton.setButtonText("Rec");
        }
        else
        {
            status << (row->track.isMuted() ? "[Looping - MUTED]" : "[Looping - ACTIVE]");
            row->recordButton.setButtonText("Overdub");
        }

        row->muteButton.setButtonText(row->track.isMuted() ? "Unmute" : "Mute");
        row->statusLabel.setText(status, juce::dontSendNotification);
    }
}

void PerformanceViewComponent::handleKeyEvent(const RawKeyEvent& event, KeyboardRole role)
{
    ProcessedKeyEvent processed;
    const bool isTrigger = shiftLayers.processKeyEvent(event, processed);

    if (! isTrigger || ! processed.isKeyDown)
        return;

    const auto* binding = mapping.findBinding(processed.deviceId,
                                              processed.effectiveVirtualKeyCode,
                                              static_cast<int>(processed.activeBank));
    if (binding == nullptr || binding->soundPath.isEmpty())
        return;

    // Fast handle resolution from RAM cache
    int handle = audio.findSampleHandle(binding->soundPath);
    if (handle < 0)
        handle = audio.loadSample(juce::File(binding->soundPath));

    if (handle >= 0)
        audio.triggerSample(handle);

    // Synchronous recording to active track
    if (auto* row = findRow(role))
    {
        if (row->track.isRecording())
        {
            const auto offsetMs = juce::Time::currentTimeMillis() - row->recordingStartMs;
            const double sRate = audio.getSampleRate() > 0 ? audio.getSampleRate() : 44100.0;
            const auto offsetSamples = static_cast<juce::int64>((offsetMs / 1000.0) * sRate);
            row->track.recordTrigger(handle, offsetSamples);
        }
    }
}

void PerformanceViewComponent::exportStems()
{
    juce::Array<const LoopTrack*> loopTracks;
    juce::StringArray trackNames;

    for (const auto* row : tracks)
    {
        if (row != nullptr)
        {
            loopTracks.add(&row->track);
            trackNames.add(row->label);
        }
    }

    const auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    const auto targetDir = docsDir.getChildFile("ArmoniComposer").getChildFile("Stems");

    StemExportOptions options;
    options.outputDirectory = targetDir;
    options.sampleRate = audio.getSampleRate();
    options.bitDepth = 24;
    options.exportIndividualStems = true;
    options.exportMasterMix = true;
    options.normalize = true;

    const auto result = StemExporter::renderStems(audio, loopTracks, trackNames, options);
    if (result.success)
    {
        exportStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        exportStatusLabel.setText("Stems exported: " + juce::String(result.exportedFiles.size()) + " files to " + targetDir.getFileName(),
                                  juce::dontSendNotification);
    }
    else
    {
        exportStatusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        exportStatusLabel.setText("Stem export notice: " + result.errorMessage, juce::dontSendNotification);
    }
}

void PerformanceViewComponent::exportMidi()
{
    juce::Array<const LoopTrack*> loopTracks;
    juce::StringArray trackNames;

    for (const auto* row : tracks)
    {
        if (row != nullptr)
        {
            loopTracks.add(&row->track);
            trackNames.add(row->label);
        }
    }

    const auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    const auto targetFile = docsDir.getChildFile("ArmoniComposer").getChildFile("ArmoniSession.mid");

    MidiExportOptions options;
    options.sampleRate = audio.getSampleRate();
    options.bpm = audio.getMetronome().getTempo();
    options.timeSignatureNumerator = audio.getMetronome().getTimeSignatureNumerator();
    options.timeSignatureDenominator = audio.getMetronome().getTimeSignatureDenominator();
    options.ticksPerQuarterNote = 960;

    const auto result = MidiExporter::exportToFile(loopTracks, trackNames, targetFile, options, &audio);
    if (result.success)
    {
        exportStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        exportStatusLabel.setText("MIDI exported: " + juce::String(result.totalNotesExported) + " notes (" + juce::String(result.totalTracksExported) + " tracks)",
                                  juce::dontSendNotification);
    }
    else
    {
        exportStatusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        exportStatusLabel.setText("MIDI export notice: " + result.errorMessage, juce::dontSendNotification);
    }
}

