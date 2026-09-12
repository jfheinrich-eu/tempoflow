#include "preset/PresetValidator.h"

#include <iostream>
#include <string_view>

namespace
{
const juce::String validPreset = R"json(
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": { "name": "Test" },
  "tempo": { "bpm": 120 },
  "meter": { "numerator": 4, "denominator": 4, "grouping": [1, 1, 1, 1] },
  "subdivision": { "mode": "none", "partsPerBeat": 1 },
  "pattern": {
    "beats": [
      { "beat": 1, "click": "accent" },
      { "beat": 2, "click": "normal" },
      { "beat": 3, "click": "normal" },
      { "beat": 4, "click": "normal" }
    ]
  },
  "sound": { "soundSet": "default", "volume": 0.8 },
  "playback": { "mode": "host" }
}
)json";

bool expectValid(const juce::String &json, std::string_view name)
{
    const auto result = tempoflow::preset::validatePresetJson(json);
    if (result.isValid())
        return true;

    std::cerr << name << " should be valid\n";
    for (const auto &error : result.errors)
        std::cerr << "  - " << error << '\n';
    return false;
}

bool expectInvalid(const juce::String &json, std::string_view name)
{
    if (!tempoflow::preset::validatePresetJson(json).isValid())
        return true;

    std::cerr << name << " should be invalid\n";
    return false;
}
} // namespace

int main()
{
    bool passed = expectValid(validPreset, "baseline preset");

    passed &= expectInvalid(validPreset.replace("[1, 1, 1, 1]", "[2, 1]"), "invalid grouping sum");
    passed &= expectInvalid(validPreset.replace("\"beat\": 4", "\"beat\": 3"), "duplicate beat position");
    passed &= expectInvalid(validPreset.replace("\"numerator\": 4, \"denominator\": 4, \"grouping\": [1, 1, 1, 1]",
                                                "\"numerator\": 5, \"denominator\": 4, \"grouping\": [1, 1, 1, 1, 1]"),
                            "missing beat");
    passed &= expectInvalid(validPreset.replaceFirstOccurrenceOf("\"normal\"", "\"laser\""), "unsupported click");
    passed &= expectInvalid(validPreset.replace("\"bpm\": 120", "\"bpm\": 301"), "out-of-range tempo");
    passed &= expectInvalid(validPreset.replace("\"volume\": 0.8", "\"volume\": 1.1"), "out-of-range volume");
    passed &= expectInvalid(validPreset.replace("\"schemaVersion\": \"1.0.0\"", "\"schemaVersion\": \"2.0.0\""),
                            "unsupported schema major version");
    passed &=
        expectInvalid(validPreset.replace("\"partsPerBeat\": 1", "\"partsPerBeat\": 3"), "inconsistent subdivision");
    passed &= expectInvalid("{ invalid JSON", "malformed JSON");
    passed &= expectInvalid(juce::String::repeatedString("x", 1024 * 1024 + 1), "oversized preset");

    if (passed)
        std::cout << "All preset validator tests passed.\n";

    return passed ? 0 : 1;
}
