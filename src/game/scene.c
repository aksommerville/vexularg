#include "vexularg.h"

/* Reset.
 * There is just one scene to the game.
 * We never do the usual drop and load sprites dance eg.
 * So this is more "reset game" than "enter a new map".
 */
 
int scene_reset() {

  // Drop sprites.
  g.hero=0;
  while (g.spritec>0) {
    struct sprite *sprite=g.spritev[--g.spritec];
    sprite_del_unlisted(sprite);
  }
  
  /* Run map commands.
   */
  struct cmdlist_reader reader={.v=g.mapcmd,.c=g.mapcmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_sprite: {
          double x=cmd.arg[0]+0.5;
          double y=cmd.arg[1]+0.5;
          int rid=(cmd.arg[2]<<8)|cmd.arg[3];
          uint32_t arg=(cmd.arg[4]<<24)|(cmd.arg[5]<<16)|(cmd.arg[6]<<8)|cmd.arg[7];
          struct sprite *sprite=sprite_spawn(0,x,y,arg,rid,0,0);
        } break;
    }
  }
  
  // Restart music.
  song(RID_song_unto_thee,1);
  
  g.camera_cut=1;
  g.time_remaining=PLAY_TIME;
  g.accelerated_time=0.0;
  g.hello_running=0;
  g.gameover_running=0;
  g.earthquake_time=0.0;
  
  //XXX During dev, enable this block to end the session fast with the given things moved to the altar.
  if (0) {
    //const int collected[]={};
    //const int collected[]={RID_sprite_fish}; // First invalid case.
    //const int collected[]={RID_sprite_marshmallow}; // Second invalid case.
    //const int collected[]={RID_sprite_marshmallow,RID_sprite_fish}; // Minimum required things.
    const int collected[]={RID_sprite_marshmallow,RID_sprite_fish,RID_sprite_teacup,RID_sprite_tomato,RID_sprite_balloon,RID_sprite_trampoline,RID_sprite_fan,RID_sprite_magnet}; // All the things.
    int collectedc=sizeof(collected)/sizeof(int);
    fprintf(stderr,"***** instant end, %d things requested *****\n",collectedc);
    struct sprite **spritep=g.spritev;
    int i=g.spritec;
    for (;i-->0;spritep++) {
      struct sprite *sprite=*spritep;
      int iscollected=0;
      const int *p=collected;
      int ii=collectedc;
      for (;ii-->0;p++) {
        if (*p==sprite->rid) {
          iscollected=1;
          break;
        }
      }
      if (!iscollected) continue;
      sprite->x=g.mapw*0.5;
      sprite->y=g.maph*0.5;
    }
    g.time_remaining=3.0;
  }
  
  return 0;
}

/* Update.
 */
 
void scene_update(double elapsed) {

  if (g.earthquake_time>0.0) {
    g.earthquake_time-=elapsed;
  }
  double adjusted_elapsed=elapsed;
  if (g.all_things_in_offeratorium&&!g.gameover_running) {
    adjusted_elapsed*=20.0;
    if (!g.gameover_running) g.accelerated_time+=adjusted_elapsed;
  }
  g.all_things_in_offeratorium=1;

  /* Update all sprites that can.
   */
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct) continue;
    if (!sprite->type->update) continue;
    if (sprite->type==&sprite_type_hero) sprite->type->update(sprite,elapsed);
    else sprite->type->update(sprite,adjusted_elapsed);
  }
  
  /* Drop defunct sprites.
   */
  for (i=g.spritec;i-->0;) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct) {
      g.spritec--;
      memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
      sprite_del_unlisted(sprite);
      if (g.hero==sprite) g.hero=0;
    } else if (sprite->unlist_soon) {
      sprite->unlist_soon=0;
      g.spritec--;
      memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
      if (g.hero==sprite) g.hero=0;
    }
  }
  
  /* Tick the master clock and end the game when it expires.
   */
  if (!g.gameover_running) {
    if ((g.time_remaining-=adjusted_elapsed)<=0.0) {
      gameover_begin();
    }
  }
}

