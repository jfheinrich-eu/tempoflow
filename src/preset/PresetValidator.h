#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <vector>

namespace tempoflow::preset
{
inline constexpr std::int64_t maximumPresetFileSizeBytes = 1024 * 1024;

struct ValidationResult
{
    [[nodiscard]] bool isValid() const noexcept
    {
        return errors.empty();
    }

    std::vector<juce::String> errors;
};

[[nodiscard]] ValidationResult validatePresetJson(const juce::String& jsonText);
} // namespace tempoflow::preset
