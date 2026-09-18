#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace tempoflow::audio
{
enum class ClickType
{
    accent,
    normal,
    high,
    low,
    wood,
    mute
};

class SyntheticClickEngine final
{
  public:
    static constexpr double maximumTailSeconds = 0.1;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void trigger(ClickType type) noexcept;
    void render(float *output, int sampleCount) noexcept;

  private:
    struct Voice
    {
        ClickType type = ClickType::mute;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        float amplitude = 0.0F;
        float decay = 0.0F;
        std::uint32_t noiseState = 1;
        bool active = false;
    };

    static constexpr std::size_t voiceCount = 64;

    [[nodiscard]] Voice &voiceForTrigger() noexcept;
    [[nodiscard]] float renderVoice(Voice &voice) noexcept;

    std::array<Voice, voiceCount> voices{};
    double currentSampleRate = 0.0;
    std::uint32_t nextNoiseSeed = 1;
};
} // namespace tempoflow::audio
