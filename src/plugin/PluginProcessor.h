#pragma once

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

namespace tempoflow::plugin
{
class TempoFlowAudioProcessor final : public juce::AudioProcessor
{
  public:
    TempoFlowAudioProcessor();
    ~TempoFlowAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    [[nodiscard]] bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &audio, juce::MidiBuffer &midi) override;

    [[nodiscard]] juce::AudioProcessorEditor *createEditor() override;
    [[nodiscard]] bool hasEditor() const override;

    [[nodiscard]] const juce::String getName() const override;
    [[nodiscard]] bool acceptsMidi() const override;
    [[nodiscard]] bool producesMidi() const override;
    [[nodiscard]] bool isMidiEffect() const override;
    [[nodiscard]] double getTailLengthSeconds() const override;

    [[nodiscard]] int getNumPrograms() override;
    [[nodiscard]] int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    [[nodiscard]] const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destinationData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoFlowAudioProcessor)
};
} // namespace tempoflow::plugin

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter();
