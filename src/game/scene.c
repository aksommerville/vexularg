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
      //TODO Other map initialization commands.
    }
  }
  
  // Restart music.
  song(RID_song_unto_thee);
  
  g.camera_cut=1;
  
  return 0;
}

/* Update.
 */
 
void scene_update(double elapsed) {

  /* Update all sprites that can.
   */
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->defunct) continue;
    if (!sprite->type->update) continue;
    sprite->type->update(sprite,elapsed);
  }
  
  //TODO Terminal conditions.
  
  /* Drop defunct sprites.
   */
  for (i=g.spritec;i-->0;) {
    struct sprite *sprite=g.spritev[i];
    if (!sprite->defunct) continue;
    g.spritec--;
    memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
    sprite_del_unlisted(sprite);
    if (g.hero==sprite) g.hero=0;
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

  //TODO I'd like to make some terrain tiles transparent and draw animated parallax layers behind. Decorative, not urgent.
  // For now, we may assume that the terrain tiles are opaque.
  
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
  
  //TODO Overlay.
}
