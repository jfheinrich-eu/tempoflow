#include "preset/PresetFileIO.h"
#include "preset/PresetValidator.h"

#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: TempoFlowPresetValidator <preset-file-or-directory> [...]\n";
        return 2;
    }

    std::vector<juce::File> inputs;
    for (int index = 1; index < argc; ++index)
        inputs.push_back(juce::File::getCurrentWorkingDirectory().getChildFile(argv[index]));

    const auto collection = tempoflow::preset::collectPresetFiles(inputs);
    for (const auto &error : collection.errors)
        std::cerr << error << '\n';

    if (collection.files.empty())
    {
        if (collection.errors.empty())
            std::cerr << "No .tempoflow files found.\n";
        return collection.errors.empty() ? 2 : 1;
    }

    bool allValid = collection.isValid();
    std::int64_t totalBytesRead = 0;
    for (const auto &file : collection.files)
    {
        const auto readResult = tempoflow::preset::readPresetFileBounded(file);
        if (!readResult.isValid())
        {
            std::cerr << file.getFullPathName() << ": " << readResult.error << '\n';
            allValid = false;
            continue;
        }

        if (totalBytesRead > tempoflow::preset::maximumTotalPresetBytes - readResult.bytesRead)
        {
            std::cerr << "Preset processing exceeds the 16 MiB cumulative size limit\n";
            allValid = false;
            break;
        }
        totalBytesRead += readResult.bytesRead;

        const auto validation = tempoflow::preset::validatePresetJson(readResult.content);
        if (validation.isValid())
        {
            std::cout << file.getFileName() << ": valid\n";
            continue;
        }

        allValid = false;
        std::cerr << file.getFileName() << ": invalid\n";
        for (const auto &error : validation.errors)
            std::cerr << "  - " << error << '\n';
    }

    return allValid ? 0 : 1;
}
