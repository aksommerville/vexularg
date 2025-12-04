#include "game/vexularg.h"

#define WALK_SPEED        6.000 /* m/s */
#define JUMP_TIME         0.333 /* s */
#define JUMP_INITIAL     20.000 /* m/s */
#define JUMP_DECEL       30.000 /* m/s**2 */
#define GRAVITY_INITIAL   1.000 /* m/s */
#define GRAVITY_MAX      20.000 /* m/s */
#define GRAVITY_ACCEL    50.000 /* m/s**2 */
#define GRAVITY_MAX_BALLOON    5.000
#define GRAVITY_ACCEL_BALLOON 30.000
#define JUMP_TIME_BALLOON      0.750
#define TRAMPOLINE_MIN     5.000 /* m/s */
#define TRAMPOLINE_RATE    2.000 /* m/s per meter of fall before */
#define TRAMPOLINE_AUGMENT 2.000 /* Multiplier, if you're holding jump. */
#define JUMP_LIMIT        32.000 /* m/s, maximum initial jump rate off trampoline. */
#define DOWN_JUMP_CHEAT 0.002 /* m. Amount to fudge on (y) to test and escape oneways downward. Beware! 0.001 is not enough to avoid all rounding errors. */
#define PREJUMP_TIME 0.125 /* A little tolerance for pressing A just before landing. */
#define COYOTE_TIME 0.100 /* Brief period after stepping off an edge when jumping will still work. */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int walking;
  int falling; // <0=jumping, 0=grounded, >0=falling
  double jumpclock;
  double jumppower; // Only relevant while (falling<0). NB: Positive always.
  int jump_blackout; // Must release button before starting another jump.
  int trampoline; // If nonzero, jump is not cancellable.
  double gravity; // Only relevant while (falling>0).
  double gravity_y0; // Position at start of falling, so we can compute final force.
  struct sprite *pumpkin; // STRONG and unlisted, if not null.
  int pumpkin_role;
  int celebrate;
  double time_since_south;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
  sprite_del(SPRITE->pumpkin);
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  SPRITE->jump_blackout=1; // Wait for SOUTH to release before a first jump, in case a stroke of SOUTH started the game.
  return 0;
}

/* Find a pumpkin we can pick up.
 */
 
static struct sprite *hero_find_pumpkin(struct sprite *sprite) {
  double fx=sprite->x;
  double fy=sprite->y;
  if (g.input&EGG_BTN_DOWN) fy+=1.0;
  else if (sprite->xform) fx-=1.0;
  else fx+=1.0;
  double fyupper=fy;
  if (!(g.input&EGG_BTN_DOWN)) fyupper-=1.0;
  struct sprite *upper=0;
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if ( // Do return Moon Song, so we don't make the reject noise when you meant to accelerate her. (it's not perfect)
      (other->type!=&sprite_type_thing)&&
      (other->type!=&sprite_type_moon)
    ) continue;
    double dx=fx-other->x;
    if ((dx<-0.5)||(dx>0.5)) continue;
    double dy=fy-other->y;
    if ((dy>=-0.5)&&(dy<=0.5)) return other;
    if (!upper) {
      dy=fyupper-other->y;
      if ((dy>=-0.5)&&(dy<=0.5)) upper=other;
    }
  }
  return upper;
}

/* If we're carrying something, drop it.
 * Otherwise, if something is in range, pick it up.
 */
 
static void hero_pickup_or_drop(struct sprite *sprite) {
  if (SPRITE->pumpkin) {
    if (sprite_thing_get_dropped(SPRITE->pumpkin,sprite)) {
      if (SPRITE->pumpkin_role==NS_role_balloon) {
        if (SPRITE->falling>0) { // Let go of balloon midair. Reset gravity things.
          SPRITE->gravity=GRAVITY_INITIAL;
          SPRITE->gravity_y0=sprite->y;
          SPRITE->jumpclock=0.0;
        }
      }
      sprite_list(SPRITE->pumpkin);
      SPRITE->pumpkin=0;
      SPRITE->pumpkin_role=NS_role_inert;
      sfx_spatial(RID_sound_drop,sprite->x,sprite->y);
    } else {
      sfx_spatial(RID_sound_reject,sprite->x,sprite->y);
    }
    return;
  }
  struct sprite *pumpkin=hero_find_pumpkin(sprite);
  if (pumpkin&&(pumpkin->type==&sprite_type_moon)) {
    // Do nothing. User pressed B to accelerate Moon Song.
  } else if (sprite_thing_get_carried(pumpkin,sprite)) {
    pumpkin->unlist_soon=1;
    SPRITE->pumpkin=pumpkin;
    SPRITE->pumpkin_role=sprite_thing_get_role(pumpkin);
    sfx_spatial(RID_sound_pickup,sprite->x,sprite->y);
  } else {
    sfx_spatial(RID_sound_reject,sprite->x,sprite->y);
  }
}

