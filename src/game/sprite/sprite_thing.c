#include "game/vexularg.h"

struct sprite_thing {
  struct sprite hdr;
  uint8_t tileid0;
  int role;
  double animclock;
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
  sprite->layer=101; // Important that we update after the hero, to simplify setting position when carried.
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
    case NS_role_balloon: {
        //TODO float
        // Return so as not to get the generic gravity handling.
      } return;
    
    // Trampoline makes the hero bounce when she lands on us -- hero takes care of that. But we need to animate the interaction.
    case NS_role_trampoline: {
        //TODO bounce animation
      } break;
  }
  //TODO generic gravity
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
