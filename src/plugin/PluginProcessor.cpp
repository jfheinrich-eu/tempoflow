#include "plugin/PluginProcessor.h"

#include <cstring>
#include <optional>

namespace tempoflow::plugin
{
namespace
{
constexpr auto defaultPresetJson = R"json({
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": { "name": "Default" },
  "tempo": { "bpm": 120 },
  "meter": { "numerator": 4, "denominator": 4, "grouping": [1, 1, 1, 1] },
  "subdivision": { "mode": "none", "partsPerBeat": 1 },
  "pattern": {
    "beats": [
      { "beat": 1, "click": "accent" },
      { "beat": 2, "click": "normal" },
      { "beat": 3, "click": "normal" },
      { "beat": 4, "click": "normal" }
    ]
  },
  "sound": { "soundSet": "default", "volume": 1.0 },
  "playback": { "mode": "host" }
})json";

std::optional<timing::HostTiming> readHostTiming(const juce::AudioPlayHead::PositionInfo &position, double sampleRate,
                                                 int blockSize) noexcept
{
    const auto bpm = position.getBpm();
    const auto timeSignature = position.getTimeSignature();
    const auto ppqPosition = position.getPpqPosition();
    const auto lastBarStart = position.getPpqPositionOfLastBarStart();
    const auto samplePosition = position.getTimeInSamples();

    if (!bpm || !timeSignature || !ppqPosition || !lastBarStart || !samplePosition)
        return std::nullopt;

    return timing::HostTiming{position.getIsPlaying(),
                              *bpm,
                              timeSignature->numerator,
                              timeSignature->denominator,
                              *ppqPosition,
                              *lastBarStart,
                              *samplePosition,
                              sampleRate,
                              blockSize};
}

audio::ClickType toAudioClickType(preset::ClickType type) noexcept
{
    switch (type)
    {
    case preset::ClickType::accent:
        return audio::ClickType::accent;
    case preset::ClickType::normal:
        return audio::ClickType::normal;
    case preset::ClickType::high:
        return audio::ClickType::high;
    case preset::ClickType::low:
        return audio::ClickType::low;
    case preset::ClickType::wood:
        return audio::ClickType::wood;
    case preset::ClickType::mute:
        return audio::ClickType::mute;
    }

    return audio::ClickType::mute;
}

RealtimePresetState makeRealtimePresetState(const preset::RuntimePreset &preset) noexcept
{
    RealtimePresetState state;
    state.beatCount = preset.pattern.beats.size();
    state.volume = static_cast<float>(preset.sound.volume);

    for (const auto &beat : preset.pattern.beats)
    {
        jassert(beat.beat >= 1 && static_cast<std::uint64_t>(beat.beat) <= state.beatCount);
        state.clicks[static_cast<std::size_t>(beat.beat - 1)] = toAudioClickType(beat.click);
    }

    return state;
}

std::vector<juce::String> prefixErrors(const juce::String &prefix, const std::vector<juce::String> &errors)
{
    std::vector<juce::String> prefixedErrors;
    prefixedErrors.reserve(errors.size());
    for (const auto &error : errors)
        prefixedErrors.push_back(prefix + error);

    return prefixedErrors;
}
} // namespace

TempoFlowAudioProcessor::TempoFlowAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::mono(), true))
{
    const auto defaultResult = applyPresetJson(defaultPresetJson);
    jassert(defaultResult.isValid());
}

void TempoFlowAudioProcessor::prepareToPlay(double sampleRate, int)
{
    beatScheduler.reset();
    clickEngine.prepare(sampleRate);
}

void TempoFlowAudioProcessor::releaseResources()
{
    beatScheduler.reset();
    clickEngine.reset();
}

bool TempoFlowAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.inputBuses.isEmpty() && layouts.outputBuses.size() == 1 &&
           layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void TempoFlowAudioProcessor::processBlock(juce::AudioBuffer<float> &audio, juce::MidiBuffer &midi)
{
    const juce::ScopedNoDenormals noDenormals;
    audio.clear();
    midi.clear();

    if (presetStateExchange.consumeIfChanged(realtimePresetState, realtimePresetGeneration))
    {
        beatScheduler.reset();
        clickEngine.reset();
    }

    const auto *hostPlayHead = getPlayHead();
    const auto position = hostPlayHead != nullptr ? hostPlayHead->getPosition() : std::nullopt;

    if (position)
    {
        if (const auto timing = readHostTiming(*position, getSampleRate(), audio.getNumSamples()))
        {
            const auto schedule = beatScheduler.schedule(*timing);
            if (!schedule.hostTimingValid || schedule.beats.overflowed())
            {
                beatScheduler.reset();
                clickEngine.reset();
                return;
            }

            if (schedule.transportDiscontinuity)
                clickEngine.reset();

            auto *output = audio.getWritePointer(0);
            auto renderedSamples = 0;

            for (std::size_t index = 0; index < schedule.beats.size(); ++index)
            {
                const auto &beat = schedule.beats[index];
                clickEngine.render(output + renderedSamples, beat.sampleOffset - renderedSamples);
                clickEngine.trigger(realtimePresetState.clickForBeat(beat.beatNumber));
                renderedSamples = beat.sampleOffset;
            }

            clickEngine.render(output + renderedSamples, audio.getNumSamples() - renderedSamples);
            audio.applyGain(realtimePresetState.volume);
        }
        else
        {
            beatScheduler.reset();
            clickEngine.reset();
        }
    }
    else
    {
        beatScheduler.reset();
        clickEngine.reset();
    }
}

