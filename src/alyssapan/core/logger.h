// The log: one file in the user directory (alyssapan.log, older runs rotated away) plus stdout, where the
// tools read the [W] and [E] lines. Every record is flushed as it is written, because the interesting log
// is always the one of a process that died badly, and SDL's own messages are captured into the same
// stream. "Send me alyssapan.log" is the whole bug-report instruction.
#pragma once

#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

namespace alyssapan::log
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
    Off, // a threshold only: no record is written at it
};

// Opens the file and installs the SDL hook. Records written earlier reach stdout only.
void open(const std::filesystem::path &file);

void write(Level level, std::string_view text);

// The threshold ([log] level, ALYSSAPAN_LOG_LEVEL): records below it reach neither the file nor stdout.
// Debug, the default, keeps everything as before. Debug records reach stdout only with ALYSSAPAN_LOG_DEBUG
// set, whatever the threshold, because the tools read stdout and want it short. The banner, the failure
// hooks' records and the crash report land whatever the threshold: a log filtered down to errors is still
// the log somebody reads after a crash.
void setLevel(Level threshold);
Level level();

// The failure hooks (RosePan's assert hook; CT3 has no assert of its own): an SDL_assert that fails, an
// exception nobody caught (std::terminate) and abort() -- where the C runtime's assert ends -- each leave
// an [E] record in the file before the process goes. Installed once, after open().
void installFailureHooks();

// One line from a fault handler (crash_report.cpp): to the file only (the caller prints stdout itself),
// and never waiting for the lock -- the faulting thread may hold it, and then the line is lost rather
// than the process hung.
void writeFromFault(std::string_view text);

template <class... Args>
void debug(std::format_string<Args...> format, Args &&...args)
{
    write(Level::Debug, std::format(format, std::forward<Args>(args)...));
}

template <class... Args>
void info(std::format_string<Args...> format, Args &&...args)
{
    write(Level::Info, std::format(format, std::forward<Args>(args)...));
}

template <class... Args>
void warning(std::format_string<Args...> format, Args &&...args)
{
    write(Level::Warning, std::format(format, std::forward<Args>(args)...));
}

template <class... Args>
void error(std::format_string<Args...> format, Args &&...args)
{
    write(Level::Error, std::format(format, std::forward<Args>(args)...));
}

} // namespace alyssapan::log
