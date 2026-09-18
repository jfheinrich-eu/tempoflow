#include "timing/BeatScheduler.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace tempoflow::timing
{
namespace
{
constexpr double positionTolerance = 1.0e-9;
constexpr double sampleTolerance = 1.0e-7;

bool isSupportedDenominator(int denominator) noexcept
{
    return denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16; // NOLINT
}

bool isValid(const HostTiming &timing) noexcept
{
    return timing.isPlaying && std::isfinite(timing.bpm) && timing.bpm > 0.0 && timing.timeSignatureNumerator > 0 &&
           isSupportedDenominator(timing.timeSignatureDenominator) && std::isfinite(timing.ppqPosition) &&
           std::isfinite(timing.ppqPositionOfLastBarStart) &&
           timing.ppqPositionOfLastBarStart <= timing.ppqPosition + positionTolerance &&
           std::isfinite(timing.sampleRate) && timing.sampleRate > 0.0 && timing.blockSize > 0;
}

bool tryCeilToInt64(double value, std::int64_t &result) noexcept
{
    if (!std::isfinite(value))
    {
        return false;
    }

    const auto nearestInteger = std::round(value);
    const auto adjusted = std::abs(value - nearestInteger) <= positionTolerance ? nearestInteger : std::ceil(value);

    const auto extended = static_cast<long double>(adjusted);
    if (extended < static_cast<long double>(std::numeric_limits<std::int64_t>::min()) ||
        extended > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
    {
        return false;
    }

    result = static_cast<std::int64_t>(adjusted);
    return true;
}

bool trySampleOffset(double value, int &result) noexcept
{
    if (!std::isfinite(value))
    {
        return false;
    }

    const auto nearestInteger = std::round(value);
    const auto adjusted = std::abs(value - nearestInteger) <= sampleTolerance ? nearestInteger : std::ceil(value);

    if (adjusted < static_cast<double>(std::numeric_limits<int>::min()) ||
        adjusted > static_cast<double>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    result = static_cast<int>(adjusted);
    return true;
}

int positiveModulo(std::int64_t value, int divisor) noexcept
{
    const auto remainder = value % divisor;
    return static_cast<int>(remainder < 0 ? remainder + divisor : remainder);
}
} // namespace

std::size_t ScheduledBeatBuffer::size() const noexcept
{
    return beatCount;
}

bool ScheduledBeatBuffer::empty() const noexcept
{
    return beatCount == 0;
}

bool ScheduledBeatBuffer::overflowed() const noexcept
{
    return didOverflow;
}

const ScheduledBeat &ScheduledBeatBuffer::operator[](std::size_t index) const noexcept
{
    return beats[index];
}

bool ScheduledBeatBuffer::add(ScheduledBeat beat) noexcept
{
    if (beatCount >= beats.size())
    {
        return false;
    }

    beats[beatCount++] = beat;
    return true;
}

void ScheduledBeatBuffer::markOverflow() noexcept
{
    didOverflow = true;
}

BeatSchedule BeatScheduler::schedule(const HostTiming &timing) noexcept
{
    BeatSchedule result;

    if (!isValid(timing))
    {
        reset();
        return result;
    }

    if (timing.samplePosition > std::numeric_limits<std::int64_t>::max() - timing.blockSize)
    {
        reset();
        return result;
    }

    const auto ppqPerSample = timing.bpm / (60.0 * timing.sampleRate);
    const auto nextPpqPosition = timing.ppqPosition + (static_cast<double>(timing.blockSize) * ppqPerSample);
    if (!std::isfinite(ppqPerSample) || !std::isfinite(nextPpqPosition))
    {
        reset();
        return result;
    }

    const auto ppqContinuityTolerance = std::max(positionTolerance, ppqPerSample * 0.5);
    result.transportDiscontinuity = !wasPlaying || timing.samplePosition != expectedNextSamplePosition ||
                                    std::abs(timing.ppqPosition - expectedNextPpqPosition) > ppqContinuityTolerance;

    wasPlaying = true;
    expectedNextSamplePosition = timing.samplePosition + timing.blockSize;
    expectedNextPpqPosition = nextPpqPosition;
    result.hostTimingValid = true;

    const auto beatLengthPpq = 4.0 / static_cast<double>(timing.timeSignatureDenominator);
    const auto relativePpq = timing.ppqPosition - timing.ppqPositionOfLastBarStart;

    std::int64_t beatOrdinal = 0;
    if (!tryCeilToInt64(relativePpq / beatLengthPpq, beatOrdinal))
    {
        reset();
        return {};
    }

    const auto samplesPerPpq = 60.0 * timing.sampleRate / timing.bpm;

    while (true)
    {
        const auto beatPpq = timing.ppqPositionOfLastBarStart + (static_cast<double>(beatOrdinal) * beatLengthPpq);
        const auto samplePositionInBlock = (beatPpq - timing.ppqPosition) * samplesPerPpq;

        int sampleOffset = 0;
        if (!trySampleOffset(samplePositionInBlock, sampleOffset))
        {
            result.beats.markOverflow();
            break;
        }

        if (sampleOffset >= timing.blockSize)
        {
            break;
        }

        if (sampleOffset >= 0)
        {
            const auto beatIndex = positiveModulo(beatOrdinal, timing.timeSignatureNumerator);
            if (!result.beats.add({sampleOffset, beatIndex + 1, beatIndex == 0}))
            {
                result.beats.markOverflow();
                break;
            }
        }

        if (beatOrdinal == std::numeric_limits<std::int64_t>::max())
        {
            result.beats.markOverflow();
            break;
        }

        ++beatOrdinal;
    }

    return result;
}

void BeatScheduler::reset() noexcept
{
    wasPlaying = false;
    expectedNextSamplePosition = 0;
    expectedNextPpqPosition = 0.0;
}
} // namespace tempoflow::timing
