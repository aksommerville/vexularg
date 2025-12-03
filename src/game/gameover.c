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
  
  //TODO song
  
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
  fprintf(stderr,"score=%d, Vexularg's opinion: '%.*s'\n",g.score,msgc,msg);
  if (!g.texid_judgment) g.texid_judgment=egg_texture_new();
  const int msg_wlimit=NS_sys_tilesize*7-4;
  const int msg_hlimit=NS_sys_tilesize*3-4;
  font_render_to_texture(g.texid_judgment,g.font,msg,msgc,msg_wlimit,msg_hlimit,0x000000ff);
  egg_texture_get_size(&g.judgmentw,&g.judgmenth,g.texid_judgment);
}

/* Update.
 */
 
void gameover_update(double elapsed) {
  g.gameover_clock+=elapsed;
  if (g.modal_blackout>0.500) {
    g.modal_blackout-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      hello_begin();
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
  
  //TODO After the things have landed, say 6 or 7 seconds on gameover_clock, Vexularg appears and passes judgment.
  // Needs a word bubble, and graphics for Vexularg.
  if (g.gameover_clock>=6.0) {
    graf_set_input(&g.graf,g.texid_judgment);
    graf_set_tint(&g.graf,0xffffffff);
    graf_decal(&g.graf,(FBW>>1)-(g.judgmentw>>1),(FBH>>1)-(g.judgmenth>>1),0,0,g.judgmentw,g.judgmenth);
    graf_set_tint(&g.graf,0);
  }
}