/* Two passes of a cocktail sort on the sprite list, to bring them closer to render order.
 * It's fine to be out of order for a little while, and we'll update this every frame.
 */
 
static int sprite_rendercmp(const struct sprite *a,const struct sprite *b) {
  return a->layer-b->layer;
}
 
static void sort_sprites_partial() {
  if (g.spritec<2) return;
  int done=1,i=1;
  for (;i<g.spritec;i++) {
    struct sprite *a=g.spritev[i-1];
    struct sprite *b=g.spritev[i];
    if (sprite_rendercmp(a,b)>0) {
      g.spritev[i-1]=b;
      g.spritev[i]=a;
      done=0;
    }
  }
  if (done) return;
  for (i=g.spritec-1;i-->0;) {
    struct sprite *a=g.spritev[i];
    struct sprite *b=g.spritev[i+1];
    if (sprite_rendercmp(a,b)>0) {
      g.spritev[i]=b;
      g.spritev[i+1]=a;
    }
  }
}

/* Draw a field of stars, independent of scroll.
 */

static void stars_init() {
  uint32_t *rgba=calloc(FBW<<2,FBH);
  if (!rgba) return;
  
  uint32_t starcolor=g.enable_color?0xffc06030:0xffffffff;
  
  int starc=200; // Will end up placing fewer than this, due to random collisions.
  while (starc-->0) {
    // Keep them off the edge, to simplify the neighbor check.
    int x=1+rand()%(FBW-2);
    int y=1+rand()%(FBH-2);
    // If we have a neighbor star, cardinal or diagonal, skip it.
    uint32_t *p=rgba+y*FBW+x;
    if (p[-FBW-1]||p[-FBW]||p[-FBW+1]||p[-1]||p[0]||p[1]||p[FBW-1]||p[FBW]||p[FBW+1]) continue;
    *p=starcolor;
  }
  
  // Replace zeroes with opaque black.
  if (g.enable_color) {
    uint32_t *p=rgba;
    int i=FBW*FBH;
    for (;i-->0;p++) if (!*p) *p=0xff402010;
  } else {
    uint8_t *p=((uint8_t*)rgba)+3;
    int i=FBW*FBH;
    for (;i-->0;p+=4) if (!*p) *p=0xff;
  }
  
  // Upload pixels.
  g.texid_stars=egg_texture_new();
  egg_texture_load_raw(g.texid_stars,FBW,FBH,FBW<<2,rgba,FBW*FBH*4);
  free(rgba);
}
 
static void draw_stars() {

  // First update, draw the random star field.
  if (!g.texid_stars) {
    stars_init();
  }
  
  // After that, it's just a static image to copy.
  graf_set_input(&g.graf,g.texid_stars);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
}

/* Maintain and draw a bunch of decorative snowflakes.
 */
 
#define SNOWFLAKE_BLINK_PERIOD 4
#define SNOWFLAKE_BLINK_DUTY 3
 
static double sinev[256];
 
static struct snowflake {
  double x,y;
  double dy;
  double rx;
  double x0;
  int p;
  int blinkp;
} snowflakev[256];

static int snowflakes_init=0;

static void snowflake_init(struct snowflake *snowflake) {
  int sinec=sizeof(sinev)/sizeof(double);
  snowflake->dy=0.100+((0.200*(rand()&0x7fff))/32768.0);
  snowflake->rx=2.0+((4.0*(rand()&0x7fff))/32768.0);
  snowflake->p=rand()%sinec;
  snowflake->blinkp=rand()%SNOWFLAKE_BLINK_PERIOD;
}
 
