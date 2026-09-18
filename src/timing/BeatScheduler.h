#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace tempoflow::timing
{
struct HostTiming
{
    bool isPlaying = false;
    double bpm = 0.0;
    int timeSignatureNumerator = 0;
    int timeSignatureDenominator = 0;
    double ppqPosition = 0.0;
    double ppqPositionOfLastBarStart = 0.0;
    std::int64_t samplePosition = 0;
    double sampleRate = 0.0;
    int blockSize = 0;
};

struct ScheduledBeat
{
    int sampleOffset = 0;
    int beatNumber = 0;
    bool isBarStart = false;
};

class ScheduledBeatBuffer final
{
  public:
    static constexpr std::size_t capacity = 64;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool overflowed() const noexcept;
    [[nodiscard]] const ScheduledBeat &operator[](std::size_t index) const noexcept;

  private:
    friend class BeatScheduler;

    bool add(ScheduledBeat beat) noexcept;
    void markOverflow() noexcept;

    std::array<ScheduledBeat, capacity> beats{};
    std::size_t beatCount = 0;
    bool didOverflow = false;
};

struct BeatSchedule final
{
    ScheduledBeatBuffer beats;
    bool transportDiscontinuity = false;
    bool hostTimingValid = false;
};

class BeatScheduler final
{
  public:
    [[nodiscard]] BeatSchedule schedule(const HostTiming &timing) noexcept;
    void reset() noexcept;

  private:
    bool wasPlaying = false;
    std::int64_t expectedNextSamplePosition = 0;
    double expectedNextPpqPosition = 0.0;
};
} // namespace tempoflow::timing
