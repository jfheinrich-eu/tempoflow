#include "PresetFileIO.h"

#include "PresetValidator.h"

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <utility>

#if JUCE_WINDOWS
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tempoflow::preset
{
namespace
{
struct PendingDirectory
{
    juce::File directory;
    std::size_t depth = 0;
};

[[nodiscard]] juce::String comparisonPath(const juce::File &file)
{
#if JUCE_WINDOWS
    return file.getFullPathName().toLowerCase();
#else
    return file.getFullPathName();
#endif
}

void appendError(PresetFileCollection &collection, const juce::File &file, const juce::String &message)
{
    collection.errors.push_back(file.getFullPathName() + ": " + message);
}

#if JUCE_WINDOWS
class ScopedFileHandle
{
  public:
    explicit ScopedFileHandle(HANDLE handleToOwn) noexcept : handle(handleToOwn)
    {
    }

    ~ScopedFileHandle()
    {
        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }

    ScopedFileHandle(const ScopedFileHandle &) = delete;
    ScopedFileHandle &operator=(const ScopedFileHandle &) = delete;

    [[nodiscard]] HANDLE get() const noexcept
    {
        return handle;
    }

  private:
    HANDLE handle = INVALID_HANDLE_VALUE;
};
#else
class ScopedFileDescriptor
{
  public:
    explicit ScopedFileDescriptor(int descriptorToOwn) noexcept : descriptor(descriptorToOwn)
    {
    }

    ~ScopedFileDescriptor()
    {
        if (descriptor >= 0)
            ::close(descriptor);
    }

    ScopedFileDescriptor(const ScopedFileDescriptor &) = delete;
    ScopedFileDescriptor &operator=(const ScopedFileDescriptor &) = delete;

    [[nodiscard]] int get() const noexcept
    {
        return descriptor;
    }

  private:
    int descriptor = -1;
};
#endif
} // namespace

PresetFileCollection collectPresetFiles(const std::vector<juce::File> &inputs)
{
    PresetFileCollection collection;
    std::vector<PendingDirectory> pendingDirectories;
    std::set<juce::String> knownFiles;
    std::set<juce::String> knownDirectories;
    std::size_t scannedEntries = 0;
    std::int64_t reportedTotalBytes = 0;
    bool resourceLimitReached = false;

    const auto appendFile = [&](const juce::File &file) {
        if (resourceLimitReached)
            return;

        const auto path = comparisonPath(file);
        if (!knownFiles.insert(path).second)
            return;

        if (collection.files.size() >= maximumPresetFileCount)
        {
            collection.errors.push_back("Preset discovery exceeds the 1024-file limit");
            resourceLimitReached = true;
            return;
        }

        const auto size = file.getSize();
        if (size < 0 || size > maximumPresetFileSizeBytes)
        {
            appendError(collection, file, "exceeds the 1 MiB size limit");
            return;
        }

        if (reportedTotalBytes > maximumTotalPresetBytes - size)
        {
            collection.errors.push_back("Preset discovery exceeds the 16 MiB cumulative size limit");
            resourceLimitReached = true;
            return;
        }

        reportedTotalBytes += size;
        collection.files.push_back(file);
    };

    const auto appendDirectory = [&](const juce::File &directory, std::size_t depth) {
        const auto path = comparisonPath(directory);
        if (knownDirectories.insert(path).second)
            pendingDirectories.push_back({directory, depth});
    };

    for (const auto &input : inputs)
    {
        if (!input.exists())
        {
            appendError(collection, input, "file or directory not found");
            continue;
        }

        if (input.isSymbolicLink())
        {
            appendError(collection, input, "symbolic links and reparse points are not allowed");
            continue;
        }

        if (input.isDirectory())
            appendDirectory(input, 0);
        else
            appendFile(input);
    }

    while (!pendingDirectories.empty() && !resourceLimitReached)
    {
        const auto pending = std::move(pendingDirectories.back());
        pendingDirectories.pop_back();

        for (const auto &entry : juce::RangedDirectoryIterator(
                 pending.directory, false, "*", juce::File::findFilesAndDirectories, juce::File::FollowSymlinks::no))
        {
            if (++scannedEntries > maximumScannedDirectoryEntries)
            {
                collection.errors.push_back("Preset discovery exceeds the 10000-entry scan limit");
                resourceLimitReached = true;
                break;
            }

            const auto file = entry.getFile();
            if (file.isSymbolicLink())
            {
                appendError(collection, file, "symbolic links and reparse points are not allowed");
                continue;
            }

            if (entry.isDirectory())
            {
                if (pending.depth >= maximumPresetDirectoryDepth)
                {
                    appendError(collection, file, "exceeds the 16-level directory depth limit");
                    continue;
                }

                appendDirectory(file, pending.depth + 1);
                continue;
            }

            if (file.hasFileExtension("tempoflow"))
                appendFile(file);

            if (resourceLimitReached)
                break;
        }
    }

    std::sort(collection.files.begin(), collection.files.end(),
              [](const auto &left, const auto &right) { return comparisonPath(left) < comparisonPath(right); });
    return collection;
}

PresetFileReadResult readPresetFileBounded(const juce::File &file)
{
    PresetFileReadResult result;

#if JUCE_WINDOWS
    const ScopedFileHandle handle(
        CreateFileW(file.getFullPathName().toWideCharPointer(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (handle.get() == INVALID_HANDLE_VALUE)
    {
        result.error = "could not open file for reading";
        return result;
    }

    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(handle.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)))
    {
        result.error = "could not inspect the opened file";
        return result;
    }

    if ((attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
        result.error = "symbolic links and reparse points are not allowed";
        return result;
    }

    if ((attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || GetFileType(handle.get()) != FILE_TYPE_DISK)
    {
        result.error = "input is not a regular file";
        return result;
    }

    juce::MemoryBlock bytes;
    std::array<std::byte, 64 * 1024> buffer{};
    constexpr auto maximumReadBytes = static_cast<std::size_t>(maximumPresetFileSizeBytes) + 1;

    while (bytes.getSize() < maximumReadBytes)
    {
        const auto remaining = maximumReadBytes - bytes.getSize();
        const auto requested = static_cast<DWORD>((std::min)(buffer.size(), remaining));
        DWORD currentRead = 0;
        if (!ReadFile(handle.get(), buffer.data(), requested, &currentRead, nullptr))
        {
            result.error = "failed while reading file";
            return result;
        }

        if (currentRead == 0)
            break;

        bytes.append(buffer.data(), currentRead);
    }
#else
    static_assert(O_NOFOLLOW != 0, "Secure preset reads require O_NOFOLLOW");
    auto openFlags = O_RDONLY | O_NOFOLLOW | O_NONBLOCK;
#ifdef O_CLOEXEC
    openFlags |= O_CLOEXEC;
#endif
    const ScopedFileDescriptor descriptor(::open(file.getFullPathName().toRawUTF8(), openFlags));
    if (descriptor.get() < 0)
    {
        result.error = errno == ELOOP ? "symbolic links are not allowed" : "could not open file for reading";
        return result;
    }

    struct stat attributes{};
    if (::fstat(descriptor.get(), &attributes) != 0)
    {
        result.error = "could not inspect the opened file";
        return result;
    }

    if (!S_ISREG(attributes.st_mode))
    {
        result.error = "input is not a regular file";
        return result;
    }

    juce::MemoryBlock bytes;
    std::array<std::byte, 64 * 1024> buffer{};
    constexpr auto maximumReadBytes = static_cast<std::size_t>(maximumPresetFileSizeBytes) + 1;

    while (bytes.getSize() < maximumReadBytes)
    {
        const auto remaining = maximumReadBytes - bytes.getSize();
        const auto requested = (std::min)(buffer.size(), remaining);
        const auto currentRead = ::read(descriptor.get(), buffer.data(), requested);
        if (currentRead < 0)
        {
            if (errno == EINTR)
                continue;

            result.error = "failed while reading file";
            return result;
        }

        if (currentRead == 0)
            break;

        bytes.append(buffer.data(), static_cast<std::size_t>(currentRead));
    }
#endif

    result.bytesRead = static_cast<std::int64_t>(bytes.getSize());
    if (result.bytesRead > maximumPresetFileSizeBytes)
    {
        result.error = "exceeds the 1 MiB size limit";
        return result;
    }

    juce::MemoryInputStream content(bytes.getData(), bytes.getSize(), false);
    result.content = content.readEntireStreamAsString();
    return result;
}
} // namespace tempoflow::preset
