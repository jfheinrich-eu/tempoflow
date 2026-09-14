#include "plugin/PluginProcessor.h"

#include <array>
#include <iostream>
#include <string_view>

namespace
{
bool expect(bool condition, std::string_view message)
{
    if (condition)
        return true;

    std::cerr << message << '\n';
    return false;
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
} // namespace

int main()
{
    const bool passed = testIdentityAndCapabilities() && testBusLayout() && testSilentProcessing();

    if (!passed)
        return 1;

    std::cout << "TempoFlow plug-in processor tests passed\n";
    return 0;
}
