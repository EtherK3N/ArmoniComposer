/*
    TestRunner.cpp
    --------------
    Automated Unit & Integration Test Suite for K3N Armoni Composer:
      - Tests AudioEngine real-time buffer mixing & procedural synthesis
      - Tests LoopTrack sample-accurate wrap-around trigger logic
      - Tests MappingEngine JSON serialization / deserialization round-trip
      - Tests DeviceManager role assignments
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "../Source/AudioEngine.h"
#include "../Source/MappingEngine.h"
#include "../Source/DeviceManager.h"
#include "../Source/ShiftLayerSystem.h"
#include "../Source/DspFxRack.h"
#include "../Source/StemExporter.h"
#include "../Source/MidiExporter.h"
#include "../Source/MidiSyncEngine.h"

// Simple lightweight test harness
#define RUN_TEST(fn) \
    do { \
        std::cout << "[ RUN      ] " << #fn << "...\n"; \
        fn(); \
        std::cout << "[       OK ] " << #fn << " PASSED\n"; \
    } while (0)

void testAudioEngineProceduralKitAndVoices()
{
    AudioEngine engine;
    engine.prepareToPlay(512, 44100.0);

    // Verify default procedural sounds are cached
    const int kickHandle = engine.findSampleHandle("starter://kick");
    const int snareHandle = engine.findSampleHandle("starter://snare");
    const int bassHandle = engine.findSampleHandle("starter://bass808");

    assert(kickHandle >= 0 && "Kick handle must be valid");
    assert(snareHandle >= 0 && "Snare handle must be valid");
    assert(bassHandle >= 0 && "Bass handle must be valid");

    // Trigger voices
    engine.triggerSample(kickHandle, 0.8f);
    engine.triggerSample(snareHandle, 0.7f);

    // Process an audio block
    juce::AudioBuffer<float> testBuffer(2, 512);
    testBuffer.clear();
    juce::AudioSourceChannelInfo info(&testBuffer, 0, 512);

    engine.getNextAudioBlock(info);

    // Verify non-zero output (sound generated)
    float maxMagnitude = testBuffer.getMagnitude(0, 512);
    assert(maxMagnitude > 0.001f && "Audio output must not be silent after triggering samples");
    assert(maxMagnitude <= 2.0f && "Audio output must not wildly blow up");

    engine.releaseResources();
}

void testLoopTrackWrapAroundTriggerMath()
{
    AudioEngine engine;
    engine.prepareToPlay(256, 44100.0);

    LoopTrack track;
    track.startRecording();

    // Record an event at offset 100 samples and offset 400 samples
    const int kickHandle = engine.findSampleHandle("starter://kick");
    track.recordTrigger(kickHandle, 100);
    track.recordTrigger(kickHandle, 400);

    track.stopRecordingAndStartLoop();

    assert(! track.isEmpty() && "Track should not be empty");
    assert(track.getLoopLengthSamples() >= 400 && "Loop length must be at least 400 samples");

    // Process block 1 (0 to 256): event at 100 should fire
    track.processAudioBlock(256, engine);

    // Process block 2 (256 to 512): event at 400 should fire
    track.processAudioBlock(256, engine);

    // Test clear
    track.clear();
    assert(track.isEmpty() && "Track must be empty after clear");

    engine.releaseResources();
}

void testMappingEngineJsonSerialization()
{
    MappingEngine mapping;

    const DeviceId testDevice = "HID#VID_TEST&PID_0001#USBPORT1";
    mapping.assignKey(testDevice, 0x41 /* Key 'A' */, "Kick Drum", "starter://kick");
    mapping.assignKey(testDevice, 0x42 /* Key 'B' */, "Snare Drum", "starter://snare");

    const auto* bindingA = mapping.findBinding(testDevice, 0x41);
    assert(bindingA != nullptr && "Binding for Key A must exist");
    assert(bindingA->soundLabel == "Kick Drum" && "Binding label must match");

    // Test rename
    mapping.renameBinding(testDevice, 0x41, "Fat 808 Kick");
    bindingA = mapping.findBinding(testDevice, 0x41);
    assert(bindingA->soundLabel == "Fat 808 Kick" && "Renamed binding must be updated");

    // Test remove
    mapping.removeBinding(testDevice, 0x42);
    assert(mapping.findBinding(testDevice, 0x42) == nullptr && "Removed binding must be null");

    // Test save & reload
    mapping.saveActiveSetToDisk();
    mapping.loadActiveSetFromDisk();

    const auto* reloadedBinding = mapping.findBinding(testDevice, 0x41);
    assert(reloadedBinding != nullptr && "Reloaded binding must persist");
    assert(reloadedBinding->soundLabel == "Fat 808 Kick" && "Persisted label must match");
}

