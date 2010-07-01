#include <osinfo.h>

G_DEFINE_TYPE (OsinfoList, osinfo_list, G_TYPE_OBJECT);

#define OSINFO_LIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_LIST, OsinfoListPrivate))

static void osinfo_list_finalize (GObject *object);

struct _OsinfoListPrivate
{
    GPtrArray *array;
};

static void
osinfo_list_finalize (GObject *object)
{
    OsinfoList *self = OSINFO_LIST (object);

    g_ptr_array_free(self->priv->array, TRUE);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_list_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_list_class_init (OsinfoListClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_list_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoListPrivate));
}

static void
osinfo_list_init (OsinfoList *self)
{
    OsinfoListPrivate *priv;
    self->priv = priv = OSINFO_LIST_GET_PRIVATE(self);

    self->priv->array = g_ptr_array_new();
}

void osinfoFreeList(OsinfoList *self)
{
    g_object_unref(self);
}

gint osinfoListLength(OsinfoList *self)
{
    return self->priv->array->len;
}

OsinfoEntity *osinfoGetEntityAtIndex(OsinfoList *self, gint idx)
{
    return g_ptr_array_index(self->priv->array, idx);
}

void __osinfoListAdd(OsinfoList *self, OsinfoEntity *entity)
{
    g_ptr_array_add(self->priv->array, entity);
}

void __osinfoDoFilter(OsinfoList *src, OsinfoList *dst, OsinfoFilter *filter)
{
    int i, len;
    len = osinfoListLength(src);
    for (i = 0; i < len; i++) {
        OsinfoEntity *entity = osinfoGetEntityAtIndex(src, i);
        if (__osinfoEntityPassesFilter(filter, entity))
            __osinfoListAdd(dst, entity);
    }
}

OsinfoList *osinfoListFilter(OsinfoList *self, OsinfoFilter *filter, GError **err)
{
    if (!OSINFO_IS_LIST(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_LIST);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // For each element in self, if passes filter, add to new list.
    OsinfoList *newList = g_object_new(OSINFO_TYPE_LIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    __osinfoDoFilter(self, newList, filter);
    return newList;
}

int __osinfoDoIntersect(OsinfoList *src1, OsinfoList *src2, OsinfoList *dst)
{
    int i, len;

    // Make set representation of otherList and newList
    GTree *otherSet = g_tree_new(__osinfoStringCompareBase);
    if (!otherSet)
        return -ENOMEM;

    GTree *newSet = g_tree_new(__osinfoStringCompareBase);
    if (!newSet) {
        g_tree_destroy(otherSet);
        return -ENOMEM;
    }

    // Add all from otherList to otherSet
    len = osinfoListLength(src2);
    for (i = 0; i < len; i++) {
        OsinfoEntity *entity = osinfoGetEntityAtIndex(src2, i);
        gchar *id = entity->priv->id;
        g_tree_insert(otherSet, id, entity);
    }

    // If other contains entity, and new list does not, add to new list
    len = osinfoListLength(src1);
    for (i = 0; i < len; i++) {
        OsinfoEntity *entity = osinfoGetEntityAtIndex(src1, i);
        gchar *id = entity->priv->id;

        if (g_tree_lookup(otherSet, entity->priv->id) &&
            !g_tree_lookup(newSet, entity->priv->id)) {
            g_tree_insert(newSet, id, entity);
            __osinfoListAdd(dst, entity);
        }
    }

    g_tree_destroy(otherSet);
    g_tree_destroy(newSet);
    return 0;
}

OsinfoList *osinfoListIntersect(OsinfoList *self, OsinfoList *otherList, GError **err)
{
    if (!OSINFO_IS_LIST(self) || !OSINFO_IS_LIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_LIST);
        return NULL;
    }

    OsinfoList *newList = g_object_new(OSINFO_TYPE_LIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    int ret;

    ret = __osinfoDoIntersect(self, otherList, newList);
    if (ret != 0) {
        g_object_unref(newList);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, __osinfoErrorToString(ret));
        return NULL;
    }

    return newList;
}

int __osinfoDoUnion(OsinfoList *src1, OsinfoList *src2, OsinfoList *dst)
{
    // Make set version of new list
    GTree *newSet = g_tree_new(__osinfoStringCompareBase);
    if (!newSet)
        return -ENOMEM;

    // Add all from other list to new list
    int i, len;
    len = osinfoListLength(src2);
    for (i = 0; i < len; i++) {
        OsinfoEntity *entity = osinfoGetEntityAtIndex(src2, i);
        gchar *id = entity->priv->id;
        __osinfoListAdd(dst, entity);
        g_tree_insert(newSet, id, entity);
    }

    // Add remaining elements from this list to new list
    len = osinfoListLength(src1);
    for (i = 0; i < len; i++) {
        OsinfoEntity *entity = osinfoGetEntityAtIndex(src1, i);
        gchar *id = entity->priv->id;
        // If new list does not contain element, add to new list
        if (!g_tree_lookup(newSet, id)) {
            __osinfoListAdd(dst, entity);
            g_tree_insert(newSet, id, entity);
        }
    }

    g_tree_destroy(newSet);
    return 0;
}

OsinfoList *osinfoListUnion(OsinfoList *self, OsinfoList *otherList, GError **err)
{
    if (!OSINFO_IS_LIST(self) || !OSINFO_IS_LIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_LIST);
        return NULL;
    }

    OsinfoList *newList = g_object_new(OSINFO_TYPE_LIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    int ret;
    ret = __osinfoDoUnion(self, otherList, newList);
    if (ret != 0) {
        g_object_unref(newList);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, __osinfoErrorToString(ret));
        return NULL;
    }

    return newList;
}
