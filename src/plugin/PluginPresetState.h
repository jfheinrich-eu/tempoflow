#pragma once

#include "audio/ClickEngine.h"
#include "preset/PresetValidator.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace tempoflow::plugin
{
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

struct RealtimePresetState final
{
    static constexpr std::size_t maximumBeatCount = preset::maximumJsonArrayElements;

    RealtimePresetState() noexcept;
    [[nodiscard]] audio::ClickType clickForBeat(int beatNumber) const noexcept;

    std::array<audio::ClickType, maximumBeatCount> clicks{};
    std::size_t beatCount = 0;
    float volume = 0.0F;
};

class PluginPresetStateExchange final
{
  public:
    void publish(const RealtimePresetState &state) noexcept;
    [[nodiscard]] bool consumeIfChanged(RealtimePresetState &state, std::uint64_t &lastGeneration) noexcept;

  private:
    struct Slot final
    {
        RealtimePresetState state;
        std::uint64_t generation = 0;
    };

    static constexpr int slotCount = 3;

    std::array<Slot, slotCount> slots{};
    std::atomic<int> activeSlot{0};
    std::atomic<int> readerSlot{-1};
    std::atomic<std::uint64_t> publishedGeneration{0};
};
} // namespace tempoflow::plugin
