#pragma once

#include "PresetFileIO.h"

#include <juce_core/juce_core.h>

#include <cstdint>
#include <vector>

namespace tempoflow::preset
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

enum class SubdivisionMode
{
    none,
    triplet
};

enum class PlaybackMode
{
    internal,
    host
};

struct RuntimeMeter final
{
    std::int64_t numerator = 0;
    std::int64_t denominator = 0;
    std::vector<std::int64_t> grouping;
};

struct RuntimeSubdivision final
{
    SubdivisionMode mode = SubdivisionMode::none;
    std::int64_t partsPerBeat = 1;
};

struct RuntimeBeat final
{
    std::int64_t beat = 0;
    ClickType click = ClickType::mute;
};

struct RuntimePattern final
{
    std::vector<RuntimeBeat> beats;
};

struct RuntimeSound final
{
    juce::String soundSet;
    double volume = 0.0;
};

struct RuntimePreset final
{
    juce::String schemaVersion;
    juce::String name;
    double bpm = 0.0;
    RuntimeMeter meter;
    RuntimeSubdivision subdivision;
    RuntimePattern pattern;
    RuntimeSound sound;
    PlaybackMode playbackMode = PlaybackMode::host;
};

struct RuntimePresetResult final
{
    [[nodiscard]] bool isValid() const noexcept
    {
        return errors.empty();
    }

    RuntimePreset preset;
    std::vector<juce::String> errors;
};

[[nodiscard]] RuntimePresetResult parsePresetJson(const juce::String &jsonText);
[[nodiscard]] RuntimePresetResult loadPresetFile(const juce::File &file);
} // namespace tempoflow::preset
