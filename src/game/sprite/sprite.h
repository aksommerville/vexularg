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
  double x,y; // World meters.
  uint8_t tileid,xform;
  int rid;
  uint32_t arg;
  const void *cmd; // Entire sprite resource, including signature.
  int cmdc;
  int solid;
};

/* Primitive delete and initialize.
 * Don't use these from outside. Prefer "defunct" and "spawn".
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(
  const struct sprite_type *type,
  double x,double y,
  uint32_t arg,
  int rid,const void *cmd,int cmdc
);

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

/* Physics. TODO
 ***************************************************************/

#endif
