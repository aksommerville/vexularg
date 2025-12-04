#include "game/vexularg.h"

#define GRAVITY 10.0 /* m/s */

struct sprite_thing {
  struct sprite hdr;
  uint8_t tileid0;
  int role;
  double animclock;
  int carried;
  int same_direction; // While (carried), nonzero means our xform should be the same as the hero's.
  double ratelimit; // Clock that runs when fan or magnet strikes nothing, so we don't spin too much wheel.
  int qx,qy; // Quantized position.
  int in_offeratorium;
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

/* Pull or push things in my horizontal line of sight.
 * (d>0) to push for (d<0) to pull.
 * Returns nonzero if we attempted to move another sprite.
 */
 
static int thing_update_fan_or_magnet(struct sprite *sprite,double elapsed,double d) {
  
  // Start with a ribbon of the correct vertical bounds, extending to both horizontal edges.
  double l=0.0,r=g.mapw;
  double t=sprite->y-0.5,b=sprite->y+0.5;
  
  // Then crop it horizontally based on my xform. Natural orientation is rightward.
  // Fudge outward a little to make sure we won't detect ourself.
  if (sprite->xform&EGG_XFORM_XREV) r=sprite->x-0.6;
  else l=sprite->x+0.6;
  
  // Scan the entire set of sprites to find the nearest which is attractable and touches the ribbon.
  // We only affect one other sprite at a time, or none.
  struct sprite *best=0;
  double bestdistance;
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if (!other->attractable) continue;
    if (other->y-0.5>=b) continue;
    if (other->y+0.5<=t) continue;
    double distance;
    if (sprite->xform&EGG_XFORM_XREV) distance=sprite->x-other->x;
    else distance=other->x-sprite->x;
    if (distance<0.5) continue; // behind me
    if (!best||(distance<bestdistance)) {
      best=other;
      bestdistance=distance;
    }
  }
  if (!best) return 0;
  
  // Intersect our vertical extents and quantize the ribbon between us.
  // Intersecting is super simple, because all sprites are the same height.
  if (sprite->y>best->y) {
    t=sprite->y-0.5;
    b=best->y+0.5;
  } else {
    t=best->y-0.5;
    b=sprite->y+0.5;
  }
  int rowa=(int)t; if (rowa<0) rowa=0;
  int rowz=(int)(b-0.001); if (rowz>=g.maph) rowz=g.maph-1;
  if (rowa>rowz) return 0;
  int cola=(int)sprite->x;
  int colz=(int)best->x;
  if (cola>colz) {
    int tmp=cola;
    cola=colz;
    colz=tmp;
  }
  if (cola<0) cola=0;
  if (colz>=g.mapw) colz=g.mapw-1;
  if (cola>colz) return 0;
  
  // At least one row of that ribbon must be fully passable.
  int lineofsight=0;
  const uint8_t *cellrow=g.cellv+rowa*g.mapw+cola;
  int row=rowa;
  for (;row<=rowz;row++,cellrow+=g.mapw) {
    lineofsight=1;
    const uint8_t *cellp=cellrow;
    int col=cola;
    for (;col<=colz;col++,cellp++) {
      if (g.physics[*cellp]==NS_physics_solid) {
        lineofsight=0;
        break;
      }
    }
    if (lineofsight) break;
  }
  if (!lineofsight) return 0;
  
  // OK let's move it!
  if (sprite->xform&EGG_XFORM_XREV) d=-d;
  const double BLOW_SPEED=4.000;
  sprite_move(best,d*elapsed*BLOW_SPEED,0.0);
  return 1;
}

/* Check offeratorium.
 */
 
static int cell_in_offeratorium(int x,int y) {
  const struct camlock *camlock=g.camlockv;
  int i=g.camlockc;
  for (;i-->0;camlock++) {
    if (camlock->id!=NS_camlock_offeratorium) continue;
    if (x<camlock->x) return 0;
    if (y<camlock->y) return 0;
    if (x>=camlock->x+camlock->w) return 0;
    if (y>=camlock->y+camlock->h) return 0;
    return 1;
  }
  return 0;
}

