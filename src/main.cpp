// The process entry. The game's own main is gameMain (game/main.c), run on the EE's main thread.
#include <span>

#include <SDL3/SDL_main.h>

#include "alyssapan/core/application.h"

int main(int argc, char **argv)
{
    alyssapan::Application application(std::span(argv, static_cast<std::size_t>(argc)));
    return application.run();
}