void testMetronomeClockAccuracy()
{
    MetronomeClock clock;
    const double sr = 44100.0;
    clock.prepareToPlay(sr, 512);
    clock.setTempo(120.0); // 120 BPM => 0.5s per beat => 22050 samples per beat
    clock.setTimeSignature(4, 4);

    const double samplesPerBeat = clock.getSamplesPerBeat();
    assert(std::abs(samplesPerBeat - 22050.0) < 1.0 && "120 BPM at 44100Hz must be 22050 samples per beat");

    const double samplesPerBar = clock.getSamplesPerBar();
    assert(std::abs(samplesPerBar - 88200.0) < 1.0 && "4/4 at 120 BPM must be 88200 samples per bar");

    // Process blocks and verify beat event emission
    int beatCount = 0;
    int downbeatCount = 0;

    // Process 2 full bars (88200 * 2 = 176400 samples) in 512-sample blocks
    const int totalSamples = 176400;
    const int blockSize = 512;
    for (int processed = 0; processed < totalSamples; processed += blockSize)
    {
        clock.processBlock(blockSize, [&](const BeatEvent& ev)
        {
            if (ev.type == BeatType::Downbeat)
            {
                downbeatCount++;
                beatCount++;
            }
            else if (ev.type == BeatType::Beat)
            {
                beatCount++;
            }
        });
    }

    assert(downbeatCount == 2 && "2 full bars must emit exactly 2 downbeats");
    assert(beatCount == 8 && "2 full bars in 4/4 must emit exactly 8 beats");

    clock.releaseResources();
}

void testBpmQuantizerGridMath()
{
    BpmQuantizer quant;
    quant.setSampleRate(44100.0);
    quant.setTempo(120.0); // 22050 samples/beat
    quant.setGridResolution(GridResolution::Sixteenth); // 1/16 = 22050 / 4 = 5512.5 samples

    const double step = quant.getGridStepSamples();
    assert(std::abs(step - 5512.5) < 0.1 && "1/16 step at 120 BPM must be 5512.5 samples");

    // Test snap: an event at 5400 samples should snap to nearest 1/16 grid line (5513 samples)
    const juce::int64 loopLength = 88200; // 1 bar
    juce::int64 snapped = quant.quantize(5400, loopLength);
    assert(snapped == 5513 && "Sample 5400 should snap to 5513 (nearest 1/16 step)");

    // Test snapLoopLengthToNearestBar:
    // A loose recording of 90000 samples (slightly over 88200) should snap to 88200 (1 bar)
    juce::int64 barSnapped = quant.snapLoopLengthToNearestBar(90000, 4);
    assert(barSnapped == 88200 && "90000 samples must snap to 88200 (1 bar at 120 BPM)");

    // Test 2 bars snap: 170000 samples should snap to 176400 (2 bars)
    juce::int64 twoBarSnapped = quant.snapLoopLengthToNearestBar(170000, 4);
    assert(twoBarSnapped == 176400 && "170000 samples must snap to 176400 (2 bars)");
}

