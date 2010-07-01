#include <osinfo.h>

G_DEFINE_TYPE (OsinfoHypervisorList, osinfo_hypervisorlist, OSINFO_TYPE_LIST);

#define OSINFO_HYPERVISORLIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_HYPERVISORLIST, OsinfoHypervisorListPrivate))

static void osinfo_hypervisorlist_finalize (GObject *object);

struct _OsinfoHypervisorListPrivate
{
    int tmp;
};

static void
osinfo_hypervisorlist_finalize (GObject *object)
{
    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_hypervisorlist_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_hypervisorlist_class_init (OsinfoHypervisorListClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_hypervisorlist_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoHypervisorListPrivate));
}

static void
osinfo_hypervisorlist_init (OsinfoHypervisorList *self)
{
    OsinfoHypervisorListPrivate *priv;
    self->priv = priv = OSINFO_HYPERVISORLIST_GET_PRIVATE(self);

}

OsinfoHypervisor *osinfoGetHypervisorAtIndex(OsinfoHypervisorList *self, gint idx, GError **err)
{
    if (!OSINFO_IS_HYPERVISORLIST(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HYPERVISORLIST);
        return NULL;
    }

    OsinfoList *selfAsList = OSINFO_LIST (self);
    OsinfoEntity *entity = osinfoGetEntityAtIndex(selfAsList, idx);
    return OSINFO_HYPERVISOR (entity);
}

OsinfoHypervisorList *osinfoHypervisorListFilter(OsinfoHypervisorList *self, OsinfoFilter *filter, GError **err)
{
    if (!OSINFO_IS_HYPERVISORLIST(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HYPERVISORLIST);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // For each element in self, if passes filter, add to new list.
    OsinfoHypervisorList *newList = g_object_new(OSINFO_TYPE_HYPERVISORLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    __osinfoDoFilter(OSINFO_LIST (self), OSINFO_LIST (newList), filter);
    return newList;
}

OsinfoHypervisorList *osinfoHypervisorListIntersect(OsinfoHypervisorList *self, OsinfoHypervisorList *otherList, GError **err)
{
    if (!OSINFO_IS_HYPERVISORLIST(self) || !OSINFO_IS_HYPERVISORLIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HYPERVISORLIST);
        return NULL;
    }

    OsinfoHypervisorList *newList = g_object_new(OSINFO_TYPE_HYPERVISORLIST, NULL);
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

OsinfoHypervisorList *osinfoHypervisorListUnion(OsinfoHypervisorList *self, OsinfoHypervisorList *otherList, GError **err)
{
    if (!OSINFO_IS_HYPERVISORLIST(self) || !OSINFO_IS_HYPERVISORLIST(otherList)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HYPERVISORLIST);
        return NULL;
    }

    OsinfoHypervisorList *newList = g_object_new(OSINFO_TYPE_HYPERVISORLIST, NULL);
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
