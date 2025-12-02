#include "game/vexularg.h"

/* Unlist.
 */
 
void sprite_unlist(struct sprite *sprite) {
  if (sprite==g.hero) g.hero=0;
  int i=g.spritec;
  struct sprite **qp=g.spritev+i-1;
  for (;i-->0;qp--) {
    struct sprite *q=*qp;
    if (q==sprite) {
      g.spritec--;
      memmove(qp,qp+1,sizeof(void*)*(g.spritec-i));
      return;
    }
  }
}

/* List.
 */
 
int sprite_list(struct sprite *sprite) {
  if (!sprite) return -1;
  if (g.spritec>=g.spritea) {
    int na=g.spritea+32;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) return 0;
    g.spritev=nv;
    g.spritea=na;
  }
  g.spritev[g.spritec++]=sprite;
  if (!g.hero&&(sprite->type==&sprite_type_hero)) g.hero=sprite;
  return 0;
}

/* Delete and unlist.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  sprite_unlist(sprite);
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}
 
void sprite_del_unlisted(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* Allocate, initialize. Do not list.
 */
  
struct sprite *sprite_new(
  const struct sprite_type *type,
  double x,double y,
  uint32_t arg,
  int rid,const void *cmd,int cmdc
) {

  // Acquire (cmd) from (rid) if needed.
  if (!cmd&&rid) {
    cmdc=res_get(&cmd,EGG_TID_sprite,rid);
  }
  
  // Acquire (type) from (cmd) if needed. Abort if we don't end up with one.
  if (!type&&cmd) {
    type=sprite_type_from_id(sprite_res_u16(cmd,cmdc,CMD_sprite_type));
  }
  if (!type) return 0;
  
  if (type->objlen<(int)sizeof(struct sprite)) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->type=type;
  sprite->x=x;
  sprite->y=y;
  sprite->arg=arg;
  sprite->rid=rid;
  sprite->cmd=cmd;
  sprite->cmdc=cmdc;
  sprite->layer=100;
  
  // Run commands generically.
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,cmd,cmdc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_sprite_solid: sprite->solid=1; break;
        case CMD_sprite_tile: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
        case CMD_sprite_layer: sprite->layer=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      }
    }
  }
  
  // And lastly, let the type initialize it.
  if (type->init) {
    if (type->init(sprite)<0) {
      sprite_del(sprite);
      return 0;
    }
  }
    
  return sprite;
}

/* Create and list.
 */

struct sprite *sprite_spawn(
  const struct sprite_type *type,
  double x,double y,
  uint32_t arg,
  int rid,const void *cmd,int cmdc
) {
  struct sprite *sprite=sprite_new(type,x,y,arg,rid,cmd,cmdc);
  if (!sprite) return 0;
  if (sprite_list(sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  return sprite;
}

/* Single-field readers for resource.
 */
 
int sprite_res_cmd(void *dstpp,const void *cmd,int cmdc,int opcode) {
  if ((opcode<1)||(opcode>0xff)) return 0;
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,cmd,cmdc)<0) return 0;
  struct cmdlist_entry entry;
  while (cmdlist_reader_next(&entry,&reader)>0) {
    if (entry.opcode!=opcode) continue;
    *(const void**)dstpp=entry.arg;
    return entry.argc;
  }
  return 0;
}

int sprite_res_u16(const void *cmd,int cmdc,int opcode) {
  const uint8_t *arg=0;
  if (sprite_res_cmd(&arg,cmd,cmdc,opcode)>=2) {
    return (arg[0]<<8)|arg[1];
  }
  return 0;
}

/* Type by id.
 */
 
const struct sprite_type *sprite_type_from_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}
