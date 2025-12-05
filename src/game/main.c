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
  hiscore_load();
  hello_begin();

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
  if (g.hello_running) {
    hello_update(elapsed);
  } else {
    scene_update(elapsed);
    if (g.gameover_running) gameover_update(elapsed);
  }
}

/* Render.
 */

void egg_client_render() {
  g.framec++;
  graf_reset(&g.graf);
  if (g.hello_running) {
    hello_render();
  } else {
    scene_render();
    if (g.gameover_running) gameover_render();
  }
  graf_flush(&g.graf);
}

/* Audio.
 */
 
static struct sound_blackout *sound_blackout_check(int rid) {
  const double BLACKOUT_TIME=0.050; // Suppress sounds within about 20 Hz of each other.
  double now=egg_time_real();
  
  // Reap expired records from the tail. Best effort only.
  double oldtime=now-BLACKOUT_TIME;
  while (g.sound_blackoutc&&(g.sound_blackoutv[g.sound_blackoutc-1].time<=oldtime)) g.sound_blackoutc--;
  
  struct sound_blackout *oldest=g.sound_blackoutv;
  struct sound_blackout *q=g.sound_blackoutv;
  int i=g.sound_blackoutc;
  for (;i-->0;q++) {
    if (q->rid==rid) {
      double age=now-q->time;
      if (age<BLACKOUT_TIME) return 0; // Already playing, don't start another.
      // Reuse this record for the new play.
      q->time=now;
      return q;
    }
    if (q->time<oldest->time) oldest=q;
  }
  if (g.sound_blackoutc<SOUND_BLACKOUT_LIMIT) {
    q=g.sound_blackoutv+g.sound_blackoutc++;
  } else {
    q=oldest;
  }
  q->time=now;
  q->rid=rid;
  return q;
}
 
void sfx_spatial(int rid,double x,double y) {
  
  // Normalize (x,y) around the camera's focus such that -1 and 1 are half a screen offscreen, those are the limits.
  double midx=(double)(g.camerax+(FBW>>1))/(double)NS_sys_tilesize;
  double rangex=(double)FBW/(double)NS_sys_tilesize;
  x=(x-midx)/rangex;
  if ((x<=-1.0)||(x>=1.0)) return;
  double midy=(double)(g.cameray+(FBH>>1))/(double)NS_sys_tilesize;
  double rangey=(double)FBH/(double)NS_sys_tilesize;
  y=(y-midy)/rangey;
  if ((y<=-1.0)||(y>=1.0)) return;
  
  // Spatial range checks out, so let's check the blackout too.
  if (!sound_blackout_check(rid)) return;
  
  // Our normalized (x) is the final answer for pan.
  // Trim is essentially the reverse of (x,y)'s magnitude.
  double pan=x;
  double trim=1.0-sqrt(x*x+y*y);
  
  egg_play_sound(rid,trim,pan);
}

void sfx_full(int rid) {
  if (!sound_blackout_check(rid)) return;
  egg_play_sound(rid,1.0,0.0);
}

void song(int rid,int repeat) {
  if (rid==g.song_playing) return;
  g.song_playing=rid;
  egg_play_song(1,rid,repeat,0.400,0.0);
}

/* High score persistence.
 */
 
void hiscore_load() {
  g.hiscore=0;
  char tmp[6];
  int tmpc=egg_store_get(tmp,sizeof(tmp),"hiscore",7);
  if (tmpc!=sizeof(tmp)) return;
  int i=0;
  for (;i<tmpc;i++) {
    char ch=tmp[i];
    if ((ch<'0')||(ch>'9')) {
      g.hiscore=0;
      return;
    }
    g.hiscore*=10;
    g.hiscore+=ch-'0';
  }
}

void hiscore_save() {
  if (g.hiscore<0) g.hiscore=0;
  else if (g.hiscore>999999) g.hiscore=999999;
  char tmp[6]={
    '0'+(g.hiscore/100000)%10,
    '0'+(g.hiscore/ 10000)%10,
    '0'+(g.hiscore/  1000)%10,
    '0'+(g.hiscore/   100)%10,
    '0'+(g.hiscore/    10)%10,
    '0'+(g.hiscore       )%10,
  };
  egg_store_set("hiscore",7,tmp,sizeof(tmp));
}
