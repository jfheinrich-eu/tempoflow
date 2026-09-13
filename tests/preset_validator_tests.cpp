#include "preset/PresetFileIO.h"
#include "preset/PresetValidator.h"

#include <iostream>
#include <string_view>
#include <vector>

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

juce::String addRootProperty(const juce::String &json, const juce::String &name, const juce::String &value)
{
    return json.replace("\n}\n", ",\n  \"" + name + "\": " + value + "\n}\n");
}

juce::String createNestedArray(std::size_t depth)
{
    juce::String value = "0";
    for (std::size_t index = 0; index < depth; ++index)
        value = "[" + value + "]";
    return value;
}

juce::String createArray(std::size_t elementCount)
{
    juce::StringArray values;
    for (std::size_t index = 0; index < elementCount; ++index)
        values.add("0");
    return "[" + values.joinIntoString(",") + "]";
}

juce::String createObject(std::size_t propertyCount)
{
    juce::StringArray properties;
    for (std::size_t index = 0; index < propertyCount; ++index)
        properties.add("\"p" + juce::String(index) + "\":0");
    return "{" + properties.joinIntoString(",") + "}";
}

class TemporaryDirectory
{
  public:
    TemporaryDirectory()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("tempoflow-preset-tests", {}, true))
    {
        directory.createDirectory();
    }

    ~TemporaryDirectory()
    {
        directory.deleteRecursively(false);
    }

    [[nodiscard]] const juce::File &get() const noexcept
    {
        return directory;
    }

  private:
    juce::File directory;
};

bool expectBoundedFileRead()
{
    TemporaryDirectory temporaryDirectory;
    const auto preset = temporaryDirectory.get().getChildFile("valid.tempoflow");
    if (!preset.replaceWithData(validPreset.toRawUTF8(), static_cast<std::size_t>(validPreset.getNumBytesAsUTF8())))
    {
        std::cerr << "could not create bounded file-read fixture\n";
        return false;
    }

    const auto readResult = tempoflow::preset::readPresetFileBounded(preset);
    if (readResult.isValid() && readResult.content == validPreset &&
        readResult.bytesRead == validPreset.getNumBytesAsUTF8())
        return true;

    std::cerr << "bounded file read should return the complete preset\n";
    return false;
}

bool expectOversizedFileRejected()
{
    TemporaryDirectory temporaryDirectory;
    const auto preset = temporaryDirectory.get().getChildFile("oversized.tempoflow");
    if (!preset.replaceWithText(
            juce::String::repeatedString("x", static_cast<int>(tempoflow::preset::maximumPresetFileSizeBytes + 1))))
    {
        std::cerr << "could not create oversized file fixture\n";
        return false;
    }

    if (!tempoflow::preset::readPresetFileBounded(preset).isValid())
        return true;

    std::cerr << "oversized file should be rejected during bounded read\n";
    return false;
}

bool expectDirectoryDepthLimit()
{
    TemporaryDirectory temporaryDirectory;
    auto directory = temporaryDirectory.get();
    juce::File boundaryPreset;
    for (std::size_t depth = 0; depth <= tempoflow::preset::maximumPresetDirectoryDepth; ++depth)
    {
        directory = directory.getChildFile("level-" + juce::String(depth));
        if (directory.createDirectory().failed())
        {
            std::cerr << "could not create directory-depth fixture\n";
            return false;
        }

        if (depth + 1 == tempoflow::preset::maximumPresetDirectoryDepth)
        {
            boundaryPreset = directory.getChildFile("boundary.tempoflow");
            if (!boundaryPreset.replaceWithText("{}"))
            {
                std::cerr << "could not create directory-depth boundary fixture\n";
                return false;
            }
        }
    }

    const auto collection = tempoflow::preset::collectPresetFiles({temporaryDirectory.get()});
    if (!collection.isValid() && collection.files.size() == 1 && collection.files.front() == boundaryPreset)
        return true;

    std::cerr << "directory traversal beyond the depth limit should be rejected\n";
    return false;
}

bool expectPresetFileCountLimit()
{
    TemporaryDirectory temporaryDirectory;
    for (std::size_t index = 0; index <= tempoflow::preset::maximumPresetFileCount; ++index)
    {
        const auto file = temporaryDirectory.get().getChildFile("preset-" + juce::String(index) + ".tempoflow");
        if (!file.create())
        {
            std::cerr << "could not create preset-count fixture\n";
            return false;
        }
    }

    const auto collection = tempoflow::preset::collectPresetFiles({temporaryDirectory.get()});
    if (!collection.isValid() && collection.files.size() == tempoflow::preset::maximumPresetFileCount)
        return true;

    std::cerr << "preset discovery beyond the file-count limit should be rejected\n";
    return false;
}

