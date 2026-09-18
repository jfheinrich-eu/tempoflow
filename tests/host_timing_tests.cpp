#include "timing/BeatScheduler.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
using tempoflow::timing::BeatScheduler;
using tempoflow::timing::HostTiming;

bool expect(bool condition, std::string_view message)
{
    if (condition)
        return true;

    std::cerr << message << '\n';
    return false;
}

HostTiming makeTiming(double ppqPosition, double lastBarStartPpq, std::int64_t samplePosition, int blockSize,
                      double sampleRate = 48'000.0, double bpm = 120.0, int numerator = 4, int denominator = 4)
{
    return {true, bpm, numerator, denominator, ppqPosition, lastBarStartPpq, samplePosition, sampleRate, blockSize};
}

bool testStoppedAndInvalidTimingProduceNoEvents()
{
    BeatScheduler scheduler;
    auto stopped = makeTiming(0.0, 0.0, 0, 512);
    stopped.isPlaying = false;

    auto invalid = makeTiming(0.0, 0.0, 0, 512);
    invalid.bpm = std::numeric_limits<double>::quiet_NaN();

    const auto stoppedResult = scheduler.schedule(stopped);
    const auto invalidResult = scheduler.schedule(invalid);

    return expect(stoppedResult.beats.empty(), "Stopped transport must produce no beats") &&
           expect(!stoppedResult.hostTimingValid, "Stopped transport must not be accepted as valid timing") &&
           expect(invalidResult.beats.empty(), "Invalid host timing must produce no beats") &&
           expect(!invalidResult.hostTimingValid, "Invalid host timing must not be accepted");
}

bool testBeatAtBlockStart()
{
    BeatScheduler scheduler;
    const auto result = scheduler.schedule(makeTiming(0.0, 0.0, 0, 512));

    return expect(result.transportDiscontinuity, "The first playing block must be a transport discontinuity") &&
           expect(result.hostTimingValid, "Valid playing timing must be accepted") &&
           expect(result.beats.size() == 1, "A block starting on a beat must contain one beat") &&
           expect(result.beats[0].sampleOffset == 0, "The first beat must be at sample zero") &&
           expect(result.beats[0].beatNumber == 1, "The first beat must be beat one") &&
           expect(result.beats[0].isBarStart, "The first beat must start the bar");
}

bool testHalfOpenBlockBoundary()
{
    BeatScheduler scheduler;
    constexpr auto sampleRate = 48'000.0;
    constexpr auto bpm = 120.0;
    constexpr auto blockSize = 64;
    constexpr auto ppqPerSample = bpm / (60.0 * sampleRate);
    const auto firstBlockStart = 1.0 - (blockSize * ppqPerSample);

    const auto first = scheduler.schedule(makeTiming(firstBlockStart, 0.0, 10'000, blockSize, sampleRate, bpm));
    const auto second = scheduler.schedule(makeTiming(1.0, 0.0, 10'064, blockSize, sampleRate, bpm));

    return expect(first.beats.empty(), "A beat at the exclusive block end must not be scheduled") &&
           expect(!second.transportDiscontinuity, "Adjacent blocks must be continuous") &&
           expect(second.beats.size() == 1, "The next block must contain the boundary beat") &&
           expect(second.beats[0].sampleOffset == 0, "The boundary beat must occur at sample zero") &&
           expect(second.beats[0].beatNumber == 2, "The boundary beat must be beat two");
}

bool testMultipleBeatsAndBarWrap()
{
    BeatScheduler scheduler;
    const auto result = scheduler.schedule(makeTiming(0.0, 0.0, 0, 260, 100.0, 120.0));

    bool passed = expect(result.beats.size() == 6, "The block must contain six beats");
    constexpr std::array expectedOffsets{0, 50, 100, 150, 200, 250};
    constexpr std::array expectedBeatNumbers{1, 2, 3, 4, 1, 2};

    for (std::size_t index = 0; index < result.beats.size(); ++index)
    {
        passed &= expect(result.beats[index].sampleOffset == expectedOffsets[index], "Unexpected beat sample offset");
        passed &= expect(result.beats[index].beatNumber == expectedBeatNumbers[index], "Unexpected beat number");
    }

    passed &= expect(result.beats[0].isBarStart, "Beat one must start a bar");
    passed &= expect(result.beats[4].isBarStart, "Wrapped beat one must start a bar");
    return passed;
}

bool testCompoundMeterUsesDenominatorBeatLength()
{
    BeatScheduler scheduler;
    const auto result = scheduler.schedule(makeTiming(0.0, 0.0, 0, 25'000, 48'000.0, 120.0, 6, 8));

    return expect(result.beats.size() == 3, "A 6/8 block must schedule eighth-note beats") &&
           expect(result.beats[0].sampleOffset == 0 && result.beats[0].beatNumber == 1,
                  "6/8 beat one must start at sample zero") &&
           expect(result.beats[1].sampleOffset == 12'000 && result.beats[1].beatNumber == 2,
                  "6/8 beat two must use an eighth-note interval") &&
           expect(result.beats[2].sampleOffset == 24'000 && result.beats[2].beatNumber == 3,
                  "6/8 beat three must use an eighth-note interval");
}

bool testMidBarStartAndSeek()
{
    BeatScheduler scheduler;
    const auto midBar = scheduler.schedule(makeTiming(1.25, 0.0, 20'000, 20'000));
    const auto seek = scheduler.schedule(makeTiming(0.0, 0.0, 500, 512));

    return expect(midBar.beats.size() == 1, "A mid-bar block must schedule its next beat") &&
           expect(midBar.beats[0].sampleOffset == 18'000, "The next mid-bar beat has the wrong sample offset") &&
           expect(midBar.beats[0].beatNumber == 3, "The next mid-bar beat must be beat three") &&
           expect(seek.transportDiscontinuity, "A non-contiguous sample position must be a seek") &&
           expect(seek.beats.size() == 1 && seek.beats[0].sampleOffset == 0,
                  "A seek to a beat boundary must schedule that beat");
}

bool testPpqJumpIsADiscontinuity()
{
    BeatScheduler scheduler;
    constexpr auto blockSize = 512;
    constexpr auto ppqPerSample = 120.0 / (60.0 * 48'000.0);

    static_cast<void>(scheduler.schedule(makeTiming(0.0, 0.0, 1'000, blockSize)));
    const auto loopJump = scheduler.schedule(makeTiming(2.0, 0.0, 1'512, blockSize));
    const auto expectedPpq = blockSize * ppqPerSample;

    return expect(std::abs(expectedPpq - 2.0) > 0.1, "The test must contain a material PPQ jump") &&
           expect(loopJump.transportDiscontinuity, "A PPQ jump must be a transport discontinuity");
}

bool testSampleRatesAndBlockSizes()
{
    constexpr std::array sampleRates{44'100.0, 48'000.0, 96'000.0};
    constexpr std::array blockSizes{32, 64, 128, 256, 512, 1'024};
    constexpr auto bpm = 120.0;
    bool passed = true;

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            BeatScheduler scheduler;
            const auto samplesPerPpq = 60.0 * sampleRate / bpm;
            const auto expectedOffset = blockSize / 2;
            const auto blockStartPpq = 1.0 - (static_cast<double>(expectedOffset) / samplesPerPpq);
            const auto result = scheduler.schedule(makeTiming(blockStartPpq, 0.0, 0, blockSize, sampleRate, bpm));

            passed &= expect(result.beats.size() == 1, "The matrix block must contain one beat");
            passed &=
                expect(result.beats[0].sampleOffset == expectedOffset, "The matrix beat has the wrong sample offset");
            passed &= expect(result.beats[0].beatNumber == 2, "The matrix beat must be beat two");
        }
    }

    return passed;
}

bool testTempoAndMeterChangesApplyImmediately()
{
    BeatScheduler scheduler;
    const auto first = scheduler.schedule(makeTiming(0.0, 0.0, 0, 12'000, 48'000.0, 120.0, 4, 4));
    const auto meterChange = scheduler.schedule(makeTiming(0.5, 0.0, 12'000, 48'000, 48'000.0, 60.0, 6, 8));

    return expect(first.beats.size() == 1 && first.beats[0].beatNumber == 1,
                  "The initial block must start with beat one") &&
           expect(!meterChange.transportDiscontinuity, "A boundary tempo or meter change must remain continuous") &&
           expect(meterChange.beats.size() == 2, "The changed block must use its new tempo") &&
           expect(meterChange.beats[0].sampleOffset == 0 && meterChange.beats[0].beatNumber == 2,
                  "The changed block must use the new 6/8 beat grid immediately") &&
           expect(meterChange.beats[1].sampleOffset == 24'000 && meterChange.beats[1].beatNumber == 3,
                  "The changed block must use the new tempo immediately");
}

bool testStopResetsContinuity()
{
    BeatScheduler scheduler;
    constexpr auto blockSize = 512;
    constexpr auto ppqPerSample = 120.0 / (60.0 * 48'000.0);

    static_cast<void>(scheduler.schedule(makeTiming(0.0, 0.0, 0, blockSize)));

    auto stopped = makeTiming(blockSize * ppqPerSample, 0.0, blockSize, blockSize);
    stopped.isPlaying = false;
    static_cast<void>(scheduler.schedule(stopped));

    const auto restartSamplePosition = 2 * static_cast<std::int64_t>(blockSize);
    const auto restarted =
        scheduler.schedule(makeTiming(2.0 * blockSize * ppqPerSample, 0.0, restartSamplePosition, blockSize));
    return expect(restarted.transportDiscontinuity, "Restarting after stop must reset scheduler continuity");
}

bool testFixedCapacityReportsOverflow()
{
    BeatScheduler scheduler;
    const auto result = scheduler.schedule(makeTiming(0.0, 0.0, 0, 1'000, 1.0, 300.0, 4, 16));

    return expect(result.beats.size() == tempoflow::timing::ScheduledBeatBuffer::capacity,
                  "The fixed beat buffer must stop at its capacity") &&
           expect(result.beats.overflowed(), "Capacity exhaustion must be reported");
}
} // namespace

int main()
{
    const bool passed =
        testStoppedAndInvalidTimingProduceNoEvents() && testBeatAtBlockStart() && testHalfOpenBlockBoundary() &&
        testMultipleBeatsAndBarWrap() && testCompoundMeterUsesDenominatorBeatLength() && testMidBarStartAndSeek() &&
        testPpqJumpIsADiscontinuity() && testSampleRatesAndBlockSizes() && testTempoAndMeterChangesApplyImmediately() &&
        testStopResetsContinuity() && testFixedCapacityReportsOverflow();

    if (!passed)
        return 1;

    std::cout << "TempoFlow host timing tests passed\n";
    return 0;
}
