#include "PresetValidator.h"

#include <cmath>
#include <limits>
#include <regex>
#include <set>

namespace tempoflow::preset
{
namespace
{
using Errors = std::vector<juce::String>;

void require(bool condition, juce::String message, Errors &errors)
{
    if (!condition)
        errors.push_back(std::move(message));
}

[[nodiscard]] bool isInteger(const juce::var &value) noexcept
{
    return value.isInt() || value.isInt64();
}

[[nodiscard]] bool isNumber(const juce::var &value) noexcept
{
    return isInteger(value) || value.isDouble();
}

[[nodiscard]] juce::DynamicObject *requireObjectProperty(juce::DynamicObject &parent, const juce::Identifier &name,
                                                         Errors &errors)
{
    if (!parent.hasProperty(name))
    {
        errors.push_back("Missing required object: " + name.toString());
        return nullptr;
    }

    auto &value = parent.getProperty(name);
    if (!value.isObject())
    {
        errors.push_back("Expected object: " + name.toString());
        return nullptr;
    }

    return value.getDynamicObject();
}

[[nodiscard]] bool isSupportedSchemaVersion(const juce::String &version)
{
    static const std::regex semanticVersion(R"(^1\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*))"
                                            R"((?:-(?:0|[1-9][0-9]*|[0-9A-Za-z-]*[A-Za-z-][0-9A-Za-z-]*))"
                                            R"((?:\.(?:0|[1-9][0-9]*|[0-9A-Za-z-]*[A-Za-z-][0-9A-Za-z-]*))*)?)"
                                            R"((?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?$)");

    return std::regex_match(version.toStdString(), semanticVersion);
}

void validateMetadata(juce::DynamicObject &metadata, Errors &errors)
{
    const auto name = metadata.getProperty("name");
    require(name.isString() && name.toString().trim().isNotEmpty(), "metadata.name must be a non-empty string", errors);
}

void validateTempo(juce::DynamicObject &tempo, Errors &errors)
{
    const auto bpm = tempo.getProperty("bpm");
    const auto numericBpm = isNumber(bpm) ? static_cast<double>(bpm) : 0.0;
    require(isNumber(bpm) && std::isfinite(numericBpm) && numericBpm >= 20.0 && numericBpm <= 300.0,
            "tempo.bpm must be a finite number from 20 through 300", errors);
}

juce::int64 validateMeter(juce::DynamicObject &meter, Errors &errors)
{
    const auto numeratorValue = meter.getProperty("numerator");
    const auto denominatorValue = meter.getProperty("denominator");
    const auto groupingValue = meter.getProperty("grouping");

    const auto numerator = isInteger(numeratorValue) ? static_cast<juce::int64>(numeratorValue) : 0;
    require(isInteger(numeratorValue) && numerator >= 1, "meter.numerator must be an integer of at least 1", errors);

    const auto denominator = isInteger(denominatorValue) ? static_cast<juce::int64>(denominatorValue) : 0;
    require(denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16,
            "meter.denominator must be 2, 4, 8, or 16", errors);

    if (!groupingValue.isArray() || groupingValue.getArray()->isEmpty())
    {
        errors.push_back("meter.grouping must be a non-empty array");
        return numerator;
    }

    juce::int64 groupingSum = 0;
    for (const auto &group : *groupingValue.getArray())
    {
        const auto groupSize = isInteger(group) ? static_cast<juce::int64>(group) : 0;
        if (!isInteger(group) || groupSize < 1)
        {
            errors.push_back("meter.grouping values must be positive integers");
            continue;
        }

        if (groupingSum > std::numeric_limits<juce::int64>::max() - groupSize)
        {
            errors.push_back("meter.grouping sum exceeds the supported integer range");
            continue;
        }

        groupingSum += groupSize;
    }

    require(groupingSum == numerator, "meter.grouping must sum to meter.numerator", errors);
    return numerator;
}

void validateSubdivision(juce::DynamicObject &subdivision, Errors &errors)
{
    const auto mode = subdivision.getProperty("mode");
    const auto partsPerBeat = subdivision.getProperty("partsPerBeat");

    if (!mode.isString() || !isInteger(partsPerBeat))
    {
        errors.push_back("subdivision.mode and subdivision.partsPerBeat are required");
        return;
    }

    const auto modeText = mode.toString();
    const auto parts = static_cast<int>(partsPerBeat);
    require((modeText == "none" && parts == 1) || (modeText == "triplet" && parts == 3),
            "subdivision must be none/1 or triplet/3", errors);
}

void validatePattern(juce::DynamicObject &pattern, juce::int64 numerator, Errors &errors)
{
    const auto beatsValue = pattern.getProperty("beats");
    if (!beatsValue.isArray())
    {
        errors.push_back("pattern.beats must be an array");
        return;
    }

    const auto &beats = *beatsValue.getArray();
    require(static_cast<juce::int64>(beats.size()) == numerator, "pattern.beats count must equal meter.numerator",
            errors);

    const std::set<juce::String> supportedClicks{"accent", "normal", "high", "low", "wood", "mute"};
    std::set<juce::int64> positions;

    for (const auto &beatValue : beats)
    {
        if (!beatValue.isObject())
        {
            errors.push_back("Every pattern.beats entry must be an object");
            continue;
        }

        auto *beat = beatValue.getDynamicObject();
        const auto positionValue = beat->getProperty("beat");
        const auto clickValue = beat->getProperty("click");

        if (!isInteger(positionValue))
        {
            errors.push_back("Every beat position must be an integer");
        }
        else
        {
            const auto position = static_cast<juce::int64>(positionValue);
            require(position >= 1 && position <= numerator, "Beat positions must be within the meter", errors);
            require(positions.insert(position).second, "Beat positions must be unique", errors);
        }

        require(clickValue.isString() && supportedClicks.count(clickValue.toString()) == 1,
                "Every beat click must use a supported ClickType", errors);
    }

    require(static_cast<juce::int64>(positions.size()) == numerator, "Beat positions must cover the complete meter",
            errors);
}

void validateSound(juce::DynamicObject &sound, Errors &errors)
{
    const auto soundSet = sound.getProperty("soundSet");
    const auto volume = sound.getProperty("volume");
    const auto numericVolume = isNumber(volume) ? static_cast<double>(volume) : 0.0;

    require(soundSet.isString() && soundSet.toString().trim().isNotEmpty(), "sound.soundSet must be a non-empty string",
            errors);
    require(isNumber(volume) && std::isfinite(numericVolume) && numericVolume >= 0.0 && numericVolume <= 1.0,
            "sound.volume must be a finite number from 0 through 1", errors);
}

void validatePlayback(juce::DynamicObject &playback, Errors &errors)
{
    const auto mode = playback.getProperty("mode");
    require(mode.isString() && (mode.toString() == "internal" || mode.toString() == "host"),
            "playback.mode must be internal or host", errors);
}
} // namespace

ValidationResult validatePresetJson(const juce::String &jsonText)
{
    ValidationResult validation;

    if (jsonText.getNumBytesAsUTF8() > maximumPresetFileSizeBytes)
    {
        validation.errors.push_back("Preset exceeds the 1 MiB size limit");
        return validation;
    }

    juce::var root;
    const auto parseResult = juce::JSON::parse(jsonText, root);

    if (parseResult.failed())
    {
        validation.errors.push_back("Invalid JSON: " + parseResult.getErrorMessage());
        return validation;
    }

    if (!root.isObject())
    {
        validation.errors.push_back("The preset root must be an object");
        return validation;
    }

    auto *rootObject = root.getDynamicObject();
    require(rootObject->getProperty("schema").toString() == "tempoflow-preset", "schema must equal tempoflow-preset",
            validation.errors);

    const auto schemaVersion = rootObject->getProperty("schemaVersion");
    require(schemaVersion.isString() && isSupportedSchemaVersion(schemaVersion.toString()),
            "schemaVersion must be a compatible Semantic Versioning 1.x version", validation.errors);

    auto *metadata = requireObjectProperty(*rootObject, "metadata", validation.errors);
    auto *tempo = requireObjectProperty(*rootObject, "tempo", validation.errors);
    auto *meter = requireObjectProperty(*rootObject, "meter", validation.errors);
    auto *subdivision = requireObjectProperty(*rootObject, "subdivision", validation.errors);
    auto *pattern = requireObjectProperty(*rootObject, "pattern", validation.errors);
    auto *sound = requireObjectProperty(*rootObject, "sound", validation.errors);
    auto *playback = requireObjectProperty(*rootObject, "playback", validation.errors);

    if (metadata != nullptr)
        validateMetadata(*metadata, validation.errors);
    if (tempo != nullptr)
        validateTempo(*tempo, validation.errors);

    const auto numerator = meter != nullptr ? validateMeter(*meter, validation.errors) : 0;

    if (subdivision != nullptr)
        validateSubdivision(*subdivision, validation.errors);
    if (pattern != nullptr)
        validatePattern(*pattern, numerator, validation.errors);
    if (sound != nullptr)
        validateSound(*sound, validation.errors);
    if (playback != nullptr)
        validatePlayback(*playback, validation.errors);

    return validation;
}
} // namespace tempoflow::preset
