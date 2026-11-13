#include "alyssapan/core/paths.h"

#include <array>
#include <cstdlib>
#include <system_error>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include "alyssapan/core/logger.h"

namespace alyssapan
{
namespace fs = std::filesystem;

fs::path pathFromUtf8(std::string_view text)
{
    return fs::path(std::u8string_view(reinterpret_cast<const char8_t *>(text.data()), text.size()));
}

std::string pathToUtf8(const fs::path &path)
{
    const std::u8string text = path.generic_u8string();
    return std::string(text.begin(), text.end());
}

Paths::Paths()
{
    if (const char *base = SDL_GetBasePath())
        m_base = pathFromUtf8(base);

    // A scripted run (build.py --run, tests) keeps its settings, log and cards apart from the player's,
    // so a smoke run's window size, position or graphics choices never land in the player's ini
    // (ReiPan's REIPAN_USER_DIR). Empty counts as unset, so `--env ALYSSAPAN_USER_DIR=` gives the
    // player's directory back.
    if (const char *const override = std::getenv("ALYSSAPAN_USER_DIR"); override != nullptr && *override)
    {
        m_user = pathFromUtf8(override);
        m_userOverridden = true;
    }
    else if (char *const user = SDL_GetPrefPath("Mikompilation", "AlyssaPan"))
    {
        m_user = pathFromUtf8(user);
        SDL_free(user);
    }
    else
    {
        m_user = m_base;
    }

    std::error_code ignored;
    fs::create_directories(memoryCardDirectory(), ignored);
}

fs::path Paths::resource(std::string_view relative) const
{
    return m_base / pathFromUtf8(relative);
}

bool Paths::isDataFolder(const fs::path &folder)
{
    std::error_code ignored;
    return fs::exists(folder / "DATA" / "CT3PACK.DAT", ignored) && fs::exists(folder / "IOP", ignored);
}

bool Paths::setDataFolder(const fs::path &folder)
{
    if (!isDataFolder(folder))
        return false;
    m_data = folder;
    log::info("Data folder: {}", pathToUtf8(m_data));
    return true;
}

bool Paths::discoverDataFolder(std::string_view configured)
{
    if (const char *const environment = std::getenv("ALYSSAPAN_DATA_DIR");
        environment != nullptr && setDataFolder(pathFromUtf8(environment)))
    {
        return true;
    }
    if (!configured.empty() && setDataFolder(pathFromUtf8(configured)))
        return true;

    std::error_code ignored;
    const fs::path working = fs::current_path(ignored);
    for (const fs::path &candidate : std::array{m_base, m_base / "data", working, working / "data"})
    {
        if (setDataFolder(candidate))
            return true;
    }

    log::warning("No data folder: looked for DATA\\CT3PACK.DAT and IOP\\");
    return false;
}

} // namespace alyssapan
