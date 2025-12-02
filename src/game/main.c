#include "vexularg.h"

struct g g={0};

void egg_client_quit(int status) {
}

/* Init.
 */

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  if (res_init()<0) return -1;
  if (!(g.font=font_new())) return -1;
  if (font_add_image(g.font,RID_image_font,0x0020)) return -1;
  if (egg_texture_load_image(g.texid_terrain=egg_texture_new(),RID_image_terrain)<0) return -1;
  if (egg_texture_load_image(g.texid_sprites=egg_texture_new(),RID_image_sprites)<0) return -1;

  srand_auto();

  //TODO Hello modal.
  if (scene_reset()<0) return -1;

  egg_play_song(1,RID_song_unto_thee,1,0.5,0.0);//XXX TEMP

  return 0;
}

/* Notify.
 */

void egg_client_notify(int k,int v) {
}

/* Update.
 */

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  
  //TODO Modals.
  scene_update(elapsed);
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  //TODO Modals.
  scene_render();
  graf_flush(&g.graf);
}
