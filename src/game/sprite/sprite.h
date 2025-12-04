/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

/* Generic sprite instance.
 **************************************************************************/
 
struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Omit to render (tileid,xform) at (dstx,dsty).
   * (dstx,dsty) is the sprite's (x,y) quantized and translated to framebuffer coords.
   * If you render from any other texture, you must restore texid_sprites in g.graf before returning.
   */
  void (*render)(struct sprite *sprite,int dstx,int dsty);
};

struct sprite {
  const struct sprite_type *type;
  int defunct; // Set nonzero to reap after the current update cycle. Do not delete sprites directly.
  int unlist_soon; // Ugly hack to have this sprite unlisted at the end of the update.
  double x,y; // World meters.
  uint8_t tileid,xform;
  int rid;
  uint32_t arg;
  const void *cmd; // Entire sprite resource, including signature.
  int cmdc;
  int solid;
  struct sprite *collcause; // WEAK. After a move returns zero, if caused by another sprite, we disclose it here.
  int layer; // Render order low to high. Hero is at 100, and that is the default.
  int attractable; // Magnet and fan can affect.
};

/* Primitive delete and initialize.
 * Don't use these from outside. Prefer "defunct" and "spawn".
 * sprite_del_unlisted() if the caller guarantees it's not in (g.spritev), so we can skip searching.
 */
void sprite_del(struct sprite *sprite);
void sprite_del_unlisted(struct sprite *sprite);
struct sprite *sprite_new(
  const struct sprite_type *type,
  double x,double y,
  uint32_t arg,
  int rid,const void *cmd,int cmdc
);
void sprite_unlist(struct sprite *sprite);
int sprite_list(struct sprite *sprite);

/* Create a sprite, register it globally, and return a WEAK reference.
 * Provide as much detail as you have and we'll fill in the rest.
 */
struct sprite *sprite_spawn(
  const struct sprite_type *type,
  double x,double y,
  uint32_t arg,
  int rid,const void *cmd,int cmdc
);

/* Usually you'll want to read the whole command list just once, and shouldn't use these.
 * But there are odd cases where you want just the first appearance of one command, conveniently.
 */
int sprite_res_cmd(void *dstpp,const void *cmd,int cmdc,int opcode);
int sprite_res_u16(const void *cmd,int cmdc,int opcode);

/* Sprite types.
 ************************************************************/
 
#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

const struct sprite_type *sprite_type_from_id(int id);

void sprite_hero_force_drop(struct sprite *sprite);
void sprite_hero_celebrate(struct sprite *sprite);
struct sprite *sprite_hero_get_pumpkin(struct sprite *sprite);

void sprite_moon_celebrate(struct sprite *sprite);

int sprite_thing_get_carried(struct sprite *sprite,struct sprite *hero);
int sprite_thing_get_dropped(struct sprite *sprite,struct sprite *hero);
int sprite_thing_get_role(const struct sprite *sprite);
void sprite_thing_animate_trampoline(struct sprite *sprite);

/* Physics.
 ***************************************************************/

/* Move a presumably solid sprite and correct collisions immediately.
 * Returns nonzero if it moved at all, not necessarily as much as you asked for.
 */
int sprite_move(struct sprite *sprite,double dx,double dy);

int sprite_collides_anything(const struct sprite *sprite);

#endif
