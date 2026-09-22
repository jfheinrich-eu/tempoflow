#include "preset/PresetRuntimeModel.h"

#include <iostream>
#include <string_view>
#include <vector>

namespace
{
const juce::String validPreset = R"json(
{
  "schema": "tempoflow-preset",
  "schemaVersion": "1.0.0",
  "metadata": { "name": "Runtime Test" },
  "tempo": { "bpm": 120.5 },
  "meter": { "numerator": 4, "denominator": 4, "grouping": [1, 1, 1, 1] },
  "subdivision": { "mode": "triplet", "partsPerBeat": 3 },
  "pattern": {
    "beats": [
      { "beat": 3, "click": "high" },
      { "beat": 1, "click": "accent" },
      { "beat": 4, "click": "mute" },
      { "beat": 2, "click": "wood" }
    ]
  },
  "sound": { "soundSet": "default", "volume": 0.75 },
  "playback": { "mode": "internal" }
}
)json";

bool expect(bool condition, std::string_view message)
{
    if (condition)
        return true;

    std::cerr << message << '\n';
    return false;
}

bool testParsesTypedRuntimeModel()
{
    const auto result = tempoflow::preset::parsePresetJson(validPreset);
    if (!expect(result.isValid(), "A valid preset must produce a runtime model"))
        return false;

    const auto &preset = result.preset;
    return expect(preset.schemaVersion == "1.0.0", "The schema version must be preserved") &&
           expect(preset.name == "Runtime Test", "The metadata name must be preserved") &&
           expect(preset.bpm == 120.5, "The BPM must be parsed as a double") &&
           expect(preset.meter.numerator == 4 && preset.meter.denominator == 4, "The meter values must be parsed") &&
           expect(preset.meter.grouping == std::vector<std::int64_t>{1, 1, 1, 1}, "The grouping must be parsed") &&
           expect(preset.subdivision.mode == tempoflow::preset::SubdivisionMode::triplet &&
                      preset.subdivision.partsPerBeat == 3,
                  "The subdivision must be parsed") &&
           expect(preset.pattern.beats.size() == 4 && preset.pattern.beats.front().beat == 1 &&
                      preset.pattern.beats.front().click == tempoflow::preset::ClickType::accent,
                  "Pattern beats must be sorted by beat position") &&
           expect(preset.sound.soundSet == "default" && preset.sound.volume == 0.75,
                  "The sound settings must be parsed") &&
           expect(preset.playbackMode == tempoflow::preset::PlaybackMode::internal, "The playback mode must be parsed");
}

bool testRejectsInvalidPresetBeforeModelConstruction()
{
    const auto result = tempoflow::preset::parsePresetJson(validPreset.replace("\"bpm\": 120.5", "\"bpm\": 301"));
    return expect(!result.isValid(), "An invalid preset must not produce a valid runtime model") &&
           expect(result.preset.pattern.beats.empty(), "Invalid input must leave the runtime model empty");
}

bool testLoadsPresetThroughBoundedFileReader()
{
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getNonexistentChildFile("tempoflow-runtime-model", {}, true);
    if (directory.createDirectory().failed())
    {
        std::cerr << "could not create runtime model test directory\n";
        return false;
    }

    const auto file = directory.getChildFile("runtime-test.tempoflow");
    const auto created =
        file.replaceWithData(validPreset.toRawUTF8(), static_cast<std::size_t>(validPreset.getNumBytesAsUTF8()));
    const auto result = created ? tempoflow::preset::loadPresetFile(file) : tempoflow::preset::RuntimePresetResult{};
    const auto removed = directory.deleteRecursively(false);

    return expect(created && result.isValid(), "A valid preset file must load through bounded file I/O") &&
           expect(result.preset.name == "Runtime Test", "The bounded file loader must preserve the parsed model") &&
           expect(removed, "The temporary runtime model directory must be removable");
}

bool testFileValidationErrorsIdentifyTheirSource()
{
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getNonexistentChildFile("tempoflow-runtime-model-errors", {}, true);
    if (directory.createDirectory().failed())
        return expect(false, "The runtime model error test directory must be created");

    const auto file = directory.getChildFile("invalid.tempoflow");
    const auto invalidPreset = validPreset.replace("\"bpm\": 120.5", "\"bpm\": 301");
    const auto created = file.replaceWithData(invalidPreset.toRawUTF8(), invalidPreset.getNumBytesAsUTF8());
    const auto result = created ? tempoflow::preset::loadPresetFile(file) : tempoflow::preset::RuntimePresetResult{};
    const auto removed = directory.deleteRecursively(false);

    return expect(created, "The invalid runtime model fixture must be created") &&
           expect(!result.isValid() && !result.errors.empty(), "The invalid runtime model fixture must fail") &&
           expect(result.errors.front().startsWith(file.getFullPathName() + ": "),
                  "Runtime model file errors must identify the source file") &&
           expect(removed, "The runtime model error test directory must be removable");
}
} // namespace

int main()
{
    const auto passed = testParsesTypedRuntimeModel() && testRejectsInvalidPresetBeforeModelConstruction() &&
                        testLoadsPresetThroughBoundedFileReader() && testFileValidationErrorsIdentifyTheirSource();
    if (!passed)
        return 1;

    std::cout << "TempoFlow runtime model tests passed\n";
    return 0;
}
