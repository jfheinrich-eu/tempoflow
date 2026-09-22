#include "plugin/PluginPresetState.h"

namespace tempoflow::plugin
{
RealtimePresetState::RealtimePresetState() noexcept
{
    clicks.fill(audio::ClickType::mute);
}

audio::ClickType RealtimePresetState::clickForBeat(int beatNumber) const noexcept
{
    if (beatNumber < 1 || static_cast<std::size_t>(beatNumber) > beatCount)
        return audio::ClickType::mute;

    return clicks[static_cast<std::size_t>(beatNumber - 1)];
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
    if (targetSlot < 0 || targetSlot >= slotCount)
        return;

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
