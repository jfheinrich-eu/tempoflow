#include <juce_core/juce_core.h>

#include <iostream>

int main()
{
    std::cout << "TempoFlow toolchain OK\n";
    std::cout << juce::SystemStats::getJUCEVersion().toStdString() << '\n';
    return 0;
}
