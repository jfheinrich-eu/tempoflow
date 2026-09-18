#include "plugin/PluginProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

class TestPlayHead final : public juce::AudioPlayHead
{
  public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        return position;
    }

    PositionInfo position;
};

void setPlayingPosition(TestPlayHead &playHead, double ppqPosition, std::int64_t samplePosition)
{
    playHead.position.setIsPlaying(true);
    playHead.position.setBpm(120.0);
    playHead.position.setTimeSignature(juce::AudioPlayHead::TimeSignature{4, 4});
    playHead.position.setPpqPosition(ppqPosition);
    playHead.position.setPpqPositionOfLastBarStart(0.0);
    playHead.position.setTimeInSamples(samplePosition);
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
} // namespace

int main()
{
    const bool passed = testIdentityAndCapabilities() && testBusLayout() && testSilentProcessing() &&
                        testScheduledClickStartsAtTheExactSample() && testStoppedTransportClearsClickTail() &&
                        testSchedulerOverflowProducesSilence();

    if (!passed)
        return 1;

    std::cout << "TempoFlow plug-in processor tests passed\n";
    return 0;
}
