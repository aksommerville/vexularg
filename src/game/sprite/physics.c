#include "game/vexularg.h"

#define SMALL (0.001)

/* Move sprite with collision correction.
 */
 
int sprite_move(struct sprite *sprite,double dx,double dy) {

  // Get out fast for invalid requests, eg not solid.
  if (!sprite) return 0;
  sprite->collcause=0;
  if (!sprite->solid) {
    sprite->x+=dx;
    sprite->y+=dy;
    return 1;
  }
  
  // Recur if multi-axis, or enumerate the axis, or get out if zero.
  uint8_t dir;
  if (dx<-0.0) {
    dir=0x10;
    if ((dy<-0.0)||(dy>0.0)) return sprite_move(sprite,dx,0.0)||sprite_move(sprite,0.0,dy);
  } else if (dx>0.0) {
    dir=0x08;
    if ((dy<-0.0)||(dy>0.0)) return sprite_move(sprite,dx,0.0)||sprite_move(sprite,0.0,dy);
  } else if (dy<-0.0) dir=0x40;
  else if (dy>0.0) dir=0x02;
  else return 0;
  
  // Select the new position and bounds around it.
  double nx=sprite->x+dx;
  double ny=sprite->y+dy;
  double sl,st,sr,sb;
  #define REFRESH_BOUNDS { \
    switch (dir) { \
      case 0x40: if (ny>=sprite->y) return 0; break; \
      case 0x10: if (nx>=sprite->x) return 0; break; \
      case 0x08: if (nx<=sprite->x) return 0; break; \
      case 0x02: if (ny<=sprite->y) return 0; break; \
    } \
    sl=nx-0.5; \
    st=ny-0.5; \
    sr=nx+0.5; \
    sb=ny+0.5; \
  }
  REFRESH_BOUNDS
  
  /* Check for grid collisions.
   */
  int cola=(int)sl; if (cola<0) cola=0;
  int colz=(int)(sr-SMALL); if (colz>=g.mapw) colz=g.mapw-1;
  int rowa=(int)st; if (rowa<0) rowa=0;
  int rowz=(int)(sb-SMALL); if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    // To do it right, we ought to sweep the grid in the direction of motion.
    // I'm not sure that it will matter for us, so let's try the easy way first.
    const uint8_t *cellrow=g.cellv+rowa*g.mapw+cola;
    int row=rowa;
    for (;row<=rowz;row++,cellrow+=g.mapw) {
      const uint8_t *cellp=cellrow;
      int col=cola;
      for (;col<=colz;col++,cellp++) {
        uint8_t physics=g.physics[*cellp];
        switch (physics) {
          // Always:
          case NS_physics_solid:
            break;
          // Sometimes:
          case NS_physics_oneway: {
              if (dir!=0x02) continue; // Not moving down; not a collision.
              int pvtoe=(int)(sprite->y+0.5-SMALL);
              if (pvtoe>=row) continue; // Already past the oneway's top edge; not a collision.
            } break; // Collision.
          // Never:
          default: continue;
        }
        // There is a grid collision. Back off to rectify.
        switch (dir) {
          case 0x40: ny=row+1.5; REFRESH_BOUNDS; break;
          case 0x10: nx=col+1.5; REFRESH_BOUNDS; break;
          case 0x08: nx=col-0.5; REFRESH_BOUNDS; break;
          case 0x02: ny=row-0.5; REFRESH_BOUNDS; break;
        }
      }
    }
  }
  
  /* Check for collisions against other solid sprites.
   */
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (!other->solid) continue;
    if (other->defunct) continue;
    if (other==sprite) continue;
    
    if (other->x-0.5>=sr) continue;
    if (other->x+0.5<=sl) continue;
    if (other->y-0.5>=sb) continue;
    if (other->y+0.5<=st) continue;
    
    // Sprites collide. Back off.
    sprite->collcause=other;
    switch (dir) {
      case 0x40: ny=other->y+1.0; REFRESH_BOUNDS; break;
      case 0x10: nx=other->x+1.0; REFRESH_BOUNDS; break;
      case 0x08: nx=other->x-1.0; REFRESH_BOUNDS; break;
      case 0x02: ny=other->y-1.0; REFRESH_BOUNDS; break;
    }
  }
  
  /* OK, let's keep the new position.
   */
  #undef REFRESH_BOUNDS
  sprite->x=nx;
  sprite->y=ny;
  return 1;
}

/* Collision test only, no rectification.
 */
 
int sprite_collides_anything(const struct sprite *sprite) {
  if (!sprite) return 0;
  
  int cola=(int)(sprite->x-0.5); if (cola<0) cola=0;
  int colz=(int)(sprite->x+0.5-SMALL); if (colz>=g.mapw) colz=g.mapw-1;
  int rowa=(int)(sprite->y-0.5); if (rowa<0) rowa=0;
  int rowz=(int)(sprite->y+0.5-SMALL); if (rowz>=g.maph) rowz=g.maph-1;
  if ((cola<=colz)&&(rowa<=rowz)) {
    const uint8_t *cellrow=g.cellv+rowa*g.mapw+cola;
    int row=rowa;
    for (;row<=rowz;row++,cellrow+=g.mapw) {
      const uint8_t *cellp=cellrow;
      int col=cola;
      for (;col<=colz;col++,cellp++) {
        uint8_t physics=g.physics[*cellp];
        switch (physics) {
          // Always:
          case NS_physics_solid:
            break;
          // oneways never count here, since the sprite is not in motion.
          // Never:
          default: continue;
        }
        return 1;
      }
    }
  }
  
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (!other->solid) continue;
    if (other->defunct) continue;
    if (other==sprite) continue;
    
    double dx=other->x-sprite->x;
    if ((dx<=-1.0)||(dx>=1.0)) continue;
    double dy=other->y-sprite->y;
    if ((dy<=-1.0)||(dy>=1.0)) continue;
    
    return 1;
  }
  
  return 0;
}
