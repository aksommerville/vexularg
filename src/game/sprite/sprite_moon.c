#include "game/vexularg.h"

struct sprite_moon {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_moon*)sprite)

/* Delete.
 */
 
static void _moon_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _moon_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _moon_update(struct sprite *sprite,double elapsed) {
}

/* Render.
 */
 
static void _moon_render(struct sprite *sprite,int dstx,int dsty) {
  graf_tile(&g.graf,dstx,dsty,sprite->tileid,sprite->xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_moon={
  .name="moon",
  .objlen=sizeof(struct sprite_moon),
  .del=_moon_del,
  .init=_moon_init,
  .update=_moon_update,
  .render=_moon_render,
};
