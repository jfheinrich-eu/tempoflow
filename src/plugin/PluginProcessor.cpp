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

void TempoFlowAudioProcessor::prepareToPlay(double, int)
{
    beatScheduler.reset();
}

void TempoFlowAudioProcessor::releaseResources()
{
    beatScheduler.reset();
}

bool TempoFlowAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.inputBuses.isEmpty() && layouts.outputBuses.size() == 1 &&
           layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void TempoFlowAudioProcessor::processBlock(juce::AudioBuffer<float> &audio, juce::MidiBuffer &midi)
{
    const juce::ScopedNoDenormals noDenormals;

    const auto *hostPlayHead = getPlayHead();
    const auto position = hostPlayHead != nullptr ? hostPlayHead->getPosition() : std::nullopt;

    if (position)
    {
        if (const auto timing = readHostTiming(*position, getSampleRate(), audio.getNumSamples()))
        {
            [[maybe_unused]] const auto scheduledBeats = beatScheduler.schedule(*timing);
        }
        else
        {
            beatScheduler.reset();
        }
    }
    else
    {
        beatScheduler.reset();
    }

    audio.clear();
    midi.clear();
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
    return 0.0;
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