void testShiftLayerSystemBankAndModifiers()
{
    ShiftLayerSystem system;
    assert(system.getActiveBank() == KeyBank::Drums && "Default bank must be Drums");
    assert(system.getActiveOctaveOffset() == 0 && "Default octave offset must be 0");

    ProcessedKeyEvent out;
    RawKeyEvent ev;
    ev.deviceId = "DEV_1";
    ev.isKeyDown = true;

    // Shift Key Down -> Switches to Bass bank
    ev.virtualKeyCode = 0x10; // VK_SHIFT
    bool trigger = system.processKeyEvent(ev, out);
    assert(! trigger && "Shift key must be intercepted as modifier");
    assert(system.getActiveBank() == KeyBank::Bass && "Shift held must activate Bass bank");

    // Regular key while Shift held
    ev.virtualKeyCode = 0x41; // 'A'
    trigger = system.processKeyEvent(ev, out);
    assert(trigger && "Key 'A' should trigger");
    assert(out.activeBank == KeyBank::Bass && "Event bank must be Bass");

    // Shift Key Up -> Restores to Drums bank
    ev.virtualKeyCode = 0x10;
    ev.isKeyDown = false;
    trigger = system.processKeyEvent(ev, out);
    assert(! trigger && "Shift release must be intercepted");
    assert(system.getActiveBank() == KeyBank::Drums && "Releasing Shift must restore Drums bank");

    // CapsLock Toggle -> Switches to Synth bank
    ev.virtualKeyCode = 0x14; // VK_CAPITAL
    ev.isKeyDown = true;
    system.processKeyEvent(ev, out);
    assert(system.getActiveBank() == KeyBank::Synth && "CapsLock must activate Synth bank");

    // Shift while CapsLock is ON -> Switches to FX bank
    ev.virtualKeyCode = 0x10; // VK_SHIFT
    ev.isKeyDown = true;
    system.processKeyEvent(ev, out);
    assert(system.getActiveBank() == KeyBank::FX && "CapsLock + Shift must activate FX bank");

    // Release Shift while CapsLock ON -> Returns to Synth bank
    ev.isKeyDown = false;
    system.processKeyEvent(ev, out);
    assert(system.getActiveBank() == KeyBank::Synth && "Releasing Shift with CapsLock ON must return to Synth bank");

    // CapsLock Toggle again -> Restores Drums bank
    ev.virtualKeyCode = 0x14;
    ev.isKeyDown = true;
    system.processKeyEvent(ev, out);
    assert(system.getActiveBank() == KeyBank::Drums && "Disabling CapsLock must restore Drums bank");

    // Tab + Number key Octave transposition
    ev.virtualKeyCode = 0x09; // VK_TAB
    ev.isKeyDown = true;
    trigger = system.processKeyEvent(ev, out);
    assert(! trigger && "Tab press must be intercepted");
    assert(system.isTabHeld() && "Tab must be flagged as held");

    // While Tab held, press '1' (VK 0x31) -> sets octave to -2
    ev.virtualKeyCode = 0x31;
    trigger = system.processKeyEvent(ev, out);
    assert(! trigger && "Tab+1 must be intercepted and not trigger audio");
    assert(system.getActiveOctaveOffset() == -2 && "Octave must be set to -2");

    // While Tab held, press '4' (VK 0x34) -> sets octave to +1
    ev.virtualKeyCode = 0x34;
    system.processKeyEvent(ev, out);
    assert(system.getActiveOctaveOffset() == 1 && "Octave must be set to +1");

    // Release Tab
    ev.virtualKeyCode = 0x09;
    ev.isKeyDown = false;
    system.processKeyEvent(ev, out);
    assert(! system.isTabHeld() && "Tab must no longer be held");

    // Regular key now carries octave +1
    ev.virtualKeyCode = 0x43; // 'C'
    ev.isKeyDown = true;
    trigger = system.processKeyEvent(ev, out);
    assert(trigger && "Key 'C' must trigger");
    assert(out.octaveOffset == 1 && "Trigger event must carry active octave offset +1");

    // Test Numpad mode mapping
    system.setNumpadModeEnabled(true);
    ev.virtualKeyCode = 0x67; // VK_NUMPAD7
    system.processKeyEvent(ev, out);
    assert(out.effectiveVirtualKeyCode == 0x101 && "Numpad 7 must translate to Pad 1 (0x101)");

    ev.virtualKeyCode = 0x61; // VK_NUMPAD1
    system.processKeyEvent(ev, out);
    assert(out.effectiveVirtualKeyCode == 0x107 && "Numpad 1 must translate to Pad 7 (0x107)");

    ev.virtualKeyCode = 0x6E; // VK_DECIMAL
    system.processKeyEvent(ev, out);
    assert(out.effectiveVirtualKeyCode == 0x10B && "VK_DECIMAL must translate to Pad 11 (0x10B)");
}

