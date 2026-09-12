#include "preset/PresetValidator.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
void appendPresetFiles(const juce::File& input, std::vector<juce::File>& files)
{
    if (input.isDirectory())
    {
        juce::Array<juce::File> directoryFiles;
        input.findChildFiles(directoryFiles, juce::File::findFiles, true, "*.tempoflow");
        for (const auto& file : directoryFiles)
            files.push_back(file);
        return;
    }

    files.push_back(input);
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: TempoFlowPresetValidator <preset-file-or-directory> [...]\n";
        return 2;
    }

    std::vector<juce::File> files;
    for (int index = 1; index < argc; ++index)
        appendPresetFiles(juce::File::getCurrentWorkingDirectory().getChildFile(argv[index]), files);

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        return left.getFullPathName() < right.getFullPathName();
    });

    if (files.empty())
    {
        std::cerr << "No .tempoflow files found.\n";
        return 2;
    }

    bool allValid = true;
    for (const auto& file : files)
    {
        if (!file.existsAsFile())
        {
            std::cerr << file.getFullPathName() << ": file not found\n";
            allValid = false;
            continue;
        }

        if (file.getSize() > tempoflow::preset::maximumPresetFileSizeBytes)
        {
            std::cerr << file.getFileName() << ": exceeds the 1 MiB size limit\n";
            allValid = false;
            continue;
        }

        const auto validation = tempoflow::preset::validatePresetJson(file.loadFileAsString());
        if (validation.isValid())
        {
            std::cout << file.getFileName() << ": valid\n";
            continue;
        }

        allValid = false;
        std::cerr << file.getFileName() << ": invalid\n";
        for (const auto& error : validation.errors)
            std::cerr << "  - " << error << '\n';
    }

    return allValid ? 0 : 1;
}
