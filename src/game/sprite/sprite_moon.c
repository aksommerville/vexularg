#include "game/vexularg.h"

// Outer dimensions of the word bubble in tiles. Width should be odd. Doesn't count the stem.
#define DLOG_COLC 7
#define DLOG_ROWC 3

// Inner dimensions of the word bubble in pixels.
#define DLOG_MARGIN_X 2
#define DLOG_MARGIN_Y 2
#define DLOG_INNER_W (DLOG_COLC*NS_sys_tilesize-DLOG_MARGIN_X*2)
#define DLOG_INNER_H (DLOG_ROWC*NS_sys_tilesize-DLOG_MARGIN_Y*2)

#define SCROLL_RATE 4.0 /* px/s */

// At SCROLL_RATE=4 and DLOG_COLC=7, this text is just within the 2048-pixel limit, and takes about 102 seconds to play out.
// That's the low end that I might want for global time. We can tweak SCROLL_RATE to suit. 2.0 is ok, so is 6.0, and fractions are no problem.
static const char spell_of_summoning[]=
  "Zon yoom ollix. Intex fannel wreng knogh. Lux pefta gar o scunnia mewdle. "
  "O aru ferrinum lasus nargoplex umbria hal yoom pax. Moop hex illius. "
  "Twint tallia nan octrum aru o intex. Twint yoom wreng yallimus gar. Braphus hal quenna. "
  "Jep ferrinum hal wumple leffimer tallia pefta torph yoom nargoplex geppina hoccet biliar. "
  "Cep wook wumple relifi knogh gar umbria ex ollix. Quish rit umbria nan lux. ONE "
  "Ept romma durgh wumple lux. Tem ir gelft relifi yit bem aps kipple tallia xa umbria. "
  "Durgh crym wreng moop romma si pax relifi bem. Illius twint marph ferrinum. "
  "Flam kwn xeff mu nan quenna si urqualle gelft braphus octrum. "
  "Hal pax sluph geppina yallimus yoom de. Que lenta octrum wumple elliani."
"";

struct sprite_moon {
  struct sprite hdr;
  double animclock;
  int animframe;
  int texid,texw,texh;
  double scroll; // px
  double startclock; // Counts down before chanting.
};

#define SPRITE ((struct sprite_moon*)sprite)

/* Delete.
 */
 
static void _moon_del(struct sprite *sprite) {
  egg_texture_del(SPRITE->texid);
}

/* Init.
 */
 
static int _moon_init(struct sprite *sprite) {
  SPRITE->texid=font_render_to_texture(0,g.font,spell_of_summoning,sizeof(spell_of_summoning)-1,DLOG_INNER_W,2048,0x000000ff);
  egg_texture_get_size(&SPRITE->texw,&SPRITE->texh,SPRITE->texid);
  SPRITE->scroll=-DLOG_INNER_H;
  SPRITE->startclock=5.0;
  return 0;
}

/* Update.
 */
 
static void _moon_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->startclock-=elapsed)>0.0) return;
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.400;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
  }
  SPRITE->scroll+=elapsed*SCROLL_RATE;
}

/* Render.
 */
 
static void _moon_render(struct sprite *sprite,int dstx,int dsty) {

  // Moon just stands here and chants.
  int chanting;
  if (SPRITE->startclock>0.0) chanting=0;
  else if (SPRITE->scroll<SPRITE->texh) chanting=1;
  else chanting=0;
  uint8_t tileid=sprite->tileid;
  if (chanting) {
    switch (SPRITE->animframe) {
      case 0: tileid+=5; break;
      case 1: tileid+=6; break;
    }
  }
  graf_tile(&g.graf,dstx,dsty-NS_sys_tilesize,tileid,sprite->xform);
  graf_tile(&g.graf,dstx,dsty,tileid+0x10,sprite->xform);
  if (!chanting) return;
  
  // The word bubble above her is a bit more interesting.
  int dlogx0=dstx-(DLOG_COLC>>1)*NS_sys_tilesize;
  int dlogy0=dsty-(DLOG_ROWC+2)*NS_sys_tilesize;
  #define DLOGTILE(cx,cy,tid,addy) graf_tile(&g.graf,dlogx0+(cx)*NS_sys_tilesize,dlogy0+(cy)*NS_sys_tilesize+(addy),tid,0);
  DLOGTILE(0,0,0x40,0)
  DLOGTILE(DLOG_COLC-1,0,0x42,0)
  DLOGTILE(0,DLOG_ROWC-1,0x60,0)
  DLOGTILE(DLOG_COLC-1,DLOG_ROWC-1,0x62,0)
  int x,y;
  for (x=DLOG_COLC-1;x-->1;) {
    DLOGTILE(x,0,0x41,0)
    DLOGTILE(x,DLOG_ROWC-1,0x61,0)
    for (y=DLOG_ROWC-1;y-->1;) {
      DLOGTILE(x,y,0x51,0)
    }
  }
  for (y=DLOG_ROWC-1;y-->1;) {
    DLOGTILE(0,y,0x50,0)
    DLOGTILE(DLOG_COLC-1,y,0x52,0)
  }
  DLOGTILE(DLOG_COLC>>1,DLOG_ROWC,0x43,-1)
  #undef DLOGTILE
  
  int ddstx=dlogx0-(NS_sys_tilesize>>1)+DLOG_MARGIN_X;
  int ddsty=dlogy0-(NS_sys_tilesize>>1)+DLOG_MARGIN_Y;
  int dsrcy=(int)SPRITE->scroll;
  int ddsth=DLOG_INNER_H;
  if (dsrcy<0) {
    ddsth+=dsrcy;
    ddsty-=dsrcy;
    dsrcy=0;
  }
  if (ddsth>0) {
    graf_set_input(&g.graf,SPRITE->texid);
    graf_decal(&g.graf,ddstx,ddsty,0,dsrcy,SPRITE->texw,ddsth); // NB texw, not DLOG_INNER_W, the texture might be short.
    graf_set_input(&g.graf,g.texid_sprites);
  }
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
