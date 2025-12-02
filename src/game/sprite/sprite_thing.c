#include "game/vexularg.h"

struct sprite_thing {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_thing*)sprite)

/* Delete.
 */
 
static void _thing_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _thing_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _thing_update(struct sprite *sprite,double elapsed) {
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_thing={
  .name="thing",
  .objlen=sizeof(struct sprite_thing),
  .del=_thing_del,
  .init=_thing_init,
  .update=_thing_update,
};
