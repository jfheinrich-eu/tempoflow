#include "preset/PresetFileIO.h"
#include "preset/PresetValidator.h"

#include <juce_audio_processors_headless/format_types/juce_VST3Headers.h>

#include <public.sdk/source/common/memorystream.h>
#include <public.sdk/source/vst/hosting/module.h>
#include <public.sdk/source/vst/vstpresetfile.h>

#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr auto expectedPluginName = "TempoFlow";

struct ComponentSession
{
    ComponentSession() = default;
    ComponentSession(const ComponentSession &) = delete;
    ComponentSession &operator=(const ComponentSession &) = delete;

    ComponentSession(ComponentSession &&other) noexcept
        : component(std::move(other.component)), initialized(std::exchange(other.initialized, false))
    {
    }

    ComponentSession &operator=(ComponentSession &&other) noexcept
    {
        if (this != &other)
        {
            terminate();
            component = std::move(other.component);
            initialized = std::exchange(other.initialized, false);
        }
        return *this;
    }

    ~ComponentSession()
    {
        terminate();
    }

    void terminate() noexcept
    {
        if (initialized && component != nullptr)
            component->terminate();

        initialized = false;
        component = nullptr;
    }

    Steinberg::IPtr<Steinberg::Vst::IComponent> component;
    bool initialized = false;
};

struct GeneratedPreset
{
    juce::File destination;
    juce::MemoryBlock data;
};

[[nodiscard]] juce::File resolvePath(const char *argument)
{
    return juce::File::isAbsolutePath(argument) ? juce::File(argument)
                                                : juce::File::getCurrentWorkingDirectory().getChildFile(argument);
}

[[nodiscard]] std::optional<VST3::Hosting::ClassInfo> findTempoFlowComponentClass(
    const VST3::Hosting::PluginFactory &factory, std::string &error)
{
    std::optional<VST3::Hosting::ClassInfo> result;

    for (const auto &classInfo : factory.classInfos())
    {
        if (classInfo.category() != kVstAudioEffectClass || classInfo.name() != expectedPluginName)
            continue;

        if (result)
        {
            error = "The module exposes more than one TempoFlow audio component";
            return std::nullopt;
        }

        result = classInfo;
    }

    if (!result)
        error = "The module does not expose the TempoFlow audio component";

    return result;
}

[[nodiscard]] ComponentSession createComponent(const VST3::Hosting::PluginFactory &factory,
                                               const VST3::Hosting::ClassInfo &classInfo,
                                               Steinberg::Vst::HostApplication &host, std::string &error)
{
    ComponentSession session;
    session.component = factory.createInstance<Steinberg::Vst::IComponent>(classInfo.ID());
    if (session.component == nullptr)
    {
        error = "The TempoFlow VST3 component could not be created";
        return session;
    }

    if (session.component->initialize(&host) != Steinberg::kResultOk)
    {
        error = "The TempoFlow VST3 component could not be initialized";
        session.component = nullptr;
        return session;
    }

    session.initialized = true;
    return session;
}

[[nodiscard]] bool setComponentState(Steinberg::Vst::IComponent &component, const juce::String &json,
                                     std::string &error)
{
    const auto utf8 = json.toUTF8();
    Steinberg::MemoryStream input(const_cast<char *>(utf8.getAddress()),
                                  static_cast<Steinberg::TSize>(utf8.sizeInBytes() - 1));

    if (component.setState(&input) != Steinberg::kResultOk)
    {
        error = "The TempoFlow VST3 component rejected the validated preset state";
        return false;
    }

    return true;
}

[[nodiscard]] bool componentStateStartsWith(Steinberg::Vst::IComponent &component, const juce::String &json,
                                            std::string &error)
{
    Steinberg::MemoryStream state;
    if (component.getState(&state) != Steinberg::kResultOk)
    {
        error = "The restored TempoFlow VST3 component state could not be read";
        return false;
    }

    const auto utf8 = json.toUTF8();
    const auto jsonSize = static_cast<std::size_t>(utf8.sizeInBytes() - 1);
    if (state.getSize() < static_cast<Steinberg::TSize>(jsonSize) ||
        std::memcmp(state.getData(), utf8.getAddress(), jsonSize) != 0)
    {
        error = "The restored TempoFlow VST3 component state does not match the source preset";
        return false;
    }

    return true;
}