juce::AudioProcessorEditor *TempoFlowAudioProcessor::createEditor()
{
    return nullptr;
}

bool TempoFlowAudioProcessor::hasEditor() const
{
    return false;
}

const juce::String TempoFlowAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TempoFlowAudioProcessor::acceptsMidi() const
{
    return false;
}

bool TempoFlowAudioProcessor::producesMidi() const
{
    return false;
}

bool TempoFlowAudioProcessor::isMidiEffect() const
{
    return false;
}

double TempoFlowAudioProcessor::getTailLengthSeconds() const
{
    return audio::SyntheticClickEngine::maximumTailSeconds;
}

int TempoFlowAudioProcessor::getNumPrograms()
{
    return 1;
}

int TempoFlowAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TempoFlowAudioProcessor::setCurrentProgram(int)
{
}

const juce::String TempoFlowAudioProcessor::getProgramName(int index)
{
    return index == 0 ? juce::String("Default") : juce::String();
}

void TempoFlowAudioProcessor::changeProgramName(int, const juce::String &)
{
}

void TempoFlowAudioProcessor::getStateInformation(juce::MemoryBlock &destinationData)
{
    const std::lock_guard<std::mutex> lock(persistentStateMutex);
    destinationData.replaceAll(persistentPresetJson.toRawUTF8(), persistentPresetJson.getNumBytesAsUTF8());
}

void TempoFlowAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    try
    {
        if (data == nullptr || sizeInBytes <= 0 || sizeInBytes > preset::maximumPresetFileSizeBytes)
        {
            recordPresetErrors({"Plug-in state must contain 1 through 1048576 bytes"});
            return;
        }

        const auto *utf8Data = static_cast<const char *>(data);
        if (std::memchr(utf8Data, 0, static_cast<std::size_t>(sizeInBytes)) != nullptr ||
            !juce::CharPointer_UTF8::isValidString(utf8Data, sizeInBytes))
        {
            recordPresetErrors({"Plug-in state must contain valid UTF-8 JSON without embedded null bytes"});
            return;
        }

        static_cast<void>(applyPresetJson(juce::String::fromUTF8(utf8Data, sizeInBytes)));
    }
    catch (...)
    {
        stateRestoreException.store(true, std::memory_order_release);
    }
}

preset::RuntimePresetResult TempoFlowAudioProcessor::applyPresetJson(const juce::String &jsonText)
{
    auto result = preset::parsePresetJson(jsonText);
    if (!result.isValid())
    {
        recordPresetErrors(result.errors);
        return result;
    }

    const auto realtimeState = makeRealtimePresetState(result.preset);

    {
        const std::lock_guard<std::mutex> lock(persistentStateMutex);
        persistentPresetJson = jsonText;
        lastPresetErrors.clear();
        stateRestoreException.store(false, std::memory_order_release);
        presetStateExchange.publish(realtimeState);
    }

    return result;
}

preset::RuntimePresetResult TempoFlowAudioProcessor::loadPresetFile(const juce::File &file)
{
    const auto readResult = preset::readPresetFileBounded(file);
    if (!readResult.isValid())
    {
        preset::RuntimePresetResult result;
        result.errors.push_back(file.getFullPathName() + ": " + readResult.error);
        recordPresetErrors(result.errors);
        return result;
    }

    auto result = applyPresetJson(readResult.content);
    if (!result.isValid())
    {
        result.errors = prefixErrors(file.getFullPathName() + ": ", result.errors);
        recordPresetErrors(result.errors);
    }

    return result;
}

std::vector<juce::String> TempoFlowAudioProcessor::getLastPresetErrors() const
{
    const std::lock_guard<std::mutex> lock(persistentStateMutex);
    if (stateRestoreException.load(std::memory_order_acquire))
        return {"Plug-in state restoration failed unexpectedly"};

    return lastPresetErrors;
}

void TempoFlowAudioProcessor::recordPresetErrors(const std::vector<juce::String> &errors)
{
    const std::lock_guard<std::mutex> lock(persistentStateMutex);
    lastPresetErrors = errors;
    stateRestoreException.store(false, std::memory_order_release);
}
} // namespace tempoflow::plugin

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new tempoflow::plugin::TempoFlowAudioProcessor();
}