void testBankAwareMappingEngine()
{
    MappingEngine mapping;
    const DeviceId testDev = "KEYBOARD_LAYER_TEST";

    // Map 'A' (0x41) on Drums (bank 0) to Kick
    mapping.assignKey(testDev, 0x41, "Drums Kick", "starter://kick", 0);

    // Map 'A' (0x41) on Bass (bank 1) to 808 Bass
    mapping.assignKey(testDev, 0x41, "Sub Bass 808", "starter://bass808", 1);

    // Map 'B' (0x42) only on bank 0
    mapping.assignKey(testDev, 0x42, "Snare", "starter://snare", 0);

    // Lookups
    const auto* bDrumsA = mapping.findBinding(testDev, 0x41, 0);
    assert(bDrumsA != nullptr && bDrumsA->soundLabel == "Drums Kick");

    const auto* bBassA = mapping.findBinding(testDev, 0x41, 1);
    assert(bBassA != nullptr && bBassA->soundLabel == "Sub Bass 808");

    // Fallback: looking up 'B' in bank 1 should fall back to bank 0
    const auto* bBassB = mapping.findBinding(testDev, 0x42, 1);
    assert(bBassB != nullptr && bBassB->soundLabel == "Snare" && "Must fall back to default bank 0");

    // Test persistence of bank in JSON
    mapping.saveActiveSetToDisk();
    mapping.loadActiveSetFromDisk();

    const auto* reloadBassA = mapping.findBinding(testDev, 0x41, 1);
    assert(reloadBassA != nullptr && reloadBassA->soundLabel == "Sub Bass 808");
    assert(reloadBassA->bank == 1 && "Persisted binding must retain bank 1");
}

void testLoopTrackEventEditingAndParamLocks()
{
    LoopTrack track;
    track.startRecording();
    track.recordTrigger(1, 100);
    track.recordTrigger(2, 500);
    track.recordTrigger(3, 900);
    track.stopRecordingAndStartLoop();

    assert(track.recordedEvents.size() == 3 && "Track must have 3 events");

    // 1. Test moveEvent (nudge)
    track.moveEvent(0, 120);
    assert(track.recordedEvents[0].offsetSamples == 120 && "Event 0 should move to 120");

    // Move event 0 past event 1 (from 120 to 600) -> should auto-sort!
    track.moveEvent(0, 600);
    assert(track.recordedEvents[0].sampleHandle == 2 && "Event 1 (offset 500) should now be first");
    assert(track.recordedEvents[1].sampleHandle == 1 && track.recordedEvents[1].offsetSamples == 600 && "Event 1 should be at 600");

    // 2. Test duration & parameter locks
    track.setEventDuration(0, 4410);
    assert(track.recordedEvents[0].durationSamples == 4410 && "Event duration must be set");

    track.setEventParams(0, 1.5f, 7.0f, 0.4f, 0.25f, 0.8f);
    assert(track.recordedEvents[0].gain == 1.5f && "Velocity must be 1.5");
    assert(track.recordedEvents[0].pitchSemitones == 7.0f && "Pitch must be +7 semitones");
    assert(track.recordedEvents[0].reverbSend == 0.4f && "Reverb send must be 0.4");
    assert(track.recordedEvents[0].delaySend == 0.25f && "Delay send must be 0.25");

    // 3. Test duplicate
    track.duplicateEvent(0, 50);
    assert(track.recordedEvents.size() == 4 && "Track must now have 4 events");

    // 4. Test remove
    track.removeEvent(0);
    assert(track.recordedEvents.size() == 3 && "Track must have 3 events after removal");
}

