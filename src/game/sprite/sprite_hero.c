#include "game/vexularg.h"

#define WALK_SPEED        6.000 /* m/s */
#define JUMP_TIME         0.333 /* s */
#define JUMP_INITIAL     20.000 /* m/s */
#define JUMP_DECEL       30.000 /* m/s**2 */
#define GRAVITY_INITIAL   1.000 /* m/s */
#define GRAVITY_MAX      20.000 /* m/s */
#define GRAVITY_ACCEL    50.000 /* m/s**2 */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int walking;
  int falling; // <0=jumping, 0=grounded, >0=falling
  double jumpclock;
  double jumppower; // Only relevant while (falling<0). NB: Positive always.
  int jump_blackout; // Must release button before starting another jump.
  double gravity; // Only relevant while (falling>0).
  double gravity_y0; // Position at start of falling, so we can compute final force.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  SPRITE->jump_blackout=1; // Wait for SOUTH to release before a first jump, in case a stroke of SOUTH started the game.
  return 0;
}

/* Test for down-jump.
 * We'll do it the easy way: Cheat our position down by one smidgeon, then try a regular move down, then reset position.
 */
 
static int hero_can_down_jump(struct sprite *sprite) {
  double y0=sprite->y;
  sprite->y+=0.001;
  int result=sprite_move(sprite,0.0,0.001);
  sprite->y=y0;
  return result;
}

/* Triggers on hitting the ground.
 * Very low-velocity ground hits do not trigger any of these.
 * A full jump, landing 2m down from the start, is about the threshold for "rough". (6 m)
 * TODO a decorative dust effect would be nice here
 */
 
static void hero_land_softly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_soft,sprite->x,sprite->y);
}
 
static void hero_land_mediumly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_medium,sprite->x,sprite->y);
}
 
static void hero_land_roughly(struct sprite *sprite) {
  sfx_spatial(RID_sound_land_hard,sprite->x,sprite->y);
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

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
  
  /* Advance jump or gravity.
   */
  if (SPRITE->falling<0) {
    if (!(g.input&EGG_BTN_SOUTH)) { // Abort jump.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
    } else if ((SPRITE->jumpclock-=elapsed)<=0.0) { // Jump expired.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
    } else { // Continue jumping.
      SPRITE->jumppower-=elapsed*JUMP_DECEL;
      sprite_move(sprite,0.0,-SPRITE->jumppower*elapsed);
    }
  } else if (SPRITE->falling>0) {
    SPRITE->gravity+=GRAVITY_ACCEL*elapsed;
    if (SPRITE->gravity>GRAVITY_MAX) SPRITE->gravity=GRAVITY_MAX;
    if (!sprite_move(sprite,0.0,SPRITE->gravity*elapsed)) { // Hit ground.
      // A full jump is about 4.7 m. Terminal velocity somewhere around 4.
      double distance=sprite->y-SPRITE->gravity_y0;
      if (distance<0.125) ; // Nothing for very short drops, might not be a real fall.
      else if (distance<2.0) hero_land_softly(sprite);
      else if (distance<6.0) hero_land_mediumly(sprite);
      else hero_land_roughly(sprite);
      SPRITE->falling=0;
    }
  } else { // Grounded, but verify.
    if (sprite_move(sprite,0.0,GRAVITY_INITIAL*elapsed)) { // Begin falling.
      SPRITE->falling=1;
      SPRITE->gravity=GRAVITY_INITIAL;
      SPRITE->gravity_y0=sprite->y;
    } else if (!(g.input&EGG_BTN_SOUTH)) { // Not holding SOUTH. Release the blackout if set.
      SPRITE->jump_blackout=0;
    } else if (SPRITE->jump_blackout) { // Wait for SOUTH to release before next jump.
    } else if (g.input&EGG_BTN_DOWN) { // Begin down-jump.
      if (hero_can_down_jump(sprite)) {
        sfx_spatial(RID_sound_downjump,sprite->x,sprite->y);
        sprite->y+=0.001; // Cheat down to escape the platform.
        SPRITE->falling=1;
        SPRITE->gravity=GRAVITY_INITIAL;
        SPRITE->gravity_y0=sprite->y;
        SPRITE->jump_blackout=1;
      } else {
        sfx_spatial(RID_sound_reject,sprite->x,sprite->y);
      }
    } else { // Begin regular jump.
      sfx_spatial(RID_sound_jump,sprite->x,sprite->y);
      SPRITE->falling=-1;
      SPRITE->jump_blackout=1;
      SPRITE->jumppower=JUMP_INITIAL;
      SPRITE->jumpclock=JUMP_TIME;
    }
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int dstx,int dsty) {
  // Our nominal (tileid) is the head. Natural orientation is rightward.
  // Our position is the body.
  uint8_t tileid=sprite->tileid;
  if (0) tileid+=0x20;//TODO if carrying. orthogonal to all others
  if (SPRITE->falling<0) { // Jumping.
    tileid+=3;
  } else if (SPRITE->falling>0) { // Falling.
    tileid+=4;
  } else switch (SPRITE->animframe) { // Idle or walking.
    case 1: tileid+=1; break;
    case 3: tileid+=2; break;
  }
  graf_tile(&g.graf,dstx,dsty-NS_sys_tilesize,tileid,sprite->xform);
  graf_tile(&g.graf,dstx,dsty,tileid+0x10,sprite->xform);
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
