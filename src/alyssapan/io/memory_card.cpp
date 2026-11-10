#include "alyssapan/io/memory_card.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <system_error>

#include "alyssapan/core/application.h"
#include "alyssapan/core/logger.h"
#include "sdk/libmc.h"

namespace fs = std::filesystem;

#ifdef _WIN32
#define ALYSSAPAN_FOPEN_MODE(text) L"" text
#else
#define ALYSSAPAN_FOPEN_MODE(text) text
#endif

namespace alyssapan::io
{
namespace
{

constexpr unsigned short kDirectoryAttributes =
    sceMcFileAttrSubdir | sceMcFileAttrReadable | sceMcFileAttrWriteable | sceMcFileAttrExecutable;
constexpr unsigned short kFileAttributes =
    sceMcFileAttrFile | sceMcFileAttrReadable | sceMcFileAttrWriteable | sceMcFileAttrClosed;

int clusters(std::uintmax_t bytes)
{
    return static_cast<int>((bytes + MemoryCards::kClusterSize - 1) / MemoryCards::kClusterSize);
}

bool isListing(std::string_view name)
{
    return !name.empty() && name.back() == '*';
}

} // namespace

MemoryCards::MemoryCards(fs::path root) : m_root(std::move(root)) {}

MemoryCards::~MemoryCards() = default;

MemoryCards &MemoryCards::instance()
{
    static MemoryCards cards = [] {
        if (const char *override = std::getenv("ALYSSAPAN_MEMCARD_DIR"); override != nullptr && *override)
            return MemoryCards(pathFromUtf8(override));
        return MemoryCards(Application::instance().paths().memoryCardDirectory());
    }();
    return cards;
}

fs::path MemoryCards::cardFolder(int port, int slot) const
{
    return m_root / ("mc" + std::to_string(port) + "-" + std::to_string(slot));
}

bool MemoryCards::validPortSlot(int port, int slot)
{
    return port >= 0 && port < kPorts && slot >= 0 && slot < kSlotsPerPort;
}

fs::path MemoryCards::resolve(int port, int slot, std::string_view name) const
{
    std::string relative(name);
    while (!relative.empty() && (relative.front() == '/' || relative.front() == '\\'))
        relative.erase(0, 1);
    if (isListing(relative))
    {
        relative.pop_back();
        if (!relative.empty() && (relative.back() == '/' || relative.back() == '\\'))
            relative.pop_back();
    }
    return cardFolder(port, slot) / pathFromUtf8(relative);
}

int MemoryCards::usedClusters(const fs::path &folder) const
{
    std::error_code ec;
    if (!fs::exists(folder, ec))
        return 0;

    int used = 0;
    for (fs::recursive_directory_iterator it(folder, ec), last; it != last && !ec; it.increment(ec))
        used += it->is_directory(ec) ? 1 : clusters(it->file_size(ec)); // a folder's entry table: 1 cluster
    return used;
}

std::FILE *MemoryCards::file(int handle) const
{
    if (handle < 0 || handle >= kMaxOpenFiles)
        return nullptr;
    return m_files[static_cast<std::size_t>(handle)].get();
}

int MemoryCards::post(int result)
{
    m_result = result;
    m_pending = true;
    return sceMcResSucceed;
}

// CT3's sceMcInit (0x01826d38) binds mcserv and refuses an IOP side older than mcserv 0x20a / mcman 0x20e
// (its strings at 0x01932010 / 0x01932038): the host card is the newest there is, so init only succeeds.
int MemoryCards::init()
{
    std::scoped_lock hold(m_lock);
    for (File &open : m_files)
        open.reset();
    m_pending = false;
    m_result = sceMcResSucceed;
    m_initialised = true;
    log::info("memory card: {}", pathToUtf8(m_root));
    return sceMcResSucceed;
}

// Closes what is open and drops the init flag. 1, like the ROM's.
int MemoryCards::end()
{
    std::scoped_lock hold(m_lock);
    for (File &open : m_files)
        open.reset();
    m_pending = false;
    m_initialised = false;
    return 1;
}

// Nothing is ever in flight on the host, so mode 0 (wait) and 1 (poll) answer alike.
MemoryCards::Sync MemoryCards::sync(int *command, int *result)
{
    std::scoped_lock hold(m_lock);
    if (command != nullptr)
        *command = 0;
    if (!m_pending)
        return Sync::Idle;
    if (result != nullptr)
        *result = m_result;
    m_pending = false;
    return Sync::Finished;
}

int MemoryCards::getInfo(int port, int slot, Info *info)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;