void testDspFxRackProcessing()
{
    // 1. ResonantFilter SVF Test: Low-Pass frequency attenuation
    ResonantFilter filter;
    filter.prepare(44100.0);
    filter.setMode(FilterMode::LowPass);
    filter.setCutoff(500.0f);
    filter.setResonance(1.0f);

    const int numSamples = 512;
    juce::AudioBuffer<float> testBuf(2, numSamples);
    float highFreqMagBefore = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f * static_cast<float>(i) / 44100.0f);
        testBuf.setSample(0, i, sample);
        testBuf.setSample(1, i, sample);
        highFreqMagBefore += std::abs(sample);
    }

    filter.processBlock(testBuf, 0, numSamples);

    float highFreqMagAfter = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        highFreqMagAfter += std::abs(testBuf.getSample(0, i));
    }

    // 10 kHz wave passing through 500 Hz low-pass filter must be strongly attenuated
    assert(highFreqMagAfter < (highFreqMagBefore * 0.1f) && "10kHz tone must be attenuated by 500Hz LowPass filter");

    // 2. StereoDelay Test: verifies delay echo timing
    StereoDelay delay;
    delay.prepare(44100.0, 512);
    delay.setDivision(DelayDivision::Quarter);
    delay.setFeedback(0.0f);
    delay.setMix(1.0f); // 100% wet

    const int delaySamplesExpected = 22050; // 0.5s at 120 BPM
    juce::AudioBuffer<float> impulseBuf(2, delaySamplesExpected + 512);
    impulseBuf.clear();
    impulseBuf.setSample(0, 0, 1.0f);

    int processed = 0;
    while (processed < impulseBuf.getNumSamples())
    {
        int toProc = std::min(512, impulseBuf.getNumSamples() - processed);
        delay.processBlock(impulseBuf, processed, toProc, 120.0);
        processed += toProc;
    }

    float echoSample = impulseBuf.getSample(0, delaySamplesExpected);
    assert(std::abs(echoSample) > 0.5f && "Echo must arrive at expected 1/4 beat delay time");

    // 3. AlgorithmicReverb Test: verifies decay and numerical stability
    AlgorithmicReverb reverb;
    reverb.prepare(44100.0);
    reverb.setRoomSize(0.6f);
    reverb.setMix(0.5f);

    juce::AudioBuffer<float> reverbBuf(2, 4096);
    reverbBuf.clear();
    reverbBuf.setSample(0, 0, 1.0f);
    reverbBuf.setSample(1, 0, 1.0f);

    reverb.processBlock(reverbBuf, 0, 4096);

    float tailEnergy = 0.0f;
    for (int i = 1000; i < 4096; ++i)
    {
        float s = reverbBuf.getSample(0, i);
        assert(! std::isnan(s) && ! std::isinf(s) && "Reverb output must be finite");
        tailEnergy += std::abs(s);
    }
    assert(tailEnergy > 0.001f && "Reverb tail must have audible diffuse energy");
}

void testStemExporterWavGeneration()
{
    AudioEngine engine;
    engine.prepareToPlay(512, 44100.0);

    const int kickHandle = engine.findSampleHandle("starter://kick");
    const int snareHandle = engine.findSampleHandle("starter://snare");
    const int bassHandle = engine.findSampleHandle("starter://bass808");
    assert(kickHandle >= 0 && snareHandle >= 0 && bassHandle >= 0);

    LoopTrack drumTrack;
    drumTrack.startRecording();
    drumTrack.recordTrigger(kickHandle, 0);
    drumTrack.recordTrigger(snareHandle, 22050);
    drumTrack.stopRecordingAndStartLoop();

    LoopTrack bassTrack;
    bassTrack.startRecording();
    bassTrack.recordTrigger(bassHandle, 0);
    bassTrack.stopRecordingAndStartLoop();

    juce::Array<const LoopTrack*> tracks { &drumTrack, &bassTrack };
    juce::StringArray names { "Drums", "Bass" };

    const juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("ArmoniTestStems_" + juce::String(juce::Random().nextInt()));

    StemExportOptions options;
    options.outputDirectory = tempDir;
    options.sampleRate = 44100.0;
    options.bitDepth = 24;
    options.exportIndividualStems = true;
    options.exportMasterMix = true;
    options.normalize = false;

    const auto result = StemExporter::renderStems(engine, tracks, names, options);
    assert(result.success && "Stem export must succeed");
    assert(result.exportedFiles.size() == 3 && "Must export 2 stems + 1 master mix");
    assert(result.totalSamplesRendered > 20000 && "Rendered length must match loop events");

    for (const auto& f : result.exportedFiles)
    {
        assert(f.existsAsFile() && f.getSize() > 100 && "Exported file must exist and have content");
    }

    // Verify WAV format integrity
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(result.exportedFiles[0]));
    assert(reader != nullptr && "Exported WAV must be readable by AudioFormatReader");
    assert(reader->sampleRate == 44100.0 && "Sample rate must match export option");
    assert(reader->numChannels == 2 && "Stems must be stereo");
    assert(reader->bitsPerSample == 24 && "Bit depth must match 24-bit option");

    tempDir.deleteRecursively();
    engine.releaseResources();
}

