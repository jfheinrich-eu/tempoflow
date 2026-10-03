#include "plugin/PluginPresetState.h"

#include <algorithm>
#include <cmath>

namespace tempoflow::plugin
{
namespace
{
bool isSupportedDenominator(std::int64_t denominator) noexcept
{
    return denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16; // NOLINT
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
} // namespace

RealtimePresetState::RealtimePresetState() noexcept
{
    clicks.fill(audio::ClickType::mute);
    groupStarts.fill(false);
}

bool RealtimePresetState::isValid() const noexcept
{
    return ready && beatCount > 0 && beatCount <= maximumBeatCount && meterNumerator > 0 &&
           static_cast<std::uint64_t>(meterNumerator) == beatCount && isSupportedDenominator(meterDenominator) &&
           std::isfinite(volume) && volume >= 0.0F && volume <= 1.0F && groupStarts[0];
}

audio::ClickType RealtimePresetState::clickForBeat(int beatNumber, int hostNumerator,
                                                   int hostDenominator) const noexcept
{
    if (!isValid() || beatNumber < 1 || static_cast<std::size_t>(beatNumber) > beatCount)
        return audio::ClickType::mute;

    const auto beatIndex = static_cast<std::size_t>(beatNumber - 1);
    const auto click = clicks[beatIndex];
    const auto meterMatches = static_cast<std::int64_t>(hostNumerator) == meterNumerator &&
                              static_cast<std::int64_t>(hostDenominator) == meterDenominator;

    if (meterMatches && groupStarts[beatIndex] && click == audio::ClickType::normal)
        return audio::ClickType::accent;

    return click;
}

RealtimePresetState makeRealtimePresetState(const preset::RuntimePreset &preset) noexcept
{
    RealtimePresetState state;

    const auto beatCount = preset.pattern.beats.size();
    if (beatCount == 0 || beatCount > RealtimePresetState::maximumBeatCount || preset.meter.numerator <= 0 ||
        static_cast<std::uint64_t>(preset.meter.numerator) != beatCount ||
        !isSupportedDenominator(preset.meter.denominator) || !std::isfinite(preset.sound.volume) ||
        preset.sound.volume < 0.0 || preset.sound.volume > 1.0 || preset.meter.grouping.empty())
    {
        return state;
    }

    state.beatCount = beatCount;
    state.meterNumerator = preset.meter.numerator;
    state.meterDenominator = preset.meter.denominator;
    state.volume = static_cast<float>(preset.sound.volume);

    for (std::size_t index = 0; index < beatCount; ++index)
    {
        const auto &beat = preset.pattern.beats[index];
        if (beat.beat != static_cast<std::int64_t>(index + 1))
            return {};

        state.clicks[index] = toAudioClickType(beat.click);
    }

    state.groupStarts[0] = true;
    const auto allSingleton = std::all_of(preset.meter.grouping.begin(), preset.meter.grouping.end(),
                                          [](std::int64_t groupSize) { return groupSize == 1; });
    std::size_t consumedBeats = 0;
    for (const auto groupSize : preset.meter.grouping)
    {
        if (groupSize <= 0 || static_cast<std::uint64_t>(groupSize) > beatCount - consumedBeats)
            return {};

        consumedBeats += static_cast<std::size_t>(groupSize);
        if (!allSingleton && consumedBeats < beatCount)
            state.groupStarts[consumedBeats] = true;
    }

    if (consumedBeats != beatCount)
        return {};

    state.ready = true;
    return state;
}

void PluginPresetStateExchange::publish(const RealtimePresetState &state) noexcept
{
    const auto currentActiveSlot = activeSlot.load(std::memory_order_seq_cst);
    const auto currentReaderSlot = readerSlot.load(std::memory_order_seq_cst);

    auto targetSlot = -1;
    for (auto candidateSlot = 0; candidateSlot < slotCount; ++candidateSlot)
    {
        if (candidateSlot != currentActiveSlot && candidateSlot != currentReaderSlot)
        {
            targetSlot = candidateSlot;
            break;
        }
    }

    jassert(targetSlot >= 0 && targetSlot < slotCount);
    const auto nextGeneration = publishedGeneration.load(std::memory_order_relaxed) + 1;
    slots[static_cast<std::size_t>(targetSlot)] = {state, nextGeneration};
    activeSlot.store(targetSlot, std::memory_order_seq_cst);
    publishedGeneration.store(nextGeneration, std::memory_order_release);
}

bool PluginPresetStateExchange::consumeIfChanged(RealtimePresetState &state, std::uint64_t &lastGeneration) noexcept
{
    if (publishedGeneration.load(std::memory_order_acquire) == lastGeneration)
        return false;

    for (auto attempt = 0; attempt < slotCount; ++attempt)
    {
        const auto candidateSlot = activeSlot.load(std::memory_order_seq_cst);
        readerSlot.store(candidateSlot, std::memory_order_seq_cst);

        if (activeSlot.load(std::memory_order_seq_cst) != candidateSlot)
        {
            readerSlot.store(-1, std::memory_order_seq_cst);
            continue;
        }

        const auto &slot = slots[static_cast<std::size_t>(candidateSlot)];
        state = slot.state;
        lastGeneration = slot.generation;
        readerSlot.store(-1, std::memory_order_seq_cst);
        return true;
    }

    readerSlot.store(-1, std::memory_order_seq_cst);
    return false;
}
} // namespace tempoflow::plugin