bool expectCumulativePresetSizeLimit()
{
    TemporaryDirectory temporaryDirectory;
    constexpr auto fileCount =
        tempoflow::preset::maximumTotalPresetBytes / tempoflow::preset::maximumPresetFileSizeBytes + 1;

    for (std::int64_t index = 0; index < fileCount; ++index)
    {
        const auto file = temporaryDirectory.get().getChildFile("large-" + juce::String(index) + ".tempoflow");
        juce::FileOutputStream output(file);
        if (!output.openedOk() || !output.setPosition(tempoflow::preset::maximumPresetFileSizeBytes - 1) ||
            !output.writeByte(0))
        {
            std::cerr << "could not create cumulative-size fixture\n";
            return false;
        }
        output.flush();
        if (output.getStatus().failed())
        {
            std::cerr << "could not flush cumulative-size fixture\n";
            return false;
        }
    }

    const auto collection = tempoflow::preset::collectPresetFiles({temporaryDirectory.get()});
    if (!collection.isValid() && collection.files.size() == static_cast<std::size_t>(fileCount - 1))
        return true;

    std::cerr << "preset discovery beyond the cumulative-size limit should be rejected\n";
    return false;
}

bool expectDuplicatePresetPathsDeduplicated()
{
    TemporaryDirectory temporaryDirectory;
    const auto preset = temporaryDirectory.get().getChildFile("duplicate.tempoflow");
    if (!preset.replaceWithText("{}"))
    {
        std::cerr << "could not create duplicate-path fixture\n";
        return false;
    }

    const auto collection = tempoflow::preset::collectPresetFiles({preset, preset, temporaryDirectory.get()});
    if (collection.isValid() && collection.files.size() == 1 && collection.files.front() == preset)
        return true;

    std::cerr << "duplicate preset paths should be collected once\n";
    return false;
}

bool expectFailedReadRejected()
{
    TemporaryDirectory temporaryDirectory;
    const auto missingPreset = temporaryDirectory.get().getChildFile("missing.tempoflow");
    if (!tempoflow::preset::readPresetFileBounded(missingPreset).isValid())
        return true;

    std::cerr << "a missing preset should fail bounded reading\n";
    return false;
}

bool expectSymbolicLinkRejected()
{
    TemporaryDirectory temporaryDirectory;
    const auto target = temporaryDirectory.get().getChildFile("target.tempoflow");
    const auto link = temporaryDirectory.get().getChildFile("link.tempoflow");
    if (!target.replaceWithText("{}"))
    {
        std::cerr << "could not create symbolic-link target fixture\n";
        return false;
    }

    if (!target.createSymbolicLink(link, false))
    {
#if JUCE_WINDOWS
        std::cout << "Symbolic-link test skipped because Windows did not grant link creation.\n";
        return true;
#else
        std::cerr << "could not create symbolic-link fixture\n";
        return false;
#endif
    }

    const auto collection = tempoflow::preset::collectPresetFiles({link});
    const auto readResult = tempoflow::preset::readPresetFileBounded(link);
    if (!collection.isValid() && !readResult.isValid())
        return true;

    std::cerr << "symbolic links should be rejected during discovery and reading\n";
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
    passed &= expectValid(
        addRootProperty(validPreset, "nested", createNestedArray(tempoflow::preset::maximumJsonNestingDepth - 1)),
        "maximum JSON nesting depth");
    passed &= expectInvalid(
        addRootProperty(validPreset, "nested", createNestedArray(tempoflow::preset::maximumJsonNestingDepth)),
        "excessive JSON nesting depth");
    passed &= expectValid(
        addRootProperty(validPreset, "extensionArray", createArray(tempoflow::preset::maximumJsonArrayElements)),
        "maximum JSON array length");
    passed &= expectInvalid(
        addRootProperty(validPreset, "extensionArray", createArray(tempoflow::preset::maximumJsonArrayElements + 1)),
        "excessive JSON array length");
    passed &= expectValid(addRootProperty(validPreset, "extensionObject",
                                          createObject(tempoflow::preset::maximumJsonPropertiesPerObject)),
                          "maximum JSON object property count");
    passed &= expectInvalid(addRootProperty(validPreset, "extensionObject",
                                            createObject(tempoflow::preset::maximumJsonPropertiesPerObject + 1)),
                            "excessive JSON object property count");

    passed &= expectBoundedFileRead();
    passed &= expectOversizedFileRejected();
    passed &= expectDirectoryDepthLimit();
    passed &= expectPresetFileCountLimit();
    passed &= expectCumulativePresetSizeLimit();
    passed &= expectDuplicatePresetPathsDeduplicated();
    passed &= expectFailedReadRejected();
    passed &= expectSymbolicLinkRejected();

    if (passed)
        std::cout << "All preset validator tests passed.\n";

    return passed ? 0 : 1;
}
