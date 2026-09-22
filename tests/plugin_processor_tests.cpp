#include "plugin/PluginProcessor.h"
#include "preset/PresetFileIO.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace
{
const juce::String statePreset = R"json(
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": { "name": "State Test" },
  "tempo": { "bpm": 120 },
  "meter": { "numerator": 4, "denominator": 4, "grouping": [1, 1, 1, 1] },
  "subdivision": { "mode": "none", "partsPerBeat": 1 },
  "pattern": {
    "beats": [
      { "beat": 1, "click": "mute" },
      { "beat": 2, "click": "wood" },
      { "beat": 3, "click": "high" },
      { "beat": 4, "click": "low" }
    ]
  },
  "sound": { "soundSet": "default", "volume": 0.25 },
  "playback": { "mode": "host" },
  "futureOptional": { "preserved": true }
}
)json";

bool expect(bool condition, std::string_view message)
{
    if (condition)
        return true;

    std::cerr << message << '\n';
    return false;
}

class TestPlayHead final : public juce::AudioPlayHead
{
  public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        return position;
    }

    PositionInfo position;
};

void setPlayingPosition(TestPlayHead &playHead, double ppqPosition, std::int64_t samplePosition, int numerator = 4,
                        int denominator = 4)
{
    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);
    playHead.position.setTimeSignature(juce::AudioPlayHead::TimeSignature{numerator, denominator});
    playHead.position.setPpqPosition(ppqPosition);
    playHead.position.setPpqPositionOfLastBarStart(0.0);
    playHead.position.setTimeInSamples(samplePosition);
}

juce::String getSerializedState(tempoflow::plugin::TempoFlowAudioProcessor &processor)
{
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    if (state.getSize() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {};

    return juce::String::fromUTF8(static_cast<const char *>(state.getData()), static_cast<int>(state.getSize()));
}

float renderPresetBeat(const juce::String &presetJson, int beatNumber)
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    const auto result = processor.applyPresetJson(presetJson);
    if (!result.isValid())
        return -1.0F;

    TestPlayHead playHead;
    constexpr auto sampleRate = 48'000.0;
    constexpr auto sampleCount = 64;
    constexpr auto samplesPerBeat = 24'000;
    setPlayingPosition(playHead, static_cast<double>(beatNumber - 1),
                       static_cast<std::int64_t>(beatNumber - 1) * samplesPerBeat);

    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleCount);
    processor.prepareToPlay(sampleRate, sampleCount);

    juce::AudioBuffer<float> audio(1, sampleCount);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);
    processor.releaseResources();
    return audio.getMagnitude(0, sampleCount);
}

bool testIdentityAndCapabilities()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    bool passed = true;

    passed &= expect(processor.getName() == "TempoFlow", "Unexpected plug-in name");
    passed &= expect(!processor.acceptsMidi(), "The scaffold must not accept MIDI");
    passed &= expect(!processor.producesMidi(), "The scaffold must not produce MIDI");
    passed &= expect(!processor.isMidiEffect(), "The scaffold must not be a MIDI effect");
    passed &= expect(!processor.hasEditor(), "The scaffold must not expose a custom editor");
    passed &= expect(processor.createEditor() == nullptr, "The scaffold editor must be null");
    passed &= expect(processor.getNumPrograms() == 1, "The scaffold must expose one factory program");
    passed &= expect(processor.getProgramName(0) == "Default", "The factory program must have a stable non-empty name");
    passed &= expect(processor.getTailLengthSeconds() == tempoflow::audio::SyntheticClickEngine::maximumTailSeconds,
                     "The plug-in must report the synthetic click tail");
    return passed;
}

