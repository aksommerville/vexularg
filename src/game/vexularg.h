#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "sprite/sprite.h"

#define FBW 160
#define FBH 88

#define SOUND_BLACKOUT_LIMIT 32
#define CAMLOCK_LIMIT 4 /* There will probably be just one. */

extern struct g {

  void *rom;
  int romc;
  struct rom_entry *resv;
  int resc,resa;
  
  struct graf graf;
  struct font *font;
  int texid_terrain;
  int texid_sprites;
  
  int input,pvinput;
  
  //TODO Bespoke modals.
  
  const uint8_t *cellv;
  int mapw,maph; // Dimensions are not fixed at build time, tho they could be. Treat them as dynamic.
  const uint8_t *mapcmd;
  int mapcmdc;
  uint8_t physics[256]; // Single tilesheet, we load it just once.
  int camerax,cameray;
  int camera_cut; // If nonzero, next camera update will go immediately to its target.
  struct camlock { uint8_t x,y,w,h; } camlockv[CAMLOCK_LIMIT];
  int camlockc;
  double time_remaining;
  int game_over;//XXX TEMP
  
  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK, OPTIONAL. Maintained sneakily by generic sprite allocator.
  
  int song_playing;
  struct sound_blackout {
    int rid;
    double time;
  } sound_blackoutv[SOUND_BLACKOUT_LIMIT];
  int sound_blackoutc;
} g;

void sfx_spatial(int rid,double x,double y); // (x,y) in world meters
void sfx_full(int rid);
void song(int rid);

int res_init();
int res_search(int tid,int rid);
int res_get(void *dstpp,int tid,int rid); // Only certain resource types are toc'd.
int res_get_string(void *dstpp,int rid,int strix);

/* Scene has its logic separated from the rest, but its data is all directly in (g).
 * Responsible for the game world.
 */
int scene_reset();
void scene_update(double elapsed);
void scene_render(); // Overwrites all.

#endif