    const fs::path folder = cardFolder(port, slot);
    std::error_code ec;
    fs::create_directories(folder, ec);

    if (info != nullptr)
    {
        info->type = sceMcTypePS2;
        info->format = 1;
        info->free = std::max(0, kFreeClusters - usedClusters(folder));
    }
    // Never sceMcResChangedCard: a host card cannot be swapped, and mc_check retries on any negative.
    return post(sceMcResSucceed);
}

int MemoryCards::getDir(int port, int slot, std::string_view name, sceMcTblGetDir *table, int maxEntries)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    if (table == nullptr || maxEntries <= 0)
        return sceMcErrEntryName;

    std::memset(table, 0, static_cast<std::size_t>(maxEntries) * sizeof(sceMcTblGetDir));

    const fs::path folder = resolve(port, slot, name);
    std::error_code ec;
    // A "DIR/*" listing of a folder that is not there answers "no entry", as the card does.
    if (!fs::is_directory(folder, ec))
        return post(isListing(name) ? sceMcResNoEntry : 0);

    int count = 0;
    for (const char *dot : {".", ".."})
    {
        if (count == maxEntries)
            break;
        table[count].AttrFile = kDirectoryAttributes;
        std::strcpy(reinterpret_cast<char *>(table[count].EntryName), dot);
        ++count;
    }

    for (fs::directory_iterator it(folder, ec), last; it != last && !ec && count < maxEntries; it.increment(ec))
    {
        const std::string leaf = pathToUtf8(it->path().filename());
        if (leaf.size() >= sizeof(table[count].EntryName))
            continue;
        if (it->is_directory(ec))
        {
            table[count].AttrFile = kDirectoryAttributes & ~sceMcFileAttrExecutable;
        }
        else
        {
            table[count].AttrFile = kFileAttributes;
            table[count].FileSizeByte = static_cast<unsigned int>(it->file_size(ec));
        }
        std::memcpy(table[count].EntryName, leaf.c_str(), leaf.size() + 1);
        ++count;
    }
    return post(count);
}

int MemoryCards::open(int port, int slot, std::string_view name, int mode)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;

    const fs::path path = resolve(port, slot, name);
    // fs::path::value_type: wchar_t on Windows, so a card folder under a non-ASCII user name opens too.
    const fs::path::value_type *access = ALYSSAPAN_FOPEN_MODE("rb");
    if ((mode & sceMcFileCreateFile) != 0)
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        access = ALYSSAPAN_FOPEN_MODE("w+b");
    }
    else if ((mode & sceMcFileAttrWriteable) != 0)
    {
        access = ALYSSAPAN_FOPEN_MODE("r+b");
    }

    for (std::size_t handle = 0; handle < m_files.size(); ++handle)
    {
        if (m_files[handle])
            continue;
#ifdef _WIN32
        m_files[handle].reset(_wfopen(path.c_str(), access));
#else
        m_files[handle].reset(std::fopen(path.c_str(), access));
#endif
        if (!m_files[handle])
            return post(sceMcResNoEntry);
        return post(static_cast<int>(handle)); // the descriptor travels back as the result
    }
    return post(sceMcResUpLimitHandle);
}

int MemoryCards::close(int handle)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (file(handle) == nullptr)
        return post(sceMcResNoEntry);
    m_files[static_cast<std::size_t>(handle)].reset();
    return post(sceMcResSucceed);
}

int MemoryCards::read(int handle, void *buffer, int size)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    std::FILE *const open = file(handle);
    if (open == nullptr || buffer == nullptr || size < 0)
        return post(sceMcResNoEntry);
    return post(static_cast<int>(std::fread(buffer, 1, static_cast<std::size_t>(size), open)));
}

