#include "vexularg.h"

/* Reset.
 * There is just one scene to the game.
 * We never do the usual drop and load sprites dance eg.
 * So this is more "reset game" than "enter a new map".
 */
 
int scene_reset() {

  //TODO Drop sprites.
  
  //TODO Run thru map poi eg for sprites.
  
  //TODO Restart music.
  
  return 0;
}

/* Update.
 */
 
void scene_update(double elapsed) {
  //TODO
}

/* Render.
 */
 
void scene_render() {

  /* Determine camera position.
   * Center on the hero, and clamp to the world.
   * World is guaranteed to be at least the size of the framebuffer.
   */
  double focusx,focusy;
  focusx=g.mapw*0.5;//TODO Hero position. And maybe other adjustments?
  focusy=g.maph*0.5;
  int worldw=g.mapw*NS_sys_tilesize;
  int worldh=g.maph*NS_sys_tilesize;
  int camerax=(int)(focusx*NS_sys_tilesize)-(FBW>>1);
  int cameray=(int)(focusy*NS_sys_tilesize)-(FBH>>1);
  if (camerax<0) camerax=0;
  else if (camerax>worldw-FBW) camerax=worldw-FBW;
  if (cameray<0) cameray=0;
  else if (cameray>worldh-FBH) cameray=worldh-FBH;

  //TODO I'd like to make some terrain tiles transparent and draw animated parallax layers behind. Decorative, not urgent.
  // For now, we may assume that the terrain tiles are opaque.
  
  int cola=camerax/NS_sys_tilesize;
  int colz=(camerax+FBW-1)/NS_sys_tilesize;
  int rowa=cameray/NS_sys_tilesize;
  int rowz=(cameray+FBH-1)/NS_sys_tilesize;
  if (cola<0) cola=0;
  if (colz>=g.mapw) colz=g.mapw-1;
  if (rowa<0) rowa=0;
  if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    graf_set_input(&g.graf,g.texid_terrain);
    int dsty=rowa*NS_sys_tilesize+(NS_sys_tilesize>>1)-cameray;
    int dstx0=cola*NS_sys_tilesize+(NS_sys_tilesize>>1)-camerax;
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
  
  //TODO Sprites.
  
  //TODO Overlay.
}
