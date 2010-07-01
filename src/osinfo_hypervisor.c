#include <osinfo.h>

G_DEFINE_TYPE (OsinfoHypervisor, osinfo_hypervisor, OSINFO_TYPE_ENTITY);

#define OSINFO_HYPERVISOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_HYPERVISOR, OsinfoHypervisorPrivate))

static void osinfo_hypervisor_finalize (GObject *object);

static void
osinfo_hypervisor_finalize (GObject *object)
{
    OsinfoHypervisor *self = OSINFO_HYPERVISOR (object);

    g_tree_destroy (self->priv->sections);
    g_tree_destroy (self->priv->sectionsAsList);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_hypervisor_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_hypervisor_class_init (OsinfoHypervisorClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_hypervisor_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoHypervisorPrivate));
}

static void
osinfo_hypervisor_init (OsinfoHypervisor *self)
{
    OsinfoHypervisorPrivate *priv;
    self->priv = priv = OSINFO_HYPERVISOR_GET_PRIVATE(self);

    self->priv->sections = g_tree_new_full(__osinfoStringCompare, NULL, g_free, __osinfoFreeDeviceSection);
    self->priv->sectionsAsList = g_tree_new_full(__osinfoStringCompare, NULL, g_free, __osinfoFreePtrArray);
}

int __osinfoAddDeviceToSectionHv(OsinfoHypervisor *self, gchar *section, gchar *id, gchar *driver)
{
    if( !OSINFO_IS_HYPERVISOR(self) || !section || !id || !driver)
        return -EINVAL;

    return __osinfoAddDeviceToSection(self->priv->sections, self->priv->sectionsAsList, section, id, driver);
}

void __osinfoClearDeviceSectionHv(OsinfoHypervisor *self, gchar *section)
{
    if (!OSINFO_IS_HYPERVISOR(self) || !section)
        return;

    __osinfoClearDeviceSection(self->priv->sections, self->priv->sectionsAsList, section);
}

GPtrArray *osinfoGetHypervisorDeviceTypes(OsinfoHypervisor *self, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_HYPERVISOR(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HV);
        return NULL;
    }

    GPtrArray *deviceTypes = g_ptr_array_sized_new(g_tree_nnodes(self->priv->sections));
    if (!deviceTypes) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    // For each key in our tree of device sections, dup and add to the array
    struct __osinfoPtrArrayErr arrayErr = {deviceTypes, 0};
    g_tree_foreach(self->priv->sections, __osinfoGetKeys, &arrayErr);
    return deviceTypes;
}

OsinfoDeviceList *osinfoGetHypervisorDevicesByType(OsinfoHypervisor *self, gchar *devType, OsinfoFilter *filter, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_HYPERVISOR(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HV);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    if (!devType) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_DEVTYPE);
        return NULL;
    }

    // Create our device list
    OsinfoDeviceList *newList = g_object_new(OSINFO_TYPE_DEVICELIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    // If section does not exist, return empty list
    GPtrArray *sectionList = NULL;
    sectionList = g_tree_lookup(self->priv->sectionsAsList, devType);
    if (!sectionList)
        return newList;

    // For each device in section list, apply filter. If filter passes, add device to list.
    int i;
    struct __osinfoDeviceLink *deviceLink;
    for (i = 0; i < sectionList->len; i++) {
        deviceLink = g_ptr_array_index(sectionList, i);
        if (__osinfoDevicePassesFilter(filter, deviceLink->dev))
            __osinfoListAdd(OSINFO_LIST (newList), OSINFO_ENTITY (deviceLink->dev));
    }

    return newList;
}
