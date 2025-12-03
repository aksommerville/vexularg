#include "vexularg.h"

/* Begin.
 * The scene's state is still live and readable when we run.
 * So we are first responsible for tabulating the score and all that.
 */
 
void gameover_begin() {
  g.hello_running=0;
  g.gameover_running=1;
  g.modal_blackout=1.000;
  g.gameover_clock=0.0;
  
  // Music stops as the suspense builds...
  song(0);
  
  /* Put the hero sprite in her initial position, and tell the camera to cut.
   * Note the bounds of the offeratorium.
   */
  g.camera_cut=1;
  double roomx=0.0,roomy=0.0,roomw=0.0,roomh=0.0;
  struct cmdlist_reader reader;
  if (cmdlist_reader_init(&reader,g.mapcmd,g.mapcmdc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_map_sprite: {
            int rid=(cmd.arg[2]<<8)|cmd.arg[3];
            if (rid==RID_sprite_hero) {
              if (g.hero) {
                g.hero->x=cmd.arg[0]+0.5;
                g.hero->y=cmd.arg[1]+0.5;
              }
            }
          } break;
        case CMD_map_camlock: {
            if (cmd.arg[4]==NS_camlock_offeratorium) {
              roomx=cmd.arg[0];
              roomy=cmd.arg[1];
              roomw=cmd.arg[2];
              roomh=cmd.arg[3];
            }
          } break;
      }
    }
  }
  if (g.hero) {
    g.hero->xform=0;
    sprite_hero_force_drop(g.hero);
  }
  
  /* Identify all the things.
   */
  int got_fish=0,got_marshmallow=0;
  g.thingc=0;
  struct sprite **spritep=g.spritev;
  int i=g.spritec;
  for (;i-->0;spritep++) {
    struct sprite *sprite=*spritep;
    if (sprite->defunct) continue;
    if (sprite->type!=&sprite_type_thing) continue;
    // Things outside the offeratorium don't count.
    // They don't actually need to be on the altar.
    if (sprite->x<roomx) continue;
    if (sprite->y<roomy) continue;
    if (sprite->x>roomx+roomw) continue;
    if (sprite->y>roomy+roomh) continue;
    // OK let's offer it. (zx) will wait until we have all of them.
    if (g.thingc>=THING_LIMIT) break;
    struct thing *thing=g.thingv+g.thingc++;
    thing->sprite=sprite;
    thing->ax=sprite->x;
    thing->ay=sprite->y;
    thing->zy=roomy+roomh*0.333;
    switch (sprite->rid) {
      case RID_sprite_fish: got_fish=1; break;
      case RID_sprite_marshmallow: got_marshmallow=1; break;
    }
  }
  if (g.thingc>0) {
    double spacing=roomw/((double)g.thingc+1.0);
    for (i=g.thingc;i-->0;) {
      struct thing *thing=g.thingv+i;
      thing->zx=roomx+spacing*(i+1);
    }
  }
  
  /* "Calculate" the final score.
   */
  if (!g.thingc) g.score=NS_score_none;
  else if (!got_marshmallow) g.score=NS_score_no_marshmallow;
  else if (!got_fish) g.score=NS_score_no_fish;
  else if (g.thingc<g.thingc_total) g.score=NS_score_ok;
  else g.score=NS_score_perfect;
  
  /* Get Vexularg's judgment.
   */
  const char *msg=0;
  int msgc=res_get_string(&msg,1,5+g.score);
  if (!g.texid_judgment) g.texid_judgment=egg_texture_new();
  const int msg_wlimit=NS_sys_tilesize*10-4;
  const int msg_hlimit=NS_sys_tilesize*3-4;
  font_render_to_texture(g.texid_judgment,g.font,msg,msgc,msg_wlimit,msg_hlimit,0x000000ff);
  egg_texture_get_size(&g.judgmentw,&g.judgmenth,g.texid_judgment);
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  double pvclock=g.gameover_clock;
  g.gameover_clock+=elapsed;
  
  if (g.modal_blackout>0.500) {
    g.modal_blackout-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      hello_begin();
    }
  }
  
  // Start the gameover music about when the message appears.
  double musictime=g.thingc?9.5:5.5;
  if ((g.gameover_clock>=musictime)&&(pvclock<musictime)) {
    if (g.score>=NS_score_ok) {
      song(RID_song_shave_and_a_haircut);
    } else {
      song(RID_song_inadequate_offering,0);
    }
  }
  
  // Position things based on gameover_clock.
  double t=(g.gameover_clock-1.0)/4.0;
  if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
  double it=1.0-t;
  struct thing *thing=g.thingv;
  int i=g.thingc;
  for (;i-->0;thing++) {
    thing->sprite->x=thing->ax*it+thing->zx*t;
    thing->sprite->y=thing->ay*it+thing->zy*t;
  }
}

