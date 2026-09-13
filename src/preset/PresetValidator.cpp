#include "PresetValidator.h"

#include <array>
#include <cmath>
#include <limits>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace tempoflow::preset
{
namespace
{
using Errors = std::vector<juce::String>;

struct JsonContainerState
{
    char openingCharacter = 0;
    std::size_t propertyCount = 0;
    std::size_t commaCount = 0;
    bool hasContent = false;
};

void require(bool condition, juce::String message, Errors &errors)
{
    if (!condition)
        errors.push_back(std::move(message));
}

[[nodiscard]] juce::String validateJsonComplexity(const juce::String &jsonText)
{
    const std::string_view json(jsonText.toRawUTF8(), static_cast<std::size_t>(jsonText.getNumBytesAsUTF8()));
    std::vector<JsonContainerState> containers;
    std::size_t containerCount = 0;
    std::size_t totalPropertyCount = 0;
    std::size_t totalArrayElementCount = 0;
    bool insideString = false;
    bool escaped = false;

    for (const auto character : json)
    {
        if (insideString)
        {
            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (character == '\\')
            {
                escaped = true;
                continue;
            }

            if (character == '"')
                insideString = false;
            continue;
        }

        if (character == '"')
        {
            insideString = true;
            if (!containers.empty())
                containers.back().hasContent = true;
            continue;
        }

        if (character == '{' || character == '[')
        {
            if (!containers.empty())
                containers.back().hasContent = true;

            ++containerCount;
            if (containerCount > maximumJsonContainerCount)
                return "JSON exceeds the 2048-container limit";

            containers.push_back({character});
            if (containers.size() > maximumJsonNestingDepth)
                return "JSON exceeds the 32-level nesting limit";
            continue;
        }

        if (character == ':' && !containers.empty() && containers.back().openingCharacter == '{')
        {
            auto &object = containers.back();
            ++object.propertyCount;
            ++totalPropertyCount;
            if (object.propertyCount > maximumJsonPropertiesPerObject)
                return "JSON object exceeds the 256-property limit";
            if (totalPropertyCount > maximumJsonTotalProperties)
                return "JSON exceeds the 8192-property total limit";
            continue;
        }

        if (character == ',' && !containers.empty() && containers.back().openingCharacter == '[')
        {
            ++containers.back().commaCount;
            continue;
        }

        if (character == '}' || character == ']')
        {
            if (containers.empty())
                continue;

            const auto expectedOpening = character == '}' ? '{' : '[';
            if (containers.back().openingCharacter != expectedOpening)
                continue;

            const auto completed = containers.back();
            containers.pop_back();
            if (completed.openingCharacter == '[' && completed.hasContent)
            {
                const auto elementCount = completed.commaCount + 1;
                if (elementCount > maximumJsonArrayElements)
                    return "JSON array exceeds the 4096-element limit";
                if (totalArrayElementCount > maximumJsonTotalArrayElements - elementCount)
                    return "JSON exceeds the 16384-array-element total limit";
                totalArrayElementCount += elementCount;
            }
            continue;
        }

        if (!containers.empty() && character != ',' && character != ':' && character != ' ' && character != '\t' &&
            character != '\r' && character != '\n')
            containers.back().hasContent = true;
    }

    return {};
}

[[nodiscard]] bool isInteger(const juce::var &value) noexcept
{
    if (value.isInt() || value.isInt64())
        return true;

    if (!value.isDouble())
        return false;

    const auto number = static_cast<double>(value);
    constexpr auto int64LowerBound = -9223372036854775808.0;
    constexpr auto int64ExclusiveUpperBound = 9223372036854775808.0;
    return std::isfinite(number) && std::trunc(number) == number && number >= int64LowerBound &&
           number < int64ExclusiveUpperBound;
}

[[nodiscard]] bool getInteger(const juce::var &value, juce::int64 &result) noexcept
{
    if (!isInteger(value))
        return false;

    result = static_cast<juce::int64>(value);
    return true;
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

[[nodiscard]] bool isLeapYear(int year) noexcept
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

[[nodiscard]] bool parseFixedDigits(std::string_view text, std::size_t offset, std::size_t count, int &result) noexcept
{
    if (offset > text.size() || count > text.size() - offset)
        return false;

    result = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto character = text.at(offset + index);
        if (character < '0' || character > '9')
            return false;

        result = result * 10 + (character - '0');
    }

    return true;
}

[[nodiscard]] bool isValidDateTime(const juce::String &value)
{
    const auto text = value.toStdString();
    constexpr std::size_t minimumDateTimeLength = 20;
    if (text.size() < minimumDateTimeLength || text.at(4) != '-' || text.at(7) != '-' ||
        (text.at(10) != 'T' && text.at(10) != 't') || text.at(13) != ':' || text.at(16) != ':')
        return false;

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!parseFixedDigits(text, 0, 4, year) || !parseFixedDigits(text, 5, 2, month) ||
        !parseFixedDigits(text, 8, 2, day) || !parseFixedDigits(text, 11, 2, hour) ||
        !parseFixedDigits(text, 14, 2, minute) || !parseFixedDigits(text, 17, 2, second))
        return false;

    if (month < 1 || month > 12 || hour > 23 || minute > 59 || second > 60)
        return false;

    constexpr std::array daysPerMonth{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const auto maximumDay = month == 2 && isLeapYear(year) ? 29 : daysPerMonth.at(static_cast<std::size_t>(month - 1));
    if (day < 1 || day > maximumDay)
        return false;

    std::size_t position = 19;
    if (text.at(position) == '.')
    {
        const auto fractionStart = ++position;
        while (position < text.size() && text.at(position) >= '0' && text.at(position) <= '9')
            ++position;

        if (position == fractionStart)
            return false;
    }

    if (position >= text.size())
        return false;

    if (text.at(position) == 'Z' || text.at(position) == 'z')
        return position + 1 == text.size();

    constexpr std::size_t offsetLength = 6;
    if ((text.at(position) != '+' && text.at(position) != '-') || text.size() - position != offsetLength ||
        text.at(position + 3) != ':')
        return false;

    int offsetHour = 0;
    int offsetMinute = 0;
    if (!parseFixedDigits(text, position + 1, 2, offsetHour) || !parseFixedDigits(text, position + 4, 2, offsetMinute))
        return false;

    return offsetHour <= 23 && offsetMinute <= 59;
}

void validateOptionalString(juce::DynamicObject &object, const juce::Identifier &name, Errors &errors)
{
    if (object.hasProperty(name))
        require(object.getProperty(name).isString(), "metadata." + name.toString() + " must be a string", errors);
}

void validateOptionalTimestamp(juce::DynamicObject &metadata, const juce::Identifier &name, Errors &errors)
{
    if (!metadata.hasProperty(name))
        return;

    const auto value = metadata.getProperty(name);
    require(value.isString() && (value.toString().isEmpty() || isValidDateTime(value.toString())),
            "metadata." + name.toString() + " must be empty or a valid RFC 3339 date-time", errors);
}

void validateMetadata(juce::DynamicObject &metadata, Errors &errors)
{
    const auto name = metadata.getProperty("name");
    require(name.isString() && name.toString().trim().isNotEmpty(), "metadata.name must be a non-empty string", errors);

    validateOptionalString(metadata, "description", errors);
    validateOptionalString(metadata, "category", errors);
    validateOptionalString(metadata, "author", errors);
    validateOptionalTimestamp(metadata, "createdAt", errors);
    validateOptionalTimestamp(metadata, "updatedAt", errors);

    if (!metadata.hasProperty("tags"))
        return;

    const auto tags = metadata.getProperty("tags");
    if (!tags.isArray())
    {
        errors.push_back("metadata.tags must be an array of strings");
        return;
    }

    for (const auto &tag : *tags.getArray())
    {
        if (!tag.isString())
        {
            errors.push_back("metadata.tags must contain only strings");
            return;
        }
    }
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

    juce::int64 numerator = 0;
    const auto hasValidNumerator = getInteger(numeratorValue, numerator);
    require(hasValidNumerator && numerator >= 1, "meter.numerator must be an integer of at least 1", errors);

    juce::int64 denominator = 0;
    const auto hasValidDenominator = getInteger(denominatorValue, denominator);
    require(hasValidDenominator && (denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16),
            "meter.denominator must be 2, 4, 8, or 16", errors);

    if (!groupingValue.isArray() || groupingValue.getArray()->isEmpty())
    {
        errors.push_back("meter.grouping must be a non-empty array");
        return numerator;
    }

    juce::int64 groupingSum = 0;
    for (const auto &group : *groupingValue.getArray())
    {
        juce::int64 groupSize = 0;
        if (!getInteger(group, groupSize) || groupSize < 1)
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
    juce::int64 parts = 0;

    if (!mode.isString() || !getInteger(partsPerBeat, parts))
    {
        errors.push_back("subdivision.mode and subdivision.partsPerBeat are required");
        return;
    }

    const auto modeText = mode.toString();
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

        juce::int64 position = 0;
        if (!getInteger(positionValue, position))
        {
            errors.push_back("Every beat position must be an integer");
        }
        else
        {
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

    if (const auto complexityError = validateJsonComplexity(jsonText); complexityError.isNotEmpty())
    {
        validation.errors.push_back(complexityError);
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
