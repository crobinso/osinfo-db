#include <osinfo.h>

G_DEFINE_TYPE (OsinfoOsList, osinfo_oslist, OSINFO_TYPE_LIST);

#define OSINFO_OSLIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_OSLIST, OsinfoOsListPrivate))

static void osinfo_oslist_finalize (GObject *object);

struct _OsinfoOsListPrivate
{
    int tmp;
};

static void
osinfo_oslist_finalize (GObject *object)
{
    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_oslist_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_oslist_class_init (OsinfoOsListClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_oslist_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoOsListPrivate));
}

static void
osinfo_oslist_init (OsinfoOsList *self)
{
    OsinfoOsListPrivate *priv;
    self->priv = priv = OSINFO_OSLIST_GET_PRIVATE(self);

}

OsinfoOs *osinfoGetOsAtIndex(OsinfoOsList *self, gint idx, GError **err)
{
    if (!OSINFO_IS_OSLIST(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OSLIST);
        return NULL;
    }

    OsinfoList *selfAsList = OSINFO_LIST (self);
    OsinfoEntity *entity = osinfoGetEntityAtIndex(selfAsList, idx);
    return OSINFO_OS (entity);
}

OsinfoOsList *osinfoOsListFilter(OsinfoOsList *self, OsinfoFilter *filter, GError **err)
{
    if (!OSINFO_IS_OSLIST(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OSLIST);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // For each element in self, if passes filter, add to new list.
    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    __osinfoDoFilter(OSINFO_LIST (self), OSINFO_LIST (newList), filter);
    return newList;
}

OsinfoOsList *osinfoOsListIntersect(OsinfoOsList *self, OsinfoOsList *otherList, GError **err)
{
    if (!OSINFO_IS_OSLIST(self) || !OSINFO_IS_OSLIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OSLIST);
        return NULL;
    }

    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
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

OsinfoOsList *osinfoOsListUnion(OsinfoOsList *self, OsinfoOsList *otherList, GError **err)
{
    if (!OSINFO_IS_OSLIST(self) || !OSINFO_IS_OSLIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OSLIST);
        return NULL;
    }

    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
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
