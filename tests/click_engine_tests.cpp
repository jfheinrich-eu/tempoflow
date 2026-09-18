#include "audio/ClickEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
using tempoflow::audio::ClickType;
using tempoflow::audio::SyntheticClickEngine;

constexpr int sampleCount = 2'048;

bool expect(bool condition, std::string_view message)
{
    if (condition)
        return true;

    std::cerr << message << '\n';
    return false;
}

std::array<float, sampleCount> renderClick(ClickType type)
{
    SyntheticClickEngine engine;
    std::array<float, sampleCount> output{};
    engine.prepare(48'000.0);
    engine.trigger(type);
    engine.render(output.data(), static_cast<int>(output.size()));
    return output;
}

template <std::size_t size> float magnitude(const std::array<float, size> &samples)
{
    float result = 0.0F;
    for (const auto sample : samples)
        result = std::max(result, std::abs(sample));
    return result;
}

bool testRolesAreAudibleAndDistinct()
{
    constexpr std::array audibleRoles{ClickType::accent, ClickType::normal, ClickType::high, ClickType::low,
                                      ClickType::wood};
    std::array<std::array<float, sampleCount>, audibleRoles.size()> outputs{};
    bool passed = true;

    for (std::size_t index = 0; index < audibleRoles.size(); ++index)
    {
        outputs[index] = renderClick(audibleRoles[index]);
        passed &= expect(magnitude(outputs[index]) > 0.1F, "Every audible role must produce a signal");
    }

    for (std::size_t left = 0; left < outputs.size(); ++left)
    {
        for (std::size_t right = left + 1; right < outputs.size(); ++right)
        {
            float difference = 0.0F;
            for (std::size_t sample = 0; sample < outputs[left].size(); ++sample)
                difference += std::abs(outputs[left][sample] - outputs[right][sample]);

            passed &= expect(difference > 1.0F, "Click roles must have distinguishable waveforms");
        }
    }

    return passed;
}

bool testMuteProducesSilence()
{
    return expect(magnitude(renderClick(ClickType::mute)) == 0.0F, "Mute must produce silence");
}

bool testUnsupportedRoleProducesSilence()
{
    const auto unsupportedRole = static_cast<ClickType>(999);
    return expect(magnitude(renderClick(unsupportedRole)) == 0.0F, "An unsupported click role must produce silence");
}

bool testTriggerStartsAtTheCurrentSample()
{
    SyntheticClickEngine engine;
    std::array<float, 32> before{};
    std::array<float, 32> after{};
    engine.prepare(48'000.0);
    engine.render(before.data(), static_cast<int>(before.size()));
    engine.trigger(ClickType::accent);
    engine.render(after.data(), static_cast<int>(after.size()));

    return expect(std::all_of(before.begin(), before.end(), [](float sample) { return sample == 0.0F; }),
                  "Samples before a trigger must remain silent") &&
           expect(std::abs(after.front()) > 0.1F, "A click must begin on the first sample after its trigger");
}

bool testTailContinuesAcrossBlocksAndResetStopsIt()
{
    SyntheticClickEngine engine;
    std::array<float, 8> firstBlock{};
    std::array<float, 64> secondBlock{};
    std::array<float, 64> afterReset{};
    engine.prepare(48'000.0);
    engine.trigger(ClickType::low);
    engine.render(firstBlock.data(), static_cast<int>(firstBlock.size()));
    engine.render(secondBlock.data(), static_cast<int>(secondBlock.size()));
    engine.reset();
    engine.render(afterReset.data(), static_cast<int>(afterReset.size()));

    return expect(magnitude(secondBlock) > 0.0F, "A click tail must continue into the next block") &&
           expect(magnitude(afterReset) == 0.0F, "Reset must stop all active voices");
}

bool testOutputIsFiniteAndBounded()
{
    SyntheticClickEngine engine;
    std::array<float, 512> output{};
    engine.prepare(48'000.0);

    for (int trigger = 0; trigger < 128; ++trigger)
        engine.trigger(ClickType::accent);

    engine.render(output.data(), static_cast<int>(output.size()));

    return expect(std::all_of(output.begin(), output.end(),
                              [](float sample) { return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F; }),
                  "Mixed click output must remain finite and bounded");
}

bool testInvalidPreparationStaysSilent()
{
    SyntheticClickEngine engine;
    std::array<float, 32> output{};
    engine.prepare(0.0);
    engine.trigger(ClickType::accent);
    engine.render(output.data(), static_cast<int>(output.size()));
    engine.render(nullptr, 32);
    engine.render(output.data(), 0);

    return expect(magnitude(output) == 0.0F, "An invalid sample rate must keep the engine silent");
}
} // namespace

int main()
{
    const bool passed = testRolesAreAudibleAndDistinct() && testMuteProducesSilence() &&
                        testUnsupportedRoleProducesSilence() && testTriggerStartsAtTheCurrentSample() &&
                        testTailContinuesAcrossBlocksAndResetStopsIt() && testOutputIsFiniteAndBounded() &&
                        testInvalidPreparationStaysSilent();

    if (!passed)
        return 1;

    std::cout << "TempoFlow click engine tests passed\n";
    return 0;
}
