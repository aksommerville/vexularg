#include "vexularg.h"

/* Begin.
 */
 
void hello_begin() {
  g.hello_running=1;
  g.gameover_running=0;
  g.modal_blackout=0.500;
  
  if (!g.texid_title) {
    g.texid_title=egg_texture_new();
    egg_texture_load_image(g.texid_title,RID_image_title);
    egg_texture_get_size(&g.titlew,&g.titleh,g.texid_title);
  }
  
  song(RID_song_wrath_of_vexularg,1);
}

/* Update.
 */
 
void hello_update(double elapsed) {
  if (g.modal_blackout>0.0) {
    g.modal_blackout-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      scene_reset();
    }
  }
}

/* Render.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  graf_set_input(&g.graf,g.texid_title);
  graf_decal(&g.graf,(FBW>>1)-(g.titlew>>1),10,0,0,g.titlew,g.titleh);
}