/* Render.
 */
 
void gameover_render() {

  /* Timing is different depending on whether at least one thing is present.
   * The first phase of gameover is showing the things levitate up to the offeratory position,
   * but that is noop when there aren't any things, so skip it.
   * They reach the offeratory position at 5 seconds on gameover_clock.
   */
  double eye_open_start_time,eye_open_end_time,sweep_start_time,sweep_end_time,msg_start_time;
  int sweep_dir; // -1=up, 1=down
  if (g.thingc) {
    eye_open_start_time=5.0;
    sweep_dir=-1;
  } else {
    eye_open_start_time=1.0;
    sweep_dir=1;
  }
  eye_open_end_time=eye_open_start_time+1.0;
  sweep_start_time=eye_open_end_time+0.5;
  sweep_end_time=sweep_start_time+2.0;
  msg_start_time=sweep_end_time+1.0;
  
  /* Eyes.
   */
  int eyeopenness=(int)((g.gameover_clock-eye_open_start_time)*5.0)/(eye_open_end_time-eye_open_start_time); // 0=closed .. 5=open
  if (eyeopenness>0) {
    if (eyeopenness>5) eyeopenness=5;
    const int eyew=23;
    const int eyeh=11;
    const int leyex=(FBW>>1)-1-eyew;
    const int reyex=FBW>>1;
    const int eyey=FBH>>1;
    // Natural eyeball positions:
    int lballx=leyex+(eyew>>1)+1;
    int lbally=eyey+(eyeh>>1)+1;
    int rballx=reyex+(eyew>>1)+1;
    int rbally=eyey+(eyeh>>1)+1;
    // Modify eyeball positions per sweep:
    double sweept=(g.gameover_clock-sweep_start_time)/(sweep_end_time-sweep_start_time);
    if ((sweept>0.0)&&(sweept<1.0)) {
      double nx=sin(sweept*M_PI*2.0);
      double ny=(cos(sweept*M_PI*2.0)-1.0)/2.0;
      if (sweep_dir>0) ny=-ny;
      int dx=(int)(nx*10.0); // horizontal radius
      int dy=(int)(ny* 4.0); // vertical radius
      lballx+=dx;
      lbally+=dy;
      rballx+=dx;
      rbally+=dy;
    }
    graf_fill_rect(&g.graf,leyex,eyey,eyew,eyeh,0xffffffff);
    graf_fill_rect(&g.graf,reyex,eyey,eyew,eyeh,0xffffffff);
    graf_set_input(&g.graf,g.texid_sprites);
    graf_tile(&g.graf,lballx,lbally,0x7f,0);
    graf_tile(&g.graf,rballx,rbally,0x7f,0);
    graf_decal(&g.graf,leyex,eyey,0,119-eyeopenness*eyeh,eyew,eyeh);
    graf_decal(&g.graf,reyex,eyey,0,119-eyeopenness*eyeh,eyew,eyeh);
  }

  /* The final judgment, revealed after a tasteful interval.
   */
  if (g.gameover_clock>=msg_start_time) {
    graf_set_input(&g.graf,g.texid_judgment);
    graf_set_tint(&g.graf,0xffffffff);
    int dsty=(FBH>>1)+20-(g.judgmenth>>1);
    graf_decal(&g.graf,(FBW>>1)-(g.judgmentw>>1),dsty,0,0,g.judgmentw,g.judgmenth);
    graf_set_tint(&g.graf,0);
  }
}
