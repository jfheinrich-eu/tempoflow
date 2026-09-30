#include "plugin/PluginPresetState.h"

#include <cmath>

namespace tempoflow::plugin
{
namespace
{
bool isSupportedDenominator(std::int64_t denominator) noexcept
{
    return denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16; // NOLINT
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
