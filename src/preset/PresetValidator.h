#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <vector>

namespace tempoflow::preset
{
inline constexpr std::int64_t maximumPresetFileSizeBytes = 1024 * 1024;
inline constexpr std::size_t maximumJsonNestingDepth = 32;
inline constexpr std::size_t maximumJsonContainerCount = 2'048;
inline constexpr std::size_t maximumJsonPropertiesPerObject = 256;
inline constexpr std::size_t maximumJsonArrayElements = 4'096;
inline constexpr std::size_t maximumJsonTotalProperties = 8'192;
inline constexpr std::size_t maximumJsonTotalArrayElements = 16'384;

struct ValidationResult
{
    [[nodiscard]] bool isValid() const noexcept
    {
        return errors.empty();
    }

    std::vector<juce::String> errors;
};

[[nodiscard]] ValidationResult validatePresetJson(const juce::String &jsonText);
} // namespace tempoflow::preset