bool testBusLayout()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    bool passed = true;

    passed &= expect(processor.getTotalNumInputChannels() == 0, "The scaffold must have no input channels");
    passed &= expect(processor.getTotalNumOutputChannels() == 1, "The scaffold must have one output channel");

    juce::AudioProcessor::BusesLayout monoOutput;
    monoOutput.outputBuses.add(juce::AudioChannelSet::mono());
    passed &= expect(processor.isBusesLayoutSupported(monoOutput), "A mono output must be supported");

    juce::AudioProcessor::BusesLayout stereoOutput;
    stereoOutput.outputBuses.add(juce::AudioChannelSet::stereo());
    passed &= expect(!processor.isBusesLayoutSupported(stereoOutput), "A stereo output must be rejected");

    juce::AudioProcessor::BusesLayout monoInputAndOutput;
    monoInputAndOutput.inputBuses.add(juce::AudioChannelSet::mono());
    monoInputAndOutput.outputBuses.add(juce::AudioChannelSet::mono());
    passed &= expect(!processor.isBusesLayoutSupported(monoInputAndOutput), "An input bus must be rejected");
    return passed;
}

bool testSilentProcessing()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    constexpr int sampleCount = 64;

    processor.prepareToPlay(48'000.0, sampleCount);

    std::array<float, sampleCount> inputSamples{};
    inputSamples.fill(0.5F);

    juce::AudioBuffer<float> audio(1, sampleCount);
    audio.copyFrom(0, 0, inputSamples.data(), sampleCount);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100)), 0);
    processor.processBlock(audio, midi);
    processor.releaseResources();

    return expect(audio.getMagnitude(0, sampleCount) == 0.0F, "The scaffold must output silence") &&
           expect(midi.isEmpty(), "The scaffold must not pass MIDI through");
}

bool testScheduledClickStartsAtTheExactSample()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    TestPlayHead playHead;
    constexpr auto sampleRate = 48'000.0;
    constexpr auto sampleCount = 64;
    constexpr auto clickOffset = 32;
    constexpr auto ppqPerSample = 120.0 / (60.0 * sampleRate);
    setPlayingPosition(playHead, 1.0 - (clickOffset * ppqPerSample), 1'000);

    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleCount);
    processor.prepareToPlay(sampleRate, sampleCount);

    juce::AudioBuffer<float> audio(1, sampleCount);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);
    processor.releaseResources();

    const auto *samples = audio.getReadPointer(0);
    const auto silentBeforeClick =
        std::all_of(samples, samples + clickOffset, [](float sample) { return sample == 0.0F; });

    return expect(silentBeforeClick, "Audio before the scheduled click must remain silent") &&
           expect(std::abs(samples[clickOffset]) > 0.1F, "The click must start at its scheduled sample offset");
}

bool testStoppedTransportClearsClickTail()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    TestPlayHead playHead;
    constexpr auto sampleRate = 48'000.0;
    constexpr auto sampleCount = 64;
    setPlayingPosition(playHead, 0.0, 0);

    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleCount);
    processor.prepareToPlay(sampleRate, sampleCount);

    juce::AudioBuffer<float> playingAudio(1, sampleCount);
    juce::MidiBuffer midi;
    processor.processBlock(playingAudio, midi);

    playHead.position.setIsPlaying(false);
    playHead.position.setPpqPosition(sampleCount * 120.0 / (60.0 * sampleRate));
    playHead.position.setTimeInSamples(sampleCount);
    juce::AudioBuffer<float> stoppedAudio(1, sampleCount);
    processor.processBlock(stoppedAudio, midi);
    processor.releaseResources();

    return expect(playingAudio.getMagnitude(0, sampleCount) > 0.1F, "Playing transport must produce a click") &&
           expect(stoppedAudio.getMagnitude(0, sampleCount) == 0.0F, "Stopped transport must clear click tails");
}

bool testSchedulerOverflowProducesSilence()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    TestPlayHead playHead;
    constexpr auto sampleRate = 100.0;
    constexpr auto sampleCount = 1'000;
    setPlayingPosition(playHead, 0.0, 0);
    playHead.position.setBpm(300.0);
    playHead.position.setTimeSignature(juce::AudioPlayHead::TimeSignature{4, 16});

    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleCount);
    processor.prepareToPlay(sampleRate, sampleCount);

    juce::AudioBuffer<float> audio(1, sampleCount);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);
    processor.releaseResources();

    return expect(audio.getMagnitude(0, sampleCount) == 0.0F,
                  "A block that exceeds the scheduler capacity must remain silent");
}

