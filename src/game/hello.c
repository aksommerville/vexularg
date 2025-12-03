#include "vexularg.h"

/* Begin.
 */
 
void hello_begin() {
  g.hello_running=1;
  g.gameover_running=0;
  //TODO song
}

/* Update.
 */
 
void hello_update(double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    scene_reset();
  }
}

/* Render.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x008000ff);
}