static void draw_snow() {

  // Position in the snowfield, driven by camera.
  int parx=g.camerax>>1;
  int pary=g.cameray>>1;

  // First update, calculate the sine table and initialize a set of snowflakes.
  if (!snowflakes_init) {
    snowflakes_init=1;
    int sinec=sizeof(sinev)/sizeof(double);
    int i=0;
    double t=0.0;
    double dt=(M_PI*2.0)/sinec;
    for (;i<sinec;i++,t+=dt) sinev[i]=sin(t);
    struct snowflake *snowflake=snowflakev;
    int snowflakec=sizeof(snowflakev)/sizeof(struct snowflake);
    for (i=snowflakec;i-->0;snowflake++) {
      snowflake_init(snowflake);
      snowflake->x=parx+rand()%FBW;
      snowflake->y=pary+rand()%FBH;
    }
  }
  
  graf_set_input(&g.graf,g.texid_terrain);
  int sinec=sizeof(sinev)/sizeof(double);
  struct snowflake *snowflake=snowflakev;
  int i=sizeof(snowflakev)/sizeof(struct snowflake);
  for (;i-->0;snowflake++) {
    if (++(snowflake->blinkp)>=SNOWFLAKE_BLINK_PERIOD) snowflake->blinkp=0;
    snowflake->y+=snowflake->dy;
    snowflake->p++;
    if (snowflake->p>=sinec) snowflake->p=0;
    if (snowflake->blinkp<SNOWFLAKE_BLINK_DUTY) {
      double x=snowflake->x+snowflake->rx*sinev[snowflake->p];
      int ix=(int)x-parx;
      int iy=(int)snowflake->y-pary;
      // If we're outside the visible range on either axis, reinit and move to the other edge.
      if ((ix<0)||(ix>=FBW)) {
        snowflake_init(snowflake);
        if (ix<0) snowflake->x=parx+FBW-1;
        else snowflake->x=parx;
        ix=(int)(snowflake->x+snowflake->rx*sinev[snowflake->p])-parx;
      }
      if ((iy<0)||(iy>=FBH)) {
        snowflake_init(snowflake);
        if (iy<0) snowflake->y=pary+FBH-1;
        else snowflake->y=pary;
        iy=(int)snowflake->y-pary;
      }
      graf_tile(&g.graf,ix,iy,0x07,0);
    }
  }
}

/* Render.
 */
 