// Drop pumpkin for gameover. Don't require a valid position and don't make a sound.
void sprite_hero_force_drop(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return;
  if (!SPRITE->pumpkin) return;
  SPRITE->pumpkin->x=sprite->x+1.0;
  SPRITE->pumpkin->y=sprite->y-0.5;
  sprite_list(SPRITE->pumpkin);
  SPRITE->pumpkin=0;
  SPRITE->pumpkin_role=NS_role_inert;
}

/* Test for down-jump.
 * We'll do it the easy way: Cheat our position down by one smidgeon, then try a regular move down, then reset position.
 */
 
static int hero_can_down_jump(struct sprite *sprite) {
  double y0=sprite->y;
  sprite->y+=DOWN_JUMP_CHEAT;
  int result=sprite_move(sprite,0.0,DOWN_JUMP_CHEAT);
  sprite->y=y0;
  return result;
}

/* Triggers on hitting the ground.
 * Very low-velocity ground hits do not trigger any of these.
 * A full jump, landing 2m down from the start, is about the threshold for "rough". (6 m)
 */
 
static void hero_land_softly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_soft,sprite->x,sprite->y);
}
 
static void hero_land_mediumly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_medium,sprite->x,sprite->y);
  sprite_spawn(0,sprite->x,sprite->y,0,RID_sprite_dust,0,0);
}
 
static void hero_land_roughly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_hard,sprite->x,sprite->y);
  sprite_spawn(0,sprite->x,sprite->y,0,RID_sprite_dust,0,0);
  g.earthquake_time=0.100;
}

/* Begin regular jump.
 */
 