[[nodiscard]] bool createAndVerifyPreset(const VST3::Hosting::PluginFactory &factory,
                                         const VST3::Hosting::ClassInfo &classInfo,
                                         Steinberg::Vst::HostApplication &host, const juce::String &json,
                                         juce::MemoryBlock &result, std::string &error)
{
    auto sourceComponent = createComponent(factory, classInfo, host, error);
    if (sourceComponent.component == nullptr || !setComponentState(*sourceComponent.component, json, error))
        return false;

    Steinberg::MemoryStream output;
    const Steinberg::FUID classId(classInfo.ID().data());
    if (!Steinberg::Vst::PresetFile::savePreset(&output, classId, sourceComponent.component.get()))
    {
        error = "Steinberg PresetFile could not serialize the TempoFlow component state";
        return false;
    }

    auto restoredComponent = createComponent(factory, classInfo, host, error);
    if (restoredComponent.component == nullptr)
        return false;

    Steinberg::MemoryStream verificationStream(output.getData(), output.getSize());
    if (!Steinberg::Vst::PresetFile::loadPreset(&verificationStream, classId, restoredComponent.component.get()))
    {
        error = "Steinberg PresetFile could not restore the generated preset";
        return false;
    }

    if (!componentStateStartsWith(*restoredComponent.component, json, error))
        return false;

    result.replaceAll(output.getData(), static_cast<std::size_t>(output.getSize()));
    return true;
}

[[nodiscard]] bool writeGeneratedPresets(const std::vector<GeneratedPreset> &presets, std::string &error)
{
    for (const auto &preset : presets)
    {
        if (!preset.destination.replaceWithData(preset.data.getData(), preset.data.getSize()))
        {
            error = "Could not write " + preset.destination.getFullPathName().toStdString();
            return false;
        }
    }

    return true;
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: TempoFlowVst3PresetGenerator <plugin.vst3> <source-directory> <output-directory>\n";
        return 2;
    }

    const auto pluginPath = resolvePath(argv[1]);
    const auto sourceDirectory = resolvePath(argv[2]);
    const auto outputDirectory = resolvePath(argv[3]);

    if (!pluginPath.exists())
    {
        std::cerr << "VST3 module not found: " << pluginPath.getFullPathName() << '\n';
        return 1;
    }

    const auto collection = tempoflow::preset::collectPresetFiles({sourceDirectory});
    for (const auto &collectionError : collection.errors)
        std::cerr << collectionError << '\n';

    if (!collection.isValid() || collection.files.empty())
    {
        if (collection.files.empty())
            std::cerr << "No .tempoflow files found in " << sourceDirectory.getFullPathName() << '\n';
        return 1;
    }

    std::string moduleError;
    auto module = VST3::Hosting::Module::create(pluginPath.getFullPathName().toStdString(), moduleError);
    if (module == nullptr)
    {
        std::cerr << "Could not load the VST3 module: " << moduleError << '\n';
        return 1;
    }

    Steinberg::Vst::HostApplication host;
    const auto &factory = module->getFactory();
    factory.setHostContext(&host);

    std::string error;
    const auto componentClass = findTempoFlowComponentClass(factory, error);
    if (!componentClass)
    {
        std::cerr << error << '\n';
        return 1;
    }

    std::set<juce::String> outputNames;
    std::vector<GeneratedPreset> generatedPresets;
    generatedPresets.reserve(collection.files.size());

    for (const auto &sourceFile : collection.files)
    {
        const auto readResult = tempoflow::preset::readPresetFileBounded(sourceFile);
        if (!readResult.isValid())
        {
            std::cerr << sourceFile.getFullPathName() << ": " << readResult.error << '\n';
            return 1;
        }

        const auto validation = tempoflow::preset::validatePresetJson(readResult.content);
        if (!validation.isValid())
        {
            std::cerr << sourceFile.getFileName() << ": invalid\n";
            for (const auto &validationError : validation.errors)
                std::cerr << "  - " << validationError << '\n';
            return 1;
        }

        const auto outputName = sourceFile.getFileNameWithoutExtension() + ".vstpreset";
        if (!outputNames.insert(outputName).second)
        {
            std::cerr << "Duplicate output preset name: " << outputName << '\n';
            return 1;
        }

        GeneratedPreset generated{outputDirectory.getChildFile(outputName), {}};
        if (!createAndVerifyPreset(factory, *componentClass, host, readResult.content, generated.data, error))
        {
            std::cerr << sourceFile.getFileName() << ": " << error << '\n';
            return 1;
        }

        generatedPresets.push_back(std::move(generated));
    }

    if (!outputDirectory.createDirectory())
    {
        std::cerr << "Could not create output directory: " << outputDirectory.getFullPathName() << '\n';
        return 1;
    }

    if (!writeGeneratedPresets(generatedPresets, error))
    {
        std::cerr << error << '\n';
        return 1;
    }

    for (const auto &preset : generatedPresets)
        std::cout << preset.destination.getFileName() << ": generated and verified\n";

    std::cout << "Generated " << generatedPresets.size() << " native VST3 factory presets for component "
              << componentClass->ID().toString() << ".\n";
    return 0;
}
