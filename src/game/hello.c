#include "vexularg.h"

#define DOTX ((FBW*2)/5)
#define DOTY (FBH-10)
#define MOONX ((FBW*3)/5)
#define MOONY (FBH-10)

#define SCENE_PERIOD 20.0

/* Begin.
 */
 
void hello_begin() {
  g.hello_running=1;
  g.gameover_running=0;
  g.modal_blackout=0.500;
  g.hello_clock=0.0;
  g.hdlog.w=g.hdlog.h=0;
  
  if (!g.texid_title) {
    g.texid_title=egg_texture_new();
    egg_texture_load_image(g.texid_title,RID_image_title);
    egg_texture_get_size(&g.titlew,&g.titleh,g.texid_title);
  }
  
  song(RID_song_wrath_of_vexularg,1);
}

/* Replace word bubble.
 */
 
static void hello_dialogue_none() {
  g.hdlog.w=0;
  g.hdlog.h=0;
}
 
static void hello_dialogue(int x,int strix) {
  if (strix<1) {
    hello_dialogue_none();
    return;
  }
  const char *text;
  int textc=res_get_string(&text,1,strix);
  if (textc<1) {
    hello_dialogue_none();
    return;
  }
  
  /* Render the text alone into a scratch texture.
   * We can't know the final bounds until this is done.
   */
  int ttexid=font_render_to_texture(0,g.font,text,textc,FBW,FBH,0x000000ff);
  if (ttexid<1) {
    hello_dialogue_none();
    return;
  }
  int tw=0,th=0;
  egg_texture_get_size(&tw,&th,ttexid);
  if ((tw<1)||(th<1)) {
    egg_texture_del(ttexid);
    hello_dialogue_none();
    return;
  }
  
  /* Determine bounds for the framing, and allocate the final texture.
   * (tw,th) plus 4 for margins, rounded up to multiples of 8, plus 4 on the bottom for the stem.
   */
  int colc=(tw+11)/NS_sys_tilesize;
  int rowc=(th+11)/NS_sys_tilesize; // Not counting the stem.
  int w=colc*NS_sys_tilesize;
  int h=rowc*NS_sys_tilesize;
  h+=4; // For the stem.
  if (!g.hdlog.texid) {
    g.hdlog.texid=egg_texture_new();
  }
  if (egg_texture_load_raw(g.hdlog.texid,w,h,w<<2,0,0)<0) {
    egg_texture_del(ttexid);
    hello_dialogue_none();
    return;
  }
  
  /* Draw everything into the real texture.
   * Mind that we are not in a render cycle.
   */
  egg_texture_clear(g.hdlog.texid);
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.hdlog.texid);
  graf_set_input(&g.graf,g.texid_sprites);
  int dsty=NS_sys_tilesize>>1;
  int row=0;
  for (;row<rowc;row++,dsty+=NS_sys_tilesize) {
    int dstx=NS_sys_tilesize>>1;
    int col=0;
    for (;col<colc;col++,dstx+=NS_sys_tilesize) {
      uint8_t tileid=0x51;
      if (!row) tileid-=0x10; else if (row==rowc-1) tileid+=0x10;
      if (!col) tileid-=0x01; else if (col==colc-1) tileid+=0x01;
      graf_tile(&g.graf,dstx,dsty,tileid,0);
    }
  }
  graf_tile(&g.graf,w>>1,h-1,0x43,0);
  graf_set_input(&g.graf,ttexid);
  graf_decal(&g.graf,(w>>1)-(tw>>1),((h-4)>>1)-(th>>1),0,0,tw,th);
  graf_flush(&g.graf);
  
  // And finally, select output position and kill the scratch texture.
  g.hdlog.w=w;
  g.hdlog.h=h;
  g.hdlog.x=x-(g.hdlog.w>>1);
  g.hdlog.y=DOTY-(NS_sys_tilesize*2)-g.hdlog.h+3;
  egg_texture_del(ttexid);
}

/* Update.
 */
 
void hello_update(double elapsed) {
  double pvclock=g.hello_clock;
  g.hello_clock+=elapsed;
  if (g.hello_clock>=SCENE_PERIOD) {
    g.hello_clock=0.0;
  }
  
  if (g.modal_blackout>0.0) {
    g.modal_blackout-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      scene_reset();
    }
  }
  
  /* Begin dialogue at specific times.
   */
  #define AT(t) ((pvclock<(t))&&(g.hello_clock>=(t)))
       if (AT( 4.0)) hello_dialogue(DOTX,10); // "Let's summon a ghoul!"
  else if (AT( 8.0)) hello_dialogue(MOONX,11); // "I'll begin the incantation. You collect an offering."
  else if (AT(12.0)) hello_dialogue(0,0);
  #undef AT
}

/* Render.
 */
 
void hello_render() {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  
  graf_set_input(&g.graf,g.texid_title);
  graf_decal(&g.graf,(FBW>>1)-(g.titlew>>1),10,0,0,g.titlew,g.titleh);
  
  /* Draw both witches near the bottom.
   */
  graf_set_input(&g.graf,g.texid_sprites);
  graf_tile(&g.graf,DOTX,DOTY,0x10,0);
  graf_tile(&g.graf,DOTX,DOTY-NS_sys_tilesize,0x00,0);
  graf_tile(&g.graf,MOONX,MOONY,0x10,EGG_XFORM_XREV);
  graf_tile(&g.graf,MOONX,MOONY-NS_sys_tilesize,0x00,EGG_XFORM_XREV);
  
  // And a word bubble if we have one.
  if (g.hdlog.w>0) {
    graf_set_input(&g.graf,g.hdlog.texid);
    graf_decal(&g.graf,g.hdlog.x,g.hdlog.y,0,0,g.hdlog.w,g.hdlog.h);
  }
}
