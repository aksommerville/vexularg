#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"

#define FBW 160
#define FBH 88

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  struct { int texid,w,h; } msg;//XXX
} g;

#endif
