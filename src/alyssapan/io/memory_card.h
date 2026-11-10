// The PS2's two memory-card slots as folders on the host: <user dir>/memcard/mc<port>-<slot>/ (RosePan's
// layout, so a card carries over between the Pans). sdk/libmc.cpp is SCE's API in front of this class; the
// class keeps libmc's asynchronous handshake, because the game's mc_* step machines are built around it: a
// request does its work at once and parks its result, the next sync hands the result back once, and a sync
// with nothing parked answers "idle" (the path the step machines take to re-issue a lost request).
//
// Two facts the card answers that the game depends on: a listing starts with "." and "..", and a formatted
// 8 MB card has 8,135 free 1 KB clusters, less what its folder holds.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

struct sceMcTblGetDir;

namespace alyssapan::io
{

class MemoryCards
{
public:
    // libmc's values (sdk/libmc.h has the SCE names).
    enum class Sync
    {
        Idle = -1,
        Running = 0,
        Finished = 1,
    };

    enum class SeekFrom
    {
        Set = 0,
        Current = 1,
        End = 2,
    };

    struct Info
    {
        int type = 0;   // sceMcTypePS2
        int free = 0;   // free 1 KB clusters
        int format = 0; // 1: formatted
    };

    static constexpr int kPorts = 2;
    static constexpr int kSlotsPerPort = 1; // no multitap
    static constexpr int kClusterSize = 1024;
    static constexpr int kFreeClusters = 8135;
    static constexpr int kMaxOpenFiles = 8;

    // The card folders under `root` (one per port and slot, made on first use).
    explicit MemoryCards(std::filesystem::path root);
    ~MemoryCards();

    MemoryCards(const MemoryCards &) = delete;
    MemoryCards &operator=(const MemoryCards &) = delete;

    // The process's cards: ALYSSAPAN_MEMCARD_DIR when set (smoke runs, tests), else the user directory's
    // memcard/ (Paths::memoryCardDirectory).
    static MemoryCards &instance();

    const std::filesystem::path &root() const { return m_root; }
    std::filesystem::path cardFolder(int port, int slot) const;

    // Requests. Each answers 0 when it was accepted (its result waits for sync) or a negative libmc
    // rejection code (sceMcErrNotInit, sceMcErrPortSlot ...).
    int init();
    int end();
    Sync sync(int *command, int *result);

    int getInfo(int port, int slot, Info *info);
    int getDir(int port, int slot, std::string_view name, sceMcTblGetDir *table, int maxEntries);
    int open(int port, int slot, std::string_view name, int mode);
    int close(int file);
    int read(int file, void *buffer, int size);
    int write(int file, const void *buffer, int size);
    int seek(int file, int offset, SeekFrom from);
    int flush(int file);
    int makeDirectory(int port, int slot, std::string_view name);
    int remove(int port, int slot, std::string_view name);
    int rename(int port, int slot, std::string_view from, std::string_view newName);
    int changeDirectory(int port, int slot, std::string_view name, char *current);
    int entrySpace(int port, int slot, std::string_view path);
    int format(int port, int slot);
    int unformat(int port, int slot);

private:
    struct FileCloser
    {
        void operator()(std::FILE *file) const { std::fclose(file); }
    };
    using File = std::unique_ptr<std::FILE, FileCloser>;

    static bool validPortSlot(int port, int slot);
    // "DIR", "/DIR/FILE" and the listing wildcard "DIR/*" -> a path under the card's folder.
    std::filesystem::path resolve(int port, int slot, std::string_view name) const;
    int usedClusters(const std::filesystem::path &folder) const;
    std::FILE *file(int handle) const;
    int post(int result);

    std::filesystem::path m_root;
    mutable std::mutex m_lock;
    std::array<File, kMaxOpenFiles> m_files;
    bool m_initialised = false;
    bool m_pending = false;
    int m_result = 0;
};

} // namespace alyssapan::io