bool testPresetPatternAndMasterVolumeAreApplied()
{
    const auto mutedMagnitude = renderPresetBeat(statePreset, 1);
    const auto quarterVolumeMagnitude = renderPresetBeat(statePreset, 2);
    const auto fullVolumeMagnitude = renderPresetBeat(statePreset.replace("\"volume\": 0.25", "\"volume\": 1.0"), 2);

    return expect(mutedMagnitude == 0.0F, "A preset mute role must produce silence") &&
           expect(quarterVolumeMagnitude > 0.0F, "An audible preset role must produce audio") &&
           expect(fullVolumeMagnitude > quarterVolumeMagnitude, "Master volume must change the rendered level") &&
           expect(std::abs(quarterVolumeMagnitude - (fullVolumeMagnitude * 0.25F)) < 0.0001F,
                  "Master volume must scale the rendered level linearly");
}

bool testUndefinedHostBeatRemainsSilent()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    if (!processor.applyPresetJson(statePreset).isValid())
        return expect(false, "The host-meter test preset must be applied");

    TestPlayHead playHead;
    constexpr auto sampleRate = 48'000.0;
    constexpr auto sampleCount = 64;
    setPlayingPosition(playHead, 2.0, 48'000, 6, 8);

    processor.setPlayHead(&playHead);
    processor.setRateAndBufferSizeDetails(sampleRate, sampleCount);
    processor.prepareToPlay(sampleRate, sampleCount);

    juce::AudioBuffer<float> audio(1, sampleCount);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);
    processor.releaseResources();

    return expect(audio.getMagnitude(0, sampleCount) == 0.0F,
                  "Host beats not defined by the preset pattern must remain silent");
}

bool testInvalidPresetLeavesActiveStateUnchanged()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    const auto applied = processor.applyPresetJson(statePreset);
    const auto stateBeforeFailure = getSerializedState(processor);

    const auto invalidResult = processor.applyPresetJson(statePreset.replace("\"volume\": 0.25", "\"volume\": 2.0"));
    const auto stateAfterInvalidPreset = getSerializedState(processor);
    constexpr std::array invalidUtf8{static_cast<char>(0xc3), static_cast<char>(0x28)};
    processor.setStateInformation(invalidUtf8.data(), static_cast<int>(invalidUtf8.size()));
    const auto stateAfterInvalidHostState = getSerializedState(processor);
    const auto errors = processor.getLastPresetErrors();

    return expect(applied.isValid(), "The valid test preset must be applied") &&
           expect(!invalidResult.isValid(), "An invalid preset must be rejected") &&
           expect(stateAfterInvalidPreset == stateBeforeFailure, "An invalid preset must not modify active state") &&
           expect(stateAfterInvalidHostState == stateBeforeFailure,
                  "Invalid host state must not modify active state") &&
           expect(!errors.empty(), "A failed state restoration must retain a diagnostic error");
}

bool testInvalidStatePayloadsAreRejected()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    const auto stateBeforeFailure = getSerializedState(processor);

    constexpr std::array stateWithNullByte{'{', '}', '\0'};
    processor.setStateInformation(stateWithNullByte.data(), static_cast<int>(stateWithNullByte.size()));
    const auto stateAfterNullByte = getSerializedState(processor);

    std::vector<char> oversizedState(static_cast<std::size_t>(tempoflow::preset::maximumPresetFileSizeBytes) + 1, ' ');
    processor.setStateInformation(oversizedState.data(), static_cast<int>(oversizedState.size()));
    const auto stateAfterOversizedPayload = getSerializedState(processor);

    processor.setStateInformation(nullptr, 0);
    const auto stateAfterEmptyPayload = getSerializedState(processor);

    return expect(stateAfterNullByte == stateBeforeFailure, "State containing a null byte must be rejected") &&
           expect(stateAfterOversizedPayload == stateBeforeFailure, "Oversized state must be rejected") &&
           expect(stateAfterEmptyPayload == stateBeforeFailure, "Empty state must be rejected") &&
           expect(!processor.getLastPresetErrors().empty(), "Rejected state must retain a diagnostic error");
}

