#include "audio/ClickEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tempoflow::audio
{
namespace
{
constexpr double twoPi = 6.28318530717958647692;
constexpr float silenceThreshold = 1.0e-5F;

struct ClickParameters
{
    double frequency = 0.0;
    float amplitude = 0.0F;
    double decaySeconds = 0.0;
};

ClickParameters parametersFor(ClickType type) noexcept
{
    switch (type)
    {
    case ClickType::accent:
        return {1'800.0, 0.80F, 0.030};
    case ClickType::normal:
        return {1'100.0, 0.55F, 0.025};
    case ClickType::high:
        return {2'600.0, 0.50F, 0.018};
    case ClickType::low:
        return {600.0, 0.60F, 0.035};
    case ClickType::wood:
        return {900.0, 0.65F, 0.012};
    case ClickType::mute:
        return {};
    }

    return {};
}

std::uint32_t nextNoise(std::uint32_t state) noexcept
{
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state == 0U ? 1U : state;
}
} // namespace

void SyntheticClickEngine::prepare(double sampleRate) noexcept
{
    reset();
    currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 0.0;
}

void SyntheticClickEngine::reset() noexcept
{
    for (auto &voice : voices)
        voice = {};

    nextNoiseSeed = 1;
}

void SyntheticClickEngine::trigger(ClickType type) noexcept
{
    if (type == ClickType::mute || currentSampleRate <= 0.0)
        return;

    const auto parameters = parametersFor(type);
    auto &voice = voiceForTrigger();
    const auto frequency = std::min(parameters.frequency, currentSampleRate * 0.45);
    const auto decaySamples = parameters.decaySeconds * currentSampleRate;

    voice.type = type;
    voice.phase = twoPi * 0.25;
    voice.phaseIncrement = twoPi * frequency / currentSampleRate;
    voice.amplitude = parameters.amplitude;
    voice.decay = decaySamples > 0.0 ? static_cast<float>(std::exp(std::log(0.001) / decaySamples)) : 0.0F;
    voice.noiseState = nextNoiseSeed;
    voice.active = true;

    nextNoiseSeed = nextNoise(nextNoiseSeed);
}

void SyntheticClickEngine::render(float *output, int sampleCount) noexcept
{
    if (output == nullptr || sampleCount <= 0)
        return;

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        float mixedSample = 0.0F;

        for (auto &voice : voices)
        {
            if (voice.active)
                mixedSample += renderVoice(voice);
        }

        output[sampleIndex] = std::clamp(output[sampleIndex] + mixedSample, -1.0F, 1.0F);
    }
}

SyntheticClickEngine::Voice &SyntheticClickEngine::voiceForTrigger() noexcept
{
    for (auto &voice : voices)
    {
        if (!voice.active)
            return voice;
    }

    return *std::min_element(voices.begin(), voices.end(),
                             [](const auto &left, const auto &right) { return left.amplitude < right.amplitude; });
}

float SyntheticClickEngine::renderVoice(Voice &voice) noexcept
{
    float waveform = 0.0F;

    if (voice.type == ClickType::wood)
    {
        voice.noiseState = nextNoise(voice.noiseState);
        const auto normalizedNoise = static_cast<float>(voice.noiseState) /
                                         static_cast<float>(std::numeric_limits<std::uint32_t>::max()) * 2.0F -
                                     1.0F;
        waveform = (0.70F * normalizedNoise) + (0.30F * static_cast<float>(std::sin(voice.phase)));
    }
    else
    {
        waveform = static_cast<float>(std::sin(voice.phase));
    }

    const auto sample = waveform * voice.amplitude;
    voice.phase += voice.phaseIncrement;
    if (voice.phase >= twoPi)
        voice.phase -= twoPi;

    voice.amplitude *= voice.decay;
    if (!std::isfinite(voice.amplitude) || voice.amplitude < silenceThreshold)
        voice.active = false;

    return sample;
}
} // namespace tempoflow::audio
