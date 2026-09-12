#pragma once

#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tempoflow::preset
{
inline constexpr std::size_t maximumPresetDirectoryDepth = 16;
inline constexpr std::size_t maximumScannedDirectoryEntries = 10'000;
inline constexpr std::size_t maximumPresetFileCount = 1'024;
inline constexpr std::int64_t maximumTotalPresetBytes = 16 * 1024 * 1024;

struct PresetFileCollection
{
    [[nodiscard]] bool isValid() const noexcept
    {
        return errors.empty();
    }

    std::vector<juce::File> files;
    std::vector<juce::String> errors;
};

struct PresetFileReadResult
{
    [[nodiscard]] bool isValid() const noexcept
    {
        return error.isEmpty();
    }

    juce::String content;
    juce::String error;
    std::int64_t bytesRead = 0;
};

[[nodiscard]] PresetFileCollection collectPresetFiles(const std::vector<juce::File> &inputs);
[[nodiscard]] PresetFileReadResult readPresetFileBounded(const juce::File &file);
} // namespace tempoflow::preset
