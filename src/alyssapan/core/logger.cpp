#include "alyssapan/core/logger.h"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

#ifndef ALYSSAPAN_BUILD_COMMIT
#define ALYSSAPAN_BUILD_COMMIT "unknown"
#endif

namespace alyssapan::log
{
namespace
{

namespace fs = std::filesystem;

constexpr int kRotatedLogs = 3;

class Logger
{
public:
    void open(const fs::path &file)
    {
        std::scoped_lock lock(m_lock);
        rotate(file);
        m_file.open(file, std::ios::out | std::ios::trunc);
    }

    void write(Level level, std::string_view text)
    {
        if (level < m_threshold.load(std::memory_order_relaxed))
            return;
        writeAlways(level, text);
    }

    // Past the threshold: the banner and the failure hooks.
    void writeAlways(Level level, std::string_view text)
    {
        const char *const tag = levelTag(level);
        const std::string line = stamped(tag, text);

        std::scoped_lock lock(m_lock);
        if (level != Level::Debug || m_verbose)
        {
            std::fprintf(stdout, "%s%.*s\n", tag, static_cast<int>(text.size()), text.data());
            std::fflush(stdout);
        }
        if (m_file.is_open())
            m_file << line << std::flush;
    }

    void writeFromFault(std::string_view text)
    {
        std::unique_lock lock(m_lock, std::try_to_lock);
        if (lock.owns_lock() && m_file.is_open())
            m_file << stamped("", text) << std::flush;
    }

    void setThreshold(Level threshold) { m_threshold.store(threshold, std::memory_order_relaxed); }
    Level threshold() const { return m_threshold.load(std::memory_order_relaxed); }

private:
    static std::string stamped(const char *tag, std::string_view text)
    {
        const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
        return std::format("{:%H:%M:%S} {}{}\n", now, tag, text);
    }

    static const char *levelTag(Level level)
    {
        switch (level)
        {
        case Level::Warning:
            return "[W] ";
        case Level::Error:
            return "[E] ";
        default:
            return "";
        }
    }

    // alyssapan.log becomes alyssapan.1.log and so on, so a crash's log survives the next launch.
    static void rotate(const fs::path &file)
    {
        const auto numbered = [&file](int index) {
            fs::path path = file;
            path.replace_extension(std::format(".{}.log", index));
            return path;
        };

        std::error_code ignored;
        fs::remove(numbered(kRotatedLogs), ignored);
        for (int index = kRotatedLogs - 1; index > 0; index--)
            fs::rename(numbered(index), numbered(index + 1), ignored);
        fs::rename(file, numbered(1), ignored);
    }

    std::mutex m_lock;
    std::ofstream m_file;
    bool m_verbose = std::getenv("ALYSSAPAN_LOG_DEBUG") != nullptr;
    std::atomic<Level> m_threshold = Level::Debug;
};

Logger &logger()
{
    static Logger instance;
    return instance;
}

Level levelOf(SDL_LogPriority priority)
{
    switch (priority)
    {
    case SDL_LOG_PRIORITY_CRITICAL:
    case SDL_LOG_PRIORITY_ERROR:
        return Level::Error;
    case SDL_LOG_PRIORITY_WARN:
        return Level::Warning;
    case SDL_LOG_PRIORITY_INFO:
        return Level::Info;
    default:
        return Level::Debug;
    }
}

void SDLCALL onSdlLog(void *, int, SDL_LogPriority priority, const char *message)
{
    logger().write(levelOf(priority), std::format("SDL: {}", message));
}

// A failed SDL_assert: the record first, then SDL's own handling (its dialog, or the debugger break), so
// the reason is in the file whichever button the player presses.
SDL_AssertState SDLCALL onSdlAssert(const SDL_AssertData *data, void *userdata)
{
    logger().writeAlways(Level::Error,
                         std::format("assertion failed: {} ({}:{}, {}), {} time(s)", data->condition,
                                     data->filename, data->linenum, data->function, data->trigger_count));
    return SDL_GetDefaultAssertionHandler()(data, userdata);
}

std::terminate_handler s_previousTerminate = nullptr;

// An exception nobody caught: what it said, then the runtime's own ending (abort).
[[noreturn]] void onTerminate()
{
    std::string what = "no exception (std::terminate called directly)";
    if (const std::exception_ptr current = std::current_exception())
    {
        try
        {
            std::rethrow_exception(current);
        }
        catch (const std::exception &exception)
        {
            what = std::format("uncaught exception: {}", exception.what());
        }
        catch (...)
        {
            what = "uncaught exception of a type that is not std::exception";
        }
    }
    try
    {
        logger().writeAlways(Level::Error, std::format("terminate: {}", what));
    }
    catch (...)
    {
    }
    if (s_previousTerminate != nullptr)
        s_previousTerminate();
    std::abort();
}

// abort(): the C runtime's assert prints its message on stderr (outside the log) and ends here, as does
// onTerminate. The handler only records; returning lets abort finish the process. Not strictly
// signal-safe, but abort raises on the calling thread on Windows, so the lock attempt is an ordinary one,
// and writeFromFault never waits for it.
extern "C" void onAbortSignal(int)
{
    logger().writeFromFault("[E] abort() called: the process ends here (an assert's message, if any, is on stderr)");
}

} // namespace

void open(const fs::path &file)
{
    logger().open(file);
    SDL_SetLogOutputFunction(onSdlLog, nullptr);
    logger().writeAlways(Level::Info,
                         std::format("AlyssaPan, build {} ({} {})", ALYSSAPAN_BUILD_COMMIT, __DATE__, __TIME__));
}

void write(Level level, std::string_view text)
{
    logger().write(level, text);
}

void setLevel(Level threshold)
{
    logger().setThreshold(threshold);
}

Level level()
{
    return logger().threshold();
}

void installFailureHooks()
{
    SDL_SetAssertionHandler(onSdlAssert, nullptr);
    s_previousTerminate = std::set_terminate(onTerminate);
    std::signal(SIGABRT, onAbortSignal);
}

void writeFromFault(std::string_view text)
{
    logger().writeFromFault(text);
}

} // namespace alyssapan::log