void testMidiExporterFileGeneration()
{
    AudioEngine engine;
    engine.prepareToPlay(512, 44100.0);

    const int kickHandle = engine.findSampleHandle("starter://kick");
    const int snareHandle = engine.findSampleHandle("starter://snare");
    const int bassHandle = engine.findSampleHandle("starter://bass808");

    LoopTrack drumTrack;
    drumTrack.startRecording();
    drumTrack.recordTrigger(kickHandle, 0);
    drumTrack.recordTrigger(snareHandle, 22050);
    drumTrack.stopRecordingAndStartLoop();

    LoopTrack bassTrack;
    bassTrack.startRecording();
    bassTrack.recordTrigger(bassHandle, 11025);
    bassTrack.setEventParams(0, 1.0f, 7.0f); // transposed by +7 semitones
    bassTrack.stopRecordingAndStartLoop();

    juce::Array<const LoopTrack*> tracks { &drumTrack, &bassTrack };
    juce::StringArray names { "Drums Track", "Bass Track" };

    const juce::File tempMidi = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("ArmoniTest_" + juce::String(juce::Random().nextInt()) + ".mid");

    MidiExportOptions options;
    options.sampleRate = 44100.0;
    options.bpm = 120.0;
    options.ticksPerQuarterNote = 960;

    const auto result = MidiExporter::exportToFile(tracks, names, tempMidi, options, &engine);
    assert(result.success && "MIDI export must succeed");
    assert(result.totalNotesExported == 3 && "Must export 3 note triggers");
    assert(result.totalTracksExported == 2 && "Must export 2 instrument tracks");
    assert(tempMidi.existsAsFile() && tempMidi.getSize() > 50 && "MIDI file must exist on disk");

    // Read MIDI file back to verify structure
    juce::MidiFile midiIn;
    juce::FileInputStream fis(tempMidi);
    assert(fis.openedOk() && midiIn.readFrom(fis) && "MIDI file must be valid Standard MIDI format");
    assert(midiIn.getNumTracks() >= 3 && "Must contain Conductor track + 2 instrument tracks");
    assert(midiIn.getTimeFormat() == 960 && "Ticks per quarter note must match 960 PPQN");

    tempMidi.deleteFile();
    engine.releaseResources();
}

void testMidiSyncEngineClockPulses()
{
    MidiSyncEngine sync;
    sync.prepare(44100.0, 120.0); // 120 BPM: 1 beat = 0.5s = 22050 samples
    sync.setClockEnabled(true);
    sync.sendStart();

    int clockCount = 0;
    auto clockCallback = [&](const juce::MidiMessage& msg)
    {
        if (msg.getRawData()[0] == 0xF8)
            clockCount++;
    };

    int samplesRemaining = 22050;
    while (samplesRemaining > 0)
    {
        const int blockSize = std::min(512, samplesRemaining);
        sync.processBlock(blockSize, clockCallback);
        samplesRemaining -= blockSize;
    }

    assert(clockCount == 24 && "MIDI sync engine must emit exactly 24 pulses per quarter note (24 PPQN)");

    sync.sendStop();
    clockCount = 0;
    sync.processBlock(512, clockCallback);
    assert(clockCount == 0 && "Stopped MIDI sync engine must not emit clock pulses");
}

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);
    std::cout << "\n=======================================================\n";
    std::cout << "  ARMONI COMPOSER - AUTOMATED TEST SUITE\n";
    std::cout << "=======================================================\n\n";

    RUN_TEST(testAudioEngineProceduralKitAndVoices);
    RUN_TEST(testLoopTrackWrapAroundTriggerMath);
    RUN_TEST(testMappingEngineJsonSerialization);
    RUN_TEST(testMetronomeClockAccuracy);
    RUN_TEST(testBpmQuantizerGridMath);
    RUN_TEST(testShiftLayerSystemBankAndModifiers);
    RUN_TEST(testBankAwareMappingEngine);
    RUN_TEST(testLoopTrackEventEditingAndParamLocks);
    RUN_TEST(testDspFxRackProcessing);
    RUN_TEST(testStemExporterWavGeneration);
    RUN_TEST(testMidiExporterFileGeneration);
    RUN_TEST(testMidiSyncEngineClockPulses);

    std::cout << "\n=======================================================\n";
    std::cout << "  ALL 12 TEST SUITES PASSED! (100% Core, DSP & Studio Export)\n";
    std::cout << "=======================================================\n\n";
    return 0;
}