/* Update.
 */
 
static void _thing_update(struct sprite *sprite,double elapsed) {
  switch (SPRITE->role) {
  
    // Fan animates and blows other sprites away, along the horizontal line of sight.
    case NS_role_fan: {
        if ((SPRITE->animclock-=elapsed)<=0.0) {
          if ((SPRITE->animclock+=0.0875)<0.0) SPRITE->animclock=0.0;
          sprite->tileid++;
          if (sprite->tileid>=SPRITE->tileid0+5) sprite->tileid=SPRITE->tileid0;
        }
        if (g.gameover_running) break;
        if (SPRITE->ratelimit>0.0) SPRITE->ratelimit-=elapsed;
        else if (!thing_update_fan_or_magnet(sprite,elapsed,1.0)) SPRITE->ratelimit=0.250;
      } break;
      
    // Magnet is the opposite of fan.
    case NS_role_magnet: {
        if (g.gameover_running) break;
        if (SPRITE->ratelimit>0.0) SPRITE->ratelimit-=elapsed;
        else if (!thing_update_fan_or_magnet(sprite,elapsed,-1.0)) SPRITE->ratelimit=0.250;
      } break;
      
    // Balloon diminishes gravity and enhances jump -- hero takes care of that. When left alone, we slowly rise.
    case NS_role_balloon: if (!SPRITE->carried&&!g.gameover_running) {
        sprite_move(sprite,0.0,-1.500*elapsed);
      } break;
    
    // Trampoline makes the hero bounce when she lands on us -- hero takes care of that. But we need to animate the interaction.
    case NS_role_trampoline: {
        if (SPRITE->animclock>0.0) {
          if ((SPRITE->animclock-=elapsed)<0.0) SPRITE->animclock=0.0;
          sprite->tileid=SPRITE->tileid0+(((int)(SPRITE->animclock*8.0))&1);
        } else {
          sprite->tileid=SPRITE->tileid0;
        }
      } break;
  }
  
  /* During gameover, we do nothing but animate, so we're done.
   */
  if (g.gameover_running) return;
  
  /* Take my quantized position, and if it changed, recheck whether I'm in the offeratorium.
   * Then if we are not in it, clear the global collector.
   */
  int qx=(int)sprite->x;
  int qy=(int)sprite->y;
  if ((qx!=SPRITE->qx)||(qy!=SPRITE->qy)) {
    SPRITE->qx=qx;
    SPRITE->qy=qy;
    SPRITE->in_offeratorium=cell_in_offeratorium(qx,qy);
  }
  if (!SPRITE->in_offeratorium) {
    g.all_things_in_offeratorium=0;
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

/* Trivial accessors.
 */
 
int sprite_thing_get_role(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_thing)) return NS_role_inert;
  return SPRITE->role;
}

void sprite_thing_animate_trampoline(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_thing)) return;
  if (SPRITE->role!=NS_role_trampoline) return;
  SPRITE->animclock=1.0;
}

/* Get carried.
 */
 
int sprite_thing_get_carried(struct sprite *sprite,struct sprite *hero) {
  if (!sprite||(sprite->type!=&sprite_type_thing)) return 0;
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
  if (g.input&EGG_BTN_DOWN) {
    double hy0=hero->y;
    hero->y-=1.0;
    if (sprite_collides_anything(hero)) {
      hero->y=hy0;
      return 0;
    }
    sprite->x=hero->x;
    sprite->y=hy0;
  } else {
    // Correct our x position right quick. This comes up when hero is under the influence of a magnet or fan.
    if (hero->xform==EGG_XFORM_XREV) sprite->x=hero->x-1.0;
    else sprite->x=hero->x+1.0;
    if (sprite_collides_anything(sprite)) return 0;
  }
  SPRITE->carried=0;
  sprite->solid=1;
  return 1;
}
