// alyssapan/core/movie_skip.h -- a seam game code includes (cpp-port.md "Scope"): whether the movie player
// ends each movie at once, as a skipped one, without decoding it. The switch is [game] skip_intro in
// alyssapan.ini (ALYSSAPAN_SKIP_INTRO=1 in the environment), off by default; smoke runs turn it on
// (build.py --run, tus.py check --run), RosePan's rule: skip cinematics in smoke runs.
#pragma once

// Non-zero: skip every movie (movie.c's AA_movie takes its PORT branch).
int AlyssaPan_SkipMovies(void);
