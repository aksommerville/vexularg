#include "vexularg.h"

/* Begin.
 */
 
void gameover_begin() {
  g.hello_running=0;
  g.gameover_running=1;
  //TODO song
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    hello_begin();
  }
}

/* Render.
 */
 
void gameover_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x800000ff);
}