bool testProjectStateRoundTripPreservesPreset()
{
    tempoflow::plugin::TempoFlowAudioProcessor source;
    if (!source.applyPresetJson(statePreset).isValid())
        return expect(false, "The source preset must be applied before state serialization");

    juce::MemoryBlock serializedState;
    source.getStateInformation(serializedState);

    tempoflow::plugin::TempoFlowAudioProcessor restored;
    restored.setStateInformation(serializedState.getData(), static_cast<int>(serializedState.getSize()));

    return expect(getSerializedState(restored) == statePreset,
                  "Project state must preserve the complete preset JSON") &&
           expect(restored.getLastPresetErrors().empty(), "A valid project state must restore without errors") &&
           expect(renderPresetBeat(getSerializedState(restored), 1) == 0.0F,
                  "Restored project state must preserve the preset pattern");
}

bool testFileErrorsIdentifyThePresetAndPreserveState()
{
    tempoflow::plugin::TempoFlowAudioProcessor processor;
    const auto stateBeforeFailure = getSerializedState(processor);
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getNonexistentChildFile("tempoflow-plugin-state", {}, true);
    if (directory.createDirectory().failed())
        return expect(false, "The plug-in state test directory must be created");

    const auto file = directory.getChildFile("invalid.tempoflow");
    const auto invalidPreset = statePreset.replace("\"volume\": 0.25", "\"volume\": 2.0");
    const auto created = file.replaceWithData(invalidPreset.toRawUTF8(), invalidPreset.getNumBytesAsUTF8());
    const auto result = created ? processor.loadPresetFile(file) : tempoflow::preset::RuntimePresetResult{};
    const auto removed = directory.deleteRecursively(false);

    return expect(created, "The invalid preset fixture must be created") &&
           expect(!result.isValid() && !result.errors.empty(), "The invalid preset file must be rejected") &&
           expect(result.errors.front().startsWith(file.getFullPathName() + ": "),
                  "Preset file errors must identify the source file") &&
           expect(getSerializedState(processor) == stateBeforeFailure,
                  "A failed preset file load must preserve active state") &&
           expect(removed, "The plug-in state test directory must be removable");
}

bool testAllReferencePresetsLoadIntoTheProcessor()
{
    const auto collection = tempoflow::preset::collectPresetFiles({juce::File(TEMPOFLOW_REFERENCE_PRESET_DIR)});
    bool passed = expect(collection.isValid(), "Reference presets must be discovered without file errors") &&
                  expect(collection.files.size() == 7, "All seven reference presets must be discovered");

    tempoflow::plugin::TempoFlowAudioProcessor processor;
    for (const auto &file : collection.files)
    {
        const auto result = processor.loadPresetFile(file);
        passed &= expect(result.isValid(), "Every reference preset must load into the plug-in processor");
    }

    return passed && expect(processor.getLastPresetErrors().empty(),
                            "The final valid reference preset must leave no diagnostic errors");
}
} // namespace

int main()
{
    const bool passed = testIdentityAndCapabilities() && testBusLayout() && testSilentProcessing() &&
                        testScheduledClickStartsAtTheExactSample() && testStoppedTransportClearsClickTail() &&
                        testSchedulerOverflowProducesSilence() && testPresetPatternAndMasterVolumeAreApplied() &&
                        testUndefinedHostBeatRemainsSilent() && testInvalidPresetLeavesActiveStateUnchanged() &&
                        testProjectStateRoundTripPreservesPreset() && testInvalidStatePayloadsAreRejected() &&
                        testFileErrorsIdentifyThePresetAndPreserveState() &&
                        testAllReferencePresetsLoadIntoTheProcessor();

    if (!passed)
        return 1;

    std::cout << "TempoFlow plug-in processor tests passed\n";
    return 0;
}
