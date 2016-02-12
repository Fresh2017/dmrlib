#include "dmr/c.h"
#include "dmr/id.h"
#include "dmr/log.h"
#include "common/byte.h"

DMR_PRV static dmr_idmap *shared = NULL;

DMR_PRV static inline int idcmp(dmr_idnode *a, dmr_idnode *b)
{
    if (a->id > b->id) return -1;
    if (a->id < b->id) return +1;
    return 0;
}

DMR_RB_GENERATE(dmr_idmap, dmr_idnode, entry, idcmp)

DMR_API dmr_idmap *dmr_idmap_new(void)
{
    return dmr_malloc(dmr_idmap);
}

DMR_API void dmr_idmap_free(dmr_idmap *map)
{
    dmr_free(map);
}

DMR_API int dmr_idmap_add(dmr_idmap *map, dmr_id id, const char *name)
{
    DMR_ERROR_IF_NULL(map, DMR_EINVAL);
    DMR_ERROR_IF_NULL(name, DMR_EINVAL);

    dmr_idnode n, *np;
    n.id = id;
    if ((np = DMR_RB_FIND(dmr_idmap, map, &n)) == NULL) {
        DMR_ERROR_IF_NULL(np = dmr_palloc(map, dmr_idnode), DMR_ENOMEM);
        np->id = id;
        DMR_ERROR_IF_NULL(np->name = dmr_strdup(np, name), DMR_ENOMEM);
        DMR_RB_INSERT(dmr_idmap, map, np);
    }

    return 0;
}

DMR_API const char *dmr_idmap_get(dmr_idmap *map, dmr_id id)
{
    DMR_NULL_CHECK(map);

    dmr_idnode n, *np;
    n.id = id;
    if ((np = DMR_RB_FIND(dmr_idmap, map, &n)) != NULL)
        return np->name;

    return NULL;
}

DMR_API int dmr_id_init(void)
{
    if (shared != NULL)
        return 0;

    DMR_ERROR_IF_NULL(shared = dmr_idmap_new(), DMR_ENOMEM);
    return 0;
}

DMR_API void dmr_id_free(void)
{
    if (shared != NULL)
        dmr_free(shared);
    shared = NULL;
}

DMR_API int dmr_id_add(dmr_id id, const char *name)
{
    if (dmr_id_init() != 0)
        return dmr_error(DMR_LASTERROR);

    return dmr_idmap_add(shared, id, name);
}

DMR_API const char *dmr_id_name(dmr_id id)
{
    if (shared == NULL)
        return NULL;

    return dmr_idmap_get(shared, id);
}

DMR_API size_t dmr_id_size(void)
{
    if (shared == NULL)
        return 0;
    
    dmr_idnode *n;
    size_t len = 0;
    DMR_RB_FOREACH(n, dmr_idmap, shared) {
        len++;
    }

    return len;
}
