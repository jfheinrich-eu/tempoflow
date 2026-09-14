#include "plugin/PluginProcessor.h"

namespace tempoflow::plugin
{
TempoFlowAudioProcessor::TempoFlowAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::mono(), true))
{
}

void TempoFlowAudioProcessor::prepareToPlay(double, int)
{
}

void TempoFlowAudioProcessor::releaseResources()
{
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

const juce::String TempoFlowAudioProcessor::getProgramName(int)
{
    return {};
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
