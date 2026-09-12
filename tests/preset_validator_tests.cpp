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

const auto validCompleteMetadataPreset = validPreset.replace(
    "\"metadata\": { \"name\": \"Test\" }",
    "\"metadata\": { \"name\": \"Test\", \"description\": \"Practice preset\", \"category\": \"practice\", "
    "\"tags\": [\"test\", \"practice\"], \"author\": \"TempoFlow\", "
    "\"createdAt\": \"2024-02-29T12:34:56Z\", \"updatedAt\": \"2024-02-29T13:34:56.123+01:00\" }");

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
    passed &= expectValid(validCompleteMetadataPreset, "complete metadata");
    passed &= expectValid(
        validCompleteMetadataPreset.replace("\"createdAt\": \"2024-02-29T12:34:56Z\"", "\"createdAt\": \"\""),
        "empty compatible metadata timestamp");
    passed &= expectValid(validPreset.replace("\"partsPerBeat\": 1", "\"partsPerBeat\": 1.0"),
                          "integer-valued subdivision number");

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
    passed &= expectInvalid(validPreset.replace("\"partsPerBeat\": 1", "\"partsPerBeat\": 2147483649"),
                            "32-bit subdivision overflow");
    passed &= expectInvalid(validPreset.replace("\"partsPerBeat\": 1", "\"partsPerBeat\": 9223372036854775807"),
                            "64-bit subdivision boundary");

    passed &= expectInvalid(
        validCompleteMetadataPreset.replace("\"description\": \"Practice preset\"", "\"description\": 42"),
        "non-string metadata description");
    passed &= expectInvalid(validCompleteMetadataPreset.replace("\"category\": \"practice\"", "\"category\": 42"),
                            "non-string metadata category");
    passed &= expectInvalid(validCompleteMetadataPreset.replace("\"author\": \"TempoFlow\"", "\"author\": 42"),
                            "non-string metadata author");
    passed &= expectInvalid(validCompleteMetadataPreset.replace("\"tags\": [\"test\", \"practice\"]", "\"tags\": 42"),
                            "non-array metadata tags");
    passed &= expectInvalid(
        validCompleteMetadataPreset.replace("\"tags\": [\"test\", \"practice\"]", "\"tags\": [\"test\", 42]"),
        "non-string metadata tag");
    passed &= expectInvalid(validCompleteMetadataPreset.replace("2024-02-29T12:34:56Z", "2023-02-29T12:34:56Z"),
                            "invalid metadata calendar date");
    passed &= expectInvalid(validCompleteMetadataPreset.replace("2024-02-29T12:34:56Z", "2024-02-29T12:34:56"),
                            "metadata timestamp without offset");
    passed &= expectInvalid(
        validCompleteMetadataPreset.replace("\"createdAt\": \"2024-02-29T12:34:56Z\"", "\"createdAt\": 42"),
        "non-string metadata timestamp");

    passed &= expectInvalid(validPreset.replace("\"schema\":", "\"futureSchema\":"), "missing schema");
    passed &=
        expectInvalid(validPreset.replace("\"schemaVersion\":", "\"futureSchemaVersion\":"), "missing schema version");
    passed &= expectInvalid(validPreset.replace("\"metadata\":", "\"futureMetadata\":"), "missing metadata");
    passed &= expectInvalid(validPreset.replace("\"name\":", "\"futureName\":"), "missing metadata name");
    passed &= expectInvalid(validPreset.replace("\"bpm\":", "\"futureBpm\":"), "missing tempo bpm");
    passed &= expectInvalid(validPreset.replace("\"numerator\":", "\"futureNumerator\":"), "missing meter numerator");
    passed &=
        expectInvalid(validPreset.replace("\"denominator\":", "\"futureDenominator\":"), "missing meter denominator");
    passed &= expectInvalid(validPreset.replace("\"grouping\":", "\"futureGrouping\":"), "missing meter grouping");
    passed &= expectInvalid(validPreset.replace("\"mode\": \"none\"", "\"futureMode\": \"none\""),
                            "missing subdivision mode");
    passed &=
        expectInvalid(validPreset.replace("\"partsPerBeat\":", "\"futurePartsPerBeat\":"), "missing subdivision parts");
    passed &= expectInvalid(validPreset.replace("\"beats\":", "\"futureBeats\":"), "missing pattern beats");
    passed &=
        expectInvalid(validPreset.replaceFirstOccurrenceOf("\"beat\":", "\"futureBeat\":"), "missing beat position");
    passed &=
        expectInvalid(validPreset.replaceFirstOccurrenceOf("\"click\":", "\"futureClick\":"), "missing beat click");
    passed &= expectInvalid(validPreset.replace("\"soundSet\":", "\"futureSoundSet\":"), "missing sound set");
    passed &= expectInvalid(validPreset.replace("\"volume\":", "\"futureVolume\":"), "missing sound volume");
    passed &= expectInvalid(validPreset.replace("\"playback\":", "\"futurePlayback\":"), "missing playback");
    passed &=
        expectInvalid(validPreset.replace("\"mode\": \"host\"", "\"futureMode\": \"host\""), "missing playback mode");
    passed &= expectInvalid("{ invalid JSON", "malformed JSON");
    passed &= expectInvalid(juce::String::repeatedString("x", 1024 * 1024 + 1), "oversized preset");

    if (passed)
        std::cout << "All preset validator tests passed.\n";

    return passed ? 0 : 1;
}
