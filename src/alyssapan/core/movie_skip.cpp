// The movie skip switch (movie_skip.h): the player's [game] skip_intro, read once the config is loaded.
#include "alyssapan/core/movie_skip.h"

#include "alyssapan/core/application.h"
#include "alyssapan/core/config.h"

int AlyssaPan_SkipMovies(void)
{
    return alyssapan::Application::instance().config().game.skipIntro ? 1 : 0;
}
