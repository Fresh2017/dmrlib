/**
 * @file
 * @brief  ...
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_ID_H
#define _DMR_ID_H

#include <stdbool.h>
#include <dmr/malloc.h>
#include <dmr/packet.h>
#include <dmr/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DMR_IDMAP_HEIGHT 64

typedef struct dmr_idnode dmr_idnode;

struct dmr_idnode {
    dmr_id                   id;     /* user id */
    const char               *name;  /* user name */
    /* private */
    DMR_RB_ENTRY(dmr_idnode) entry;
};

typedef struct dmr_idmap dmr_idmap;
DMR_RB_HEAD(dmr_idmap, dmr_idnode);

/** Initialize a new idmap. */
extern dmr_idmap *dmr_idmap_new(void);

/** Free an idmap. */
extern void dmr_idmap_free(dmr_idmap *map);

/** Add to an idmap. */
extern int dmr_idmap_add(dmr_idmap *map, dmr_id id, const char *name);

/** Query an idmap. */
extern const char *dmr_idmap_get(dmr_idmap *map, dmr_id id);

/** Initialize the global (shared) idmap. */
extern int dmr_id_init(void);

/** Free the global (shared) idmap. */
extern void dmr_id_free(void);

/** Add to the global (shared) idmap. */
extern int dmr_id_add(dmr_id id, const char *name);

/** Query the global (shared) idmap. */
extern const char *dmr_id_name(dmr_id id);

/** Query the size of the global (shared) idmap. */
extern size_t dmr_id_size(void);

#ifdef __cplusplus
}
#endif

#endif // _DMR_ID_H
