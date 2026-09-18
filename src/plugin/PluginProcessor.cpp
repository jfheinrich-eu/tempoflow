#include "plugin/PluginProcessor.h"

#include <optional>

namespace tempoflow::plugin
{
namespace
{
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
} // namespace

TempoFlowAudioProcessor::TempoFlowAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::mono(), true))
{
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
                clickEngine.trigger(beat.isBarStart ? audio::ClickType::accent : audio::ClickType::normal);
                renderedSamples = beat.sampleOffset;
            }

            clickEngine.render(output + renderedSamples, audio.getNumSamples() - renderedSamples);
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
    destinationData.reset();
}

void TempoFlowAudioProcessor::setStateInformation(const void *, int)
{
}
} // namespace tempoflow::plugin

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new tempoflow::plugin::TempoFlowAudioProcessor();
}
