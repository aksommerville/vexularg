#include "vexularg.h"

/* Receive the one tilesheet.
 */
 
static int res_welcome_tilesheet(const void *src,int srcc) {
  struct tilesheet_reader reader;
  if (tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  struct tilesheet_entry entry;
  while (tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue; // Only physics gets recorded.
    memcpy(g.physics+entry.tileid,entry.v,entry.c);
  }
  return 0;
}

/* Receive the one map.
 */
 
static int res_welcome_map(const void *src,int srcc) {
  struct map_res rmap;
  if (map_res_decode(&rmap,src,srcc)<0) return -1;
  g.cellv=rmap.v;
  g.mapw=rmap.w;
  g.maph=rmap.h;
  g.mapcmd=rmap.cmd;
  g.mapcmdc=rmap.cmdc;
  struct cmdlist_reader reader={.v=rmap.cmd,.c=rmap.cmdc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      //TODO Process one-time map commands, eg capture POI that need realtime access.
    }
  }
  return 0;
}

/* Receive one resource.
 * Return >0 to list in the TOC.
 */
 
static int res_welcome(const struct rom_entry *res) {
  switch (res->tid) {
    // Some types get special handling at load time:
    case EGG_TID_map: {
        if (res->rid==1) return res_welcome_map(res->v,res->c);
        fprintf(stderr,"Ignoring unexpected map %d\n",res->rid);
      } return 0;
    case EGG_TID_tilesheet: {
        if (res->rid==RID_image_terrain) return res_welcome_tilesheet(res->v,res->c);
        fprintf(stderr,"Ignoring unexpected tilesheet %d\n",res->rid);
      } return 0;
    // Some times get registered generically for future lookup:
    case EGG_TID_strings:
    case EGG_TID_sprite:
      return 1;
    // Everything else can be ignored.
    default: return 0;
  }
}

/* Load resources.
 */
 
int res_init() {

  if ((g.romc=egg_rom_get(0,0))<0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_rom_get(g.rom,g.romc)!=g.romc) return -1;
  
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    int err=res_welcome(&res);
    if (err<0) return -1;
    if (err>0) { // Record in TOC.
      if (g.resc>=g.resa) {
        int na=g.resa+32;
        if (na>INT_MAX/sizeof(struct rom_entry)) return -1;
        void *nv=realloc(g.resv,sizeof(struct rom_entry)*na);
        if (!nv) return -1;
        g.resv=nv;
        g.resa=na;
      }
      g.resv[g.resc++]=res;
    }
  }
  
  if (!g.cellv||(g.mapw<1)||(g.maph<1)) {
    fprintf(stderr,"Map not loaded.\n");
    return -1;
  }
  if ((g.mapw*NS_sys_tilesize<FBW)||(g.maph*NS_sys_tilesize<FBH)) {
    fprintf(stderr,"Map must cover the %dx%d framebuffer. Have %dx%d of %d-pixel tiles. Make it bigger.\n",FBW,FBH,g.mapw,g.maph,NS_sys_tilesize);
    return -1;
  }
  
  return 0;
}

/* Search TOC.
 */
 
int res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_entry *q=g.resv+ck;
         if (tid<q->tid) hi=ck;
    else if (tid>q->tid) lo=ck+1;
    else if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get TOC'd resource.
 */
 
int res_get(void *dstpp,int tid,int rid) {
  int p=res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_entry *res=g.resv+p;
  if (dstpp) *(const void**)dstpp=res->v;
  return res->c;
}

/* Get string.
 */
 
int res_get_string(void *dstpp,int rid,int strix) {
  if (rid<1) return 0;
  if (strix<1) return 0;
  if (rid<0x40) rid|=egg_prefs_get(EGG_PREF_LANG)<<6;
  const void *serial=0;
  int serialc=res_get(&serial,EGG_TID_strings,rid);
  struct strings_reader reader;
  if (strings_reader_init(&reader,serial,serialc)<0) return 0;
  struct strings_entry entry;
  while (strings_reader_next(&entry,&reader)>0) {
    if (entry.index==strix) {
      if (dstpp) *(const void**)dstpp=entry.v;
      return entry.c;
    }
    if (entry.index>strix) return 0;
  }
  return 0;
}
