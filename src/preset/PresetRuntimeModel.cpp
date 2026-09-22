#include "PresetRuntimeModel.h"

#include "PresetValidator.h"

#include <algorithm>

namespace tempoflow::preset
{
namespace
{
template <typename Enum> struct EnumValue;

template <> struct EnumValue<ClickType>
{
    static bool fromString(const juce::String &value, ClickType &result) noexcept
    {
        if (value == "accent")
            result = ClickType::accent;
        else if (value == "normal")
            result = ClickType::normal;
        else if (value == "high")
            result = ClickType::high;
        else if (value == "low")
            result = ClickType::low;
        else if (value == "wood")
            result = ClickType::wood;
        else if (value == "mute")
            result = ClickType::mute;
        else
            return false;

        return true;
    }
};

template <> struct EnumValue<SubdivisionMode>
{
    static bool fromString(const juce::String &value, SubdivisionMode &result) noexcept
    {
        if (value == "none")
            result = SubdivisionMode::none;
        else if (value == "triplet")
            result = SubdivisionMode::triplet;
        else
            return false;

        return true;
    }
};

template <> struct EnumValue<PlaybackMode>
{
    static bool fromString(const juce::String &value, PlaybackMode &result) noexcept
    {
        if (value == "internal")
            result = PlaybackMode::internal;
        else if (value == "host")
            result = PlaybackMode::host;
        else
            return false;

        return true;
    }
};

template <typename Enum> bool parseEnum(const juce::var &value, Enum &result) noexcept
{
    return value.isString() && EnumValue<Enum>::fromString(value.toString(), result);
}

[[nodiscard]] juce::var parseValidatedJson(const juce::String &jsonText, std::vector<juce::String> &errors)
{
    const auto validation = validatePresetJson(jsonText);
    if (!validation.isValid())
    {
        errors = validation.errors;
        return {};
    }

    juce::var root;
    const auto parseResult = juce::JSON::parse(jsonText, root);
    if (parseResult.failed() || !root.isObject())
    {
        errors.push_back("Validated preset could not be parsed as an object");
        return {};
    }

    return root;
}
} // namespace

RuntimePresetResult parsePresetJson(const juce::String &jsonText)
{
    RuntimePresetResult result;
    const auto root = parseValidatedJson(jsonText, result.errors);
    if (!result.errors.empty())
        return result;

    const auto *rootObject = root.getDynamicObject();
    const auto *metadata = rootObject->getProperty("metadata").getDynamicObject();
    const auto *tempo = rootObject->getProperty("tempo").getDynamicObject();
    const auto *meter = rootObject->getProperty("meter").getDynamicObject();
    const auto *subdivision = rootObject->getProperty("subdivision").getDynamicObject();
    const auto *pattern = rootObject->getProperty("pattern").getDynamicObject();
    const auto *sound = rootObject->getProperty("sound").getDynamicObject();
    const auto *playback = rootObject->getProperty("playback").getDynamicObject();

    result.preset.schemaVersion = rootObject->getProperty("schemaVersion").toString();
    result.preset.name = metadata->getProperty("name").toString();
    result.preset.bpm = static_cast<double>(tempo->getProperty("bpm"));

    result.preset.meter.numerator = static_cast<std::int64_t>(meter->getProperty("numerator"));
    result.preset.meter.denominator = static_cast<std::int64_t>(meter->getProperty("denominator"));
    for (const auto &group : *meter->getProperty("grouping").getArray())
        result.preset.meter.grouping.push_back(static_cast<std::int64_t>(group));

    result.preset.subdivision.partsPerBeat = static_cast<std::int64_t>(subdivision->getProperty("partsPerBeat"));
    static_cast<void>(parseEnum(subdivision->getProperty("mode"), result.preset.subdivision.mode));

    for (const auto &beatValue : *pattern->getProperty("beats").getArray())
    {
        const auto *beat = beatValue.getDynamicObject();
        RuntimeBeat runtimeBeat;
        runtimeBeat.beat = static_cast<std::int64_t>(beat->getProperty("beat"));
        static_cast<void>(parseEnum(beat->getProperty("click"), runtimeBeat.click));
        result.preset.pattern.beats.push_back(runtimeBeat);
    }

    result.preset.sound.soundSet = sound->getProperty("soundSet").toString();
    result.preset.sound.volume = static_cast<double>(sound->getProperty("volume"));
    static_cast<void>(parseEnum(playback->getProperty("mode"), result.preset.playbackMode));

    std::sort(result.preset.pattern.beats.begin(), result.preset.pattern.beats.end(),
              [](const auto &left, const auto &right) { return left.beat < right.beat; });
    return result;
}

RuntimePresetResult loadPresetFile(const juce::File &file)
{
    const auto readResult = readPresetFileBounded(file);
    if (!readResult.isValid())
        return {{}, {file.getFullPathName() + ": " + readResult.error}};

    auto result = parsePresetJson(readResult.content);
    for (auto &error : result.errors)
        error = file.getFullPathName() + ": " + error;

    return result;
}
} // namespace tempoflow::preset