static void hero_begin_jump(struct sprite *sprite) {
  sfx_spatial(RID_sound_jump,sprite->x,sprite->y);
  SPRITE->falling=-1;
  SPRITE->jump_blackout=1;
  SPRITE->jumppower=JUMP_INITIAL;
  if (SPRITE->pumpkin_role==NS_role_balloon) {
    SPRITE->jumpclock=JUMP_TIME_BALLOON;
  } else {
    SPRITE->jumpclock=JUMP_TIME;
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  /* If gameover, nullify everything and get out.
   * gameover_begin() must take care of our pumpkin. If it didn't, leave it be.
   */
  if (g.gameover_running) {
    SPRITE->walking=0;
    SPRITE->falling=0;
    SPRITE->animframe=0;
    return;
  }

  /* Walk left and right.
   */
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: SPRITE->walking=1; sprite->xform=EGG_XFORM_XREV; sprite_move(sprite,-WALK_SPEED*elapsed,0.0); break;
    case EGG_BTN_RIGHT: SPRITE->walking=1; sprite->xform=0; sprite_move(sprite,WALK_SPEED*elapsed,0.0); break;
    default: SPRITE->walking=0;
  }
  
  /* If walking, animate.
   */
  if (SPRITE->walking) {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.200;
      if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    }
  } else {
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
  }
  
  /* Track strokes of SOUTH for prejump. Actual jump processing is further below.
   */
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    SPRITE->time_since_south=0.0;
  } else {
    SPRITE->time_since_south+=elapsed;
  }
  
  /* Advance jump or gravity.
   */
  if (SPRITE->falling<0) {
    if (!(g.input&EGG_BTN_SOUTH)&&!SPRITE->trampoline) { // Abort jump.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
      SPRITE->jump_blackout=1;
      SPRITE->jumpclock=1.0; // Too high for an accidental coyote.
    } else if ((SPRITE->jumpclock-=elapsed)<=0.0) { // Jump expired.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
      SPRITE->jump_blackout=1;
      SPRITE->trampoline=0;
      SPRITE->jumpclock=1.0; // Too high for an accidental coyote.
    } else { // Continue jumping.
      SPRITE->jumppower-=elapsed*JUMP_DECEL;
      sprite_move(sprite,0.0,-SPRITE->jumppower*elapsed);
    }
  } else if (SPRITE->falling>0) {
    SPRITE->jumpclock+=elapsed;
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)&&!SPRITE->jump_blackout&&(SPRITE->jumpclock<COYOTE_TIME)) {
      hero_begin_jump(sprite);
      goto _done_jump_;
    }
    if (SPRITE->pumpkin_role==NS_role_balloon) {
      SPRITE->gravity+=GRAVITY_ACCEL_BALLOON*elapsed;
      if (SPRITE->gravity>GRAVITY_MAX_BALLOON) SPRITE->gravity=GRAVITY_MAX_BALLOON;
    } else {
      SPRITE->gravity+=GRAVITY_ACCEL*elapsed;
      if (SPRITE->gravity>GRAVITY_MAX) SPRITE->gravity=GRAVITY_MAX;
    }
    if (!sprite_move(sprite,0.0,SPRITE->gravity*elapsed)) { // Hit ground.
      // A full jump is about 4.7 m. Terminal velocity somewhere around 4.
      SPRITE->falling=0;
      double distance=sprite->y-SPRITE->gravity_y0;
      if (sprite_thing_get_role(sprite->collcause)==NS_role_trampoline) {
        SPRITE->jumppower=TRAMPOLINE_MIN+TRAMPOLINE_RATE*distance;
        if (g.input&EGG_BTN_SOUTH) { // Hit trampoline while jumping: multiply force.
          SPRITE->jumppower*=TRAMPOLINE_AUGMENT;
        }
        if (SPRITE->jumppower>JUMP_LIMIT) {
          SPRITE->jumppower=JUMP_LIMIT;
        }
        if (SPRITE->jumppower>=1.0) sfx_spatial(RID_sound_trampoline_big,sprite->collcause->x,sprite->collcause->y);
        else sfx_spatial(RID_sound_trampoline_lil,sprite->collcause->x,sprite->collcause->y);
        sprite_thing_animate_trampoline(sprite->collcause);
        SPRITE->falling=-1;
        SPRITE->jumpclock=JUMP_TIME;
        SPRITE->jump_blackout=1;
        SPRITE->trampoline=1;
      } else if ((g.input&EGG_BTN_SOUTH)&&(SPRITE->time_since_south<PREJUMP_TIME)) { // Pressed A a little before landing -- allow the re-jump.
        hero_begin_jump(sprite);
      } else if ((distance<0.125)||(SPRITE->pumpkin_role==NS_role_balloon)) ; // Nothing for very short drops, might not be a real fall.
      else if (distance<2.0) hero_land_softly(sprite);
      else if (distance<6.0) hero_land_mediumly(sprite);
      else hero_land_roughly(sprite);
    }
  } else { // Grounded, but verify.
    if (sprite_move(sprite,0.0,GRAVITY_INITIAL*elapsed)) { // Begin falling.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
      SPRITE->jumpclock=0.0;
    } else if (!(g.input&EGG_BTN_SOUTH)) { // Not holding SOUTH. Release the blackout if set.
      SPRITE->jump_blackout=0;
    } else if (SPRITE->jump_blackout||(g.pvinput&EGG_BTN_SOUTH)) { // Wait for SOUTH to release before next jump.
    } else if (g.input&EGG_BTN_DOWN) { // Begin down-jump.
      if (hero_can_down_jump(sprite)) {
        sfx_spatial(RID_sound_downjump,sprite->x,sprite->y);
        sprite->y+=DOWN_JUMP_CHEAT; // Cheat down to escape the platform.
        SPRITE->falling=1;
        SPRITE->gravity=GRAVITY_INITIAL;
        SPRITE->gravity_y0=sprite->y;
        SPRITE->jump_blackout=1;
      } else if (!(g.pvinput&EGG_BTN_SOUTH)) {
        sfx_spatial(RID_sound_reject,sprite->x,sprite->y);
      }
    } else { // Begin regular jump.
      hero_begin_jump(sprite);
    }
  }
 _done_jump_:;
  
  // Pick up or drop things on WEST.
  if ((g.input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) {
    hero_pickup_or_drop(sprite);
  }
  
  // If we have a pumpkin, update it.
  if (SPRITE->pumpkin) {
    if (SPRITE->pumpkin->defunct) {
      sprite_del(SPRITE->pumpkin);
      SPRITE->pumpkin=0;
    } else if (SPRITE->pumpkin->type->update) {
      SPRITE->pumpkin->type->update(SPRITE->pumpkin,elapsed);
    }
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int dstx,int dsty) {
  // Our nominal (tileid) is the head. Natural orientation is rightward.
  // Our position is the body.
  uint8_t tileid=sprite->tileid;
  if (SPRITE->pumpkin) tileid+=0x20;
  if (SPRITE->falling<0) { // Jumping.
    tileid+=3;
  } else if (SPRITE->falling>0) { // Falling.
    tileid+=4;
  } else switch (SPRITE->animframe) { // Idle or walking.
    case 1: tileid+=1; break;
    case 3: tileid+=2; break;
  }
  
  // If celebrating, bounce up and down per global frame count, and show a heart above my head.
  if (SPRITE->celebrate) {
    graf_tile(&g.graf,dstx,dsty-NS_sys_tilesize*2-2,0x47,0);
    if (g.framec%20>=10) dsty--;
  }
  
  graf_tile(&g.graf,dstx,dsty-NS_sys_tilesize,tileid,sprite->xform);
  graf_tile(&g.graf,dstx,dsty,tileid+0x10,sprite->xform);
  
  /* If we have a pumpkin, render it.
   * It's not in the global list while carried, so it won't render itself.
   * And that's important! Rounding errors would cause it to jitter relative to the hero if it rendered independently.
   */
  if (SPRITE->pumpkin) {
    int pdstx=dstx;
    if (sprite->xform&EGG_XFORM_XREV) pdstx-=NS_sys_tilesize; else pdstx+=NS_sys_tilesize;
    int pdsty=dsty-(NS_sys_tilesize>>1);
    if (SPRITE->pumpkin->type->render) {
      SPRITE->pumpkin->type->render(SPRITE->pumpkin,pdstx,pdsty);
    } else {
      graf_tile(&g.graf,pdstx,pdsty,SPRITE->pumpkin->tileid,SPRITE->pumpkin->xform);
    }
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};

/* Begin celebration.
 */
 
void sprite_hero_celebrate(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return;
  SPRITE->celebrate=1;
}

struct sprite *sprite_hero_get_pumpkin(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0;
  return SPRITE->pumpkin;
}