int MemoryCards::write(int handle, const void *buffer, int size)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    std::FILE *const open = file(handle);
    if (open == nullptr || buffer == nullptr || size < 0)
        return post(sceMcResNoEntry);
    const std::size_t written = std::fwrite(buffer, 1, static_cast<std::size_t>(size), open);
    std::fflush(open);
    return post(static_cast<int>(written));
}

int MemoryCards::seek(int handle, int offset, SeekFrom from)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    std::FILE *const open = file(handle);
    if (open == nullptr)
        return post(sceMcResNoEntry);
    const int whence = from == SeekFrom::Current ? SEEK_CUR : from == SeekFrom::End ? SEEK_END : SEEK_SET;
    if (std::fseek(open, offset, whence) != 0)
        return post(sceMcResDeniedPermit);
    return post(static_cast<int>(std::ftell(open))); // the new position, as the card answers
}

int MemoryCards::flush(int handle)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    std::FILE *const open = file(handle);
    if (open == nullptr)
        return post(sceMcResNoEntry);
    std::fflush(open);
    return post(sceMcResSucceed);
}

int MemoryCards::makeDirectory(int port, int slot, std::string_view name)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    const fs::path path = resolve(port, slot, name);
    std::error_code ec;
    if (fs::exists(path, ec))
        return post(sceMcResNoEntry); // the card refuses a name already taken
    fs::create_directories(path, ec);
    return post(fs::is_directory(path, ec) ? sceMcResSucceed : sceMcResFullDevice);
}

// A folder goes only once it is empty, as on the card.
int MemoryCards::remove(int port, int slot, std::string_view name)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    const fs::path path = resolve(port, slot, name);
    std::error_code ec;
    if (!fs::exists(path, ec))
        return post(sceMcResNoEntry);
    if (fs::is_directory(path, ec) && !fs::is_empty(path, ec))
        return post(sceMcResNotEmpty);
    fs::remove(path, ec);
    return post(ec ? sceMcResDeniedPermit : sceMcResSucceed);
}

int MemoryCards::rename(int port, int slot, std::string_view from, std::string_view newName)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    const fs::path source = resolve(port, slot, from);
    std::error_code ec;
    if (!fs::exists(source, ec))
        return post(sceMcResNoEntry);
    fs::rename(source, source.parent_path() / pathFromUtf8(newName), ec);
    return post(ec ? sceMcResDeniedPermit : sceMcResSucceed);
}

// The host keeps no current directory: the folder must exist, and the current one reads "/".
int MemoryCards::changeDirectory(int port, int slot, std::string_view name, char *current)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    if (current != nullptr)
        std::strcpy(current, "/");
    std::error_code ec;
    return post(fs::is_directory(resolve(port, slot, name), ec) ? sceMcResSucceed : sceMcResNoEntry);
}

// Entries still free in the folder's last cluster: two 512-byte entries per cluster, "." and ".." included.
int MemoryCards::entrySpace(int port, int slot, std::string_view path)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    const fs::path folder = resolve(port, slot, path);
    std::error_code ec;
    if (!fs::is_directory(folder, ec))
        return post(sceMcResNoEntry);
    int count = 2;
    for (fs::directory_iterator it(folder, ec), last; it != last && !ec; it.increment(ec))
        ++count;
    return post(count % 2);
}

int MemoryCards::format(int port, int slot)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    const fs::path folder = cardFolder(port, slot);
    std::error_code ec;
    fs::remove_all(folder, ec);
    fs::create_directories(folder, ec);
    return post(ec ? sceMcResDeniedPermit : sceMcResSucceed);
}

// An unformatted host card is one whose folder is gone; the next getInfo makes it again, formatted.
int MemoryCards::unformat(int port, int slot)
{
    std::scoped_lock hold(m_lock);
    if (!m_initialised)
        return sceMcErrNotInit;
    if (!validPortSlot(port, slot))
        return sceMcErrPortSlot;
    std::error_code ec;
    fs::remove_all(cardFolder(port, slot), ec);
    return post(ec ? sceMcResDeniedPermit : sceMcResSucceed);
}

} // namespace alyssapan::io
