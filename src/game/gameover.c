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
  song(0,0);
  
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
    thing->zy=roomy+roomh*0.400;
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
   * As in, the enum of ending dispositions. Now we're doing a proper score too, that's further down.
   */
  if (!g.thingc) g.score=NS_score_none;
  else if (!got_marshmallow) g.score=NS_score_no_marshmallow;
  else if (!got_fish) g.score=NS_score_no_fish;
  else if (g.thingc<g.thingc_total) g.score=NS_score_ok;
  else g.score=NS_score_perfect;
  
  /* Calculate a numeric score under 1M.
   * All items instantly: 335000-ish. Unpredictable because there's three seconds accelerated, exact amount depends on update timing.
   * No items: 1
   * Fish alone: 14780
   * Minimum valid: 129559
   * My speed run (finish with around 1:11 left): 724742
   */
  const int PARTICIPATION_BONUS=1; // If you finish the game at all, your score will be greater than zero.
  const int THING_BONUS=14779; // Per thing. It's a prime number in order to shake up the score's lower digits a little.
  const int VALID_BONUS=100000;
  const int PERFECT_BONUS=100000;
  const int TIME_BONUS=1000000; // Scaled by acceleration_time. At best, you can get about 1/4 of this. Only applies when perfect.
  g.points=0;
  g.points+=PARTICIPATION_BONUS;
  g.points+=THING_BONUS*g.thingc;
  if (g.score>=NS_score_ok) g.points+=VALID_BONUS;
  if (g.score>=NS_score_perfect) g.points+=PERFECT_BONUS;
  if (g.score>=NS_score_perfect) g.points+=(int)((g.accelerated_time*TIME_BONUS)/PLAY_TIME);
  if (g.points>999999) g.points=999999;
  if (g.points>g.hiscore) {
    g.hiscore=g.points;
    hiscore_save();
  }
  
  /* Generate a label with score and hiscore.
   */
  if (!g.texid_score) g.texid_score=egg_texture_new();
  char tmp[32];
  int tmpc=snprintf(tmp,sizeof(tmp)," Score: %6d\nRecord: %6d",g.points,g.hiscore);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  font_render_to_texture(g.texid_score,g.font,tmp,tmpc,FBW,FBH,g.enable_color?0x000000ff:0xffffffff);
  egg_texture_get_size(&g.scorew,&g.scoreh,g.texid_score);
  
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
      song(RID_song_shave_and_a_haircut,0);
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
  
  // With a perfect score, after 10 seconds, Dot and Moon jump up and down.
  // Their sprites will manage the activity, we just trigger it.
  if (g.score>=NS_score_perfect) {
    double celebrate_time=10.0;
    if ((pvclock<celebrate_time)&&(g.gameover_clock>=celebrate_time)) {
      struct sprite **spritep=g.spritev;
      int i=g.spritec;
      for (;i-->0;spritep++) {
        struct sprite *sprite=*spritep;
        if (sprite->type==&sprite_type_hero) sprite_hero_celebrate(sprite);
        else if (sprite->type==&sprite_type_moon) sprite_moon_celebrate(sprite);
      }
    }
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
  double eye_close_start_time,eye_close_end_time;
  double wink_start_time,wink_end_time;
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
  eye_close_start_time=msg_start_time;
  eye_close_end_time=eye_close_start_time+1.0;
  wink_start_time=eye_close_end_time+3.0;
  wink_end_time=wink_start_time+0.5;
  
  /* Eyes.
   */
  int eyeopenness;
  if ((g.score<NS_score_ok)&&(g.gameover_clock>=eye_close_start_time)) {
    eyeopenness=(int)((eye_close_end_time-g.gameover_clock)*5.0)/(eye_close_end_time-eye_close_start_time);
  } else {
    eyeopenness=(int)((g.gameover_clock-eye_open_start_time)*5.0)/(eye_open_end_time-eye_open_start_time); // 0=closed .. 5=open
  }
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
      if (dx<-8) dx=-8; else if (dx>8) dx=8;
      int dy=(int)(ny* 4.0); // vertical radius
      if (dy<-2) dy=-2; else if (dy>2) dy=2;
      lballx+=dx;
      lbally+=dy;
      rballx+=dx;
      rbally+=dy;
    }
    if ((eyeopenness==5)&&(g.score>=NS_score_perfect)&&(g.gameover_clock>=wink_start_time)&&(g.gameover_clock<wink_end_time)) {
      // Winking. Draw only the left eye.
      graf_fill_rect(&g.graf,leyex,eyey,eyew,eyeh,0xffffffff);
      graf_set_input(&g.graf,g.texid_sprites);
      graf_tile(&g.graf,lballx,lbally,0x7f,0);
      graf_decal(&g.graf,leyex,eyey,0,119-eyeopenness*eyeh,eyew,eyeh);
    } else {
      graf_fill_rect(&g.graf,leyex,eyey,eyew,eyeh,0xffffffff);
      graf_fill_rect(&g.graf,reyex,eyey,eyew,eyeh,0xffffffff);
      graf_set_input(&g.graf,g.texid_sprites);
      graf_tile(&g.graf,lballx,lbally,0x7f,0);
      graf_tile(&g.graf,rballx,rbally,0x7f,0);
      graf_decal(&g.graf,leyex,eyey,0,119-eyeopenness*eyeh,eyew,eyeh);
      graf_decal(&g.graf,reyex,eyey,0,119-eyeopenness*eyeh,eyew,eyeh);
    }
  }

  /* The final judgment, revealed after a tasteful interval.
   */
  if (g.gameover_clock>=msg_start_time) {
    graf_set_input(&g.graf,g.texid_judgment);
    graf_set_tint(&g.graf,g.enable_color?0x000000ff:0xffffffff);
    int dsty=(FBH>>1)+20-(g.judgmenth>>1);
    graf_decal(&g.graf,(FBW>>1)-(g.judgmentw>>1),dsty,0,0,g.judgmentw,g.judgmenth);
    graf_set_tint(&g.graf,0);
  }
  
  /* Score and high score, high and centered.
   */
  if (g.scorew>0) {
    graf_set_input(&g.graf,g.texid_score);
    graf_decal(&g.graf,(FBW>>1)-(g.scorew>>1),13,0,0,g.scorew,g.scoreh);
  }
}