void scene_render() {

  /* Determine camera position.
   * Center on the hero, and clamp to the world.
   * World is guaranteed to be at least the size of the framebuffer.
   */
  if (g.hero) {
    double focusx,focusy;
    focusx=g.hero->x;
    focusy=g.hero->y;
    int fqx=(int)focusx,fqy=(int)focusy,locked=0;
    const struct camlock *camlock=g.camlockv;
    int i=g.camlockc;
    for (;i-->0;camlock++) {
      if (fqx<camlock->x) continue;
      if (fqy<camlock->y) continue;
      if (fqx>=camlock->x+camlock->w) continue;
      if (fqy>=camlock->y+camlock->h) continue;
      focusx=(double)camlock->x+(double)camlock->w*0.5;
      focusy=(double)camlock->y+(double)camlock->h*0.5;
      locked=1;
      break;
    }
    int worldw=g.mapw*NS_sys_tilesize;
    int worldh=g.maph*NS_sys_tilesize;
    int camerax=(int)(focusx*NS_sys_tilesize)-(FBW>>1);
    int cameray=(int)(focusy*NS_sys_tilesize)-(FBH>>1);
    if (camerax<0) camerax=0;
    else if (camerax>worldw-FBW) camerax=worldw-FBW;
    if (cameray<0) cameray=0;
    else if (cameray>worldh-FBH) cameray=worldh-FBH;
    if (g.earthquake_time>0.0) cameray--; // Earthquakes allow camera to leave the map.
    if (g.camera_cut) { // Cut? Jump immediately to the new target.
      g.camera_cut=0;
      g.camerax=camerax;
      g.cameray=cameray;
    } else { // No cut? step by no more than some limit.
      const int maxstep=3; // 3 stays synced always. 2 is pretty good, but can fall behind at high gravity.
      if (g.camerax<camerax) {
        if ((g.camerax+=maxstep)>camerax) g.camerax=camerax;
      } else if (g.camerax>camerax) {
        if ((g.camerax-=maxstep)<camerax) g.camerax=camerax;
      }
      if (g.cameray<cameray) {
        if ((g.cameray+=maxstep)>cameray) g.cameray=cameray;
      } else if (g.cameray>cameray) {
        if ((g.cameray-=maxstep)<cameray) g.cameray=cameray;
      }
    }
  }

  /* Draw the far parallax background.
   */
  //graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff); // No need to blackout; the stars image is opaque.
  draw_stars();
  draw_snow();
  
  /* Fill framebuffer with the grid.
   */
  int cola=g.camerax/NS_sys_tilesize;
  int colz=(g.camerax+FBW-1)/NS_sys_tilesize;
  int rowa=g.cameray/NS_sys_tilesize;
  int rowz=(g.cameray+FBH-1)/NS_sys_tilesize;
  if (cola<0) cola=0;
  if (colz>=g.mapw) colz=g.mapw-1;
  if (rowa<0) rowa=0;
  if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    graf_set_input(&g.graf,g.texid_terrain);
    int dsty=rowa*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.cameray;
    int dstx0=cola*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.camerax;
    const uint8_t *cellrow=g.cellv+rowa*g.mapw+cola;
    int row=rowa;
    for (;row<=rowz;row++,cellrow+=g.mapw,dsty+=NS_sys_tilesize) {
      const uint8_t *cellp=cellrow;
      int col=cola;
      int dstx=dstx0;
      for (;col<=colz;col++,cellp++,dstx+=NS_sys_tilesize) {
        graf_tile(&g.graf,dstx,dsty,*cellp,0);
      }
    }
  }
  
  /* Sprites.
   * We're doing a large single map with one set of sprites distributed about it.
   * It's worth a little effort to cull sprite well offscreen.
   * Beware that some sprites cast a wide shadow, particularly Moon Song.
   */
  sort_sprites_partial();
  double viewl=(double)(g.camerax-NS_sys_tilesize*6)/(double)NS_sys_tilesize;
  double viewt=(double)(g.cameray-NS_sys_tilesize*6)/(double)NS_sys_tilesize;
  double viewr=(double)(g.camerax+FBW+NS_sys_tilesize*6)/(double)NS_sys_tilesize;
  double viewb=(double)(g.cameray+FBH+NS_sys_tilesize*6)/(double)NS_sys_tilesize;
  graf_set_input(&g.graf,g.texid_sprites);
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->x<viewl) continue;
    if (sprite->x>viewr) continue;
    if (sprite->y<viewt) continue;
    if (sprite->y>viewb) continue;
    int dstx=(int)(sprite->x*NS_sys_tilesize)-g.camerax;
    int dsty=(int)(sprite->y*NS_sys_tilesize)-g.cameray;
    if (sprite->type->render) {
      sprite->type->render(sprite,dstx,dsty);
    } else {
      graf_tile(&g.graf,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
  
  // Clock only before gameover. We do continue running during gameover.
  if (!g.gameover_running) {
  
    /* Time remaining, upper-right corner.
     * These use the sprites texture, which is already active.
     */
    int ms=(int)(g.time_remaining*1000.0);
    if (ms<0) ms=0;
    int sec=ms/1000; ms%=1000;
    int min=sec/60; sec%=60;
    if (min>9) { min=9; sec=99; ms=999; }
    graf_tile(&g.graf,FBW-3,4,0x70+sec%10,0);
    graf_tile(&g.graf,FBW-7,4,0x70+sec/10,0);
    if (!ms||(ms>=200)) graf_tile(&g.graf,FBW-10,4,0x7a,0);
    else if (g.enable_color) graf_tile(&g.graf,FBW-10,4,0x68,0);
    graf_tile(&g.graf,FBW-13,4,0x70+min,0);
  
    /* Extra blinking "Hurry!" message when time is short.
     * Ensure its phase is such that nothing renders at ms==0.
     */
    if (!min&&(sec<=20)&&(ms>=300)) {
      graf_tile(&g.graf,FBW-21,4,0x7e,0);
      graf_tile(&g.graf,FBW-29,4,0x7d,0);
      graf_tile(&g.graf,FBW-37,4,0x7c,0);
      graf_tile(&g.graf,FBW-45,4,0x7b,0);
    }
  }
}
