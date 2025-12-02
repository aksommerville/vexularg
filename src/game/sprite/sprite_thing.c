#include "game/vexularg.h"

#define GRAVITY 10.0 /* m/s */

struct sprite_thing {
  struct sprite hdr;
  uint8_t tileid0;
  int role;
  double animclock;
  int carried;
  int same_direction; // While (carried), nonzero means our xform should be the same as the hero's.
};

#define SPRITE ((struct sprite_thing*)sprite)

/* Delete.
 */
 
static void _thing_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _thing_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->role=sprite_res_u16(sprite->cmd,sprite->cmdc,CMD_sprite_role);
  return 0;
}

/* Update.
 */
 
static void _thing_update(struct sprite *sprite,double elapsed) {
  switch (SPRITE->role) {
  
    // Fan animates and blows other sprites away, along the horizontal line of sight.
    case NS_role_fan: {
        if ((SPRITE->animclock-=elapsed)<=0.0) {
          SPRITE->animclock+=0.0875;
          sprite->tileid++;
          if (sprite->tileid>=SPRITE->tileid0+5) sprite->tileid=SPRITE->tileid0;
        }
        //TODO blow
      } break;
      
    // Magnet is the opposite of fan.
    case NS_role_magnet: {
        //TODO suck
      } break;
      
    // Balloon diminishes gravity and enhances jump -- hero takes care of that. When left alone, we slowly rise.
    case NS_role_balloon: if (!SPRITE->carried) {
        //TODO float
      } break;
    
    // Trampoline makes the hero bounce when she lands on us -- hero takes care of that. But we need to animate the interaction.
    case NS_role_trampoline: {
        //TODO bounce animation
      } break;
  }
  
  /* If we're being carried, bind to the hero's edge.
   * The ongoing activities above do keep happening during the carry.
   */
  if (SPRITE->carried) {
    if (g.hero) {
      sprite->y=g.hero->y-0.5;
      if (g.hero->xform&EGG_XFORM_XREV) {
        sprite->x=g.hero->x-1.0;
        if (SPRITE->same_direction) sprite->xform=EGG_XFORM_XREV;
        else sprite->xform=0;
      } else {
        sprite->x=g.hero->x+1.0;
        if (SPRITE->same_direction) sprite->xform=0;
        else sprite->xform=EGG_XFORM_XREV;
      }
    }
  
  /* We're not being carried, so apply gravity.
   * Unlike the hero, our gravity is constant.
   */
  } else if (SPRITE->role!=NS_role_balloon) {
    sprite_move(sprite,0.0,GRAVITY*elapsed);
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_thing={
  .name="thing",
  .objlen=sizeof(struct sprite_thing),
  .del=_thing_del,
  .init=_thing_init,
  .update=_thing_update,
};

/* Get carried.
 */
 
int sprite_thing_get_carried(struct sprite *sprite,struct sprite *hero) {
  if (!sprite||(sprite->type!=&sprite_type_thing)) return 0;
  //TODO
  SPRITE->carried=1;
  SPRITE->same_direction=((sprite->xform&EGG_XFORM_XREV)==(hero->xform&EGG_XFORM_XREV));
  sprite->solid=0;
  return 1;
}

/* Get dropped.
 * Hero will call this on every thing when she drops something.
 * If we're not the thing, don't raise a fuss over it.
 */
 
int sprite_thing_get_dropped(struct sprite *sprite,struct sprite *hero) {
  if (!sprite||(sprite->type!=&sprite_type_thing)) return 0;
  if (!SPRITE->carried) return 0; // see, no fuss
  /* If we really wanted to get slick about it, in collision cases we should try nearby position, and maybe shuffle the hero.
   * Instead we just reject the drop if we're overlapping something.
   */
  if (sprite_collides_anything(sprite)) return 0;
  SPRITE->carried=0;
  sprite->solid=1;
  return 1;
}
