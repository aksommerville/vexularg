#include "game/vexularg.h"

#define FRAME_TIME 0.080

struct sprite_dust {
  struct sprite hdr;
  uint8_t tileid0;
  double animclock;
  int animframe; // At 4, we go defunct.
};

#define SPRITE ((struct sprite_dust*)sprite)

static int _dust_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->animclock=FRAME_TIME;
  return 0;
}

static void _dust_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=FRAME_TIME;
    if (++(SPRITE->animframe)>=4) sprite->defunct=1;
  }
}

static void _dust_render(struct sprite *sprite,int dstx,int dsty) {
  // Our nominal position is the hero's, where she landed. We are two tiles on either side of that.
  uint8_t tileid=SPRITE->tileid0;
  if (SPRITE->animframe>=4) tileid+=4;
  else tileid+=SPRITE->animframe;
  graf_tile(&g.graf,dstx-NS_sys_tilesize,dsty,tileid,0);
  graf_tile(&g.graf,dstx+NS_sys_tilesize,dsty,tileid,EGG_XFORM_XREV);
}

const struct sprite_type sprite_type_dust={
  .name="dust",
  .objlen=sizeof(struct sprite_dust),
  .init=_dust_init,
  .update=_dust_update,
  .render=_dust_render,
};
