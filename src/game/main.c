#include "vexularg.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image(g.font,RID_image_font,0x0020)) return -1;
  
  g.msg.texid=font_render_to_texture(0,g.font,
    "Let's summon the dread ghoul Vexularg!\n"
    "Summoning is fun!\n"
    "We're going to have a fabulous time collecting all manner of things to offer to Vexularg.\n"
  ,-1,FBW,FBH,0xffffffff);
  egg_texture_get_size(&g.msg.w,&g.msg.h,g.msg.texid);

  srand_auto();

  //TODO

  return 0;
}

void egg_client_notify(int k,int v) {
}

void egg_client_update(double elapsed) {
  //TODO
}

void egg_client_render() {
  graf_reset(&g.graf);
  graf_set_input(&g.graf,g.msg.texid);
  graf_decal(&g.graf,(FBW>>1)-(g.msg.w>>1),(FBH>>1)-(g.msg.h>>1),0,0,g.msg.w,g.msg.h);
  //TODO
  graf_flush(&g.graf);
}
