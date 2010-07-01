#include <osinfo.h>

G_DEFINE_TYPE (OsinfoDevice, osinfo_device, OSINFO_TYPE_ENTITY);

#define OSINFO_DEVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_DEVICE, OsinfoDevicePrivate))

static void osinfo_device_finalize (GObject *object);

static void
osinfo_device_finalize (GObject *object)
{
    OsinfoDevice *self = OSINFO_DEVICE (object);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_device_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_device_class_init (OsinfoDeviceClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_device_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoDevicePrivate));
}

static void
osinfo_device_init (OsinfoDevice *self)
{
    OsinfoDevicePrivate *priv;
    self->priv = priv = OSINFO_DEVICE_GET_PRIVATE(self);
}

gchar *osinfoGetDeviceDriver(OsinfoDevice *self,
                             gchar *devType,
                             OsinfoOs *os,
                             OsinfoHypervisor *hv,
                             GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DEVICE(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DEVICE);
        return NULL;
    }

    if (!OSINFO_IS_OS(os)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OS);
        return NULL;
    }

    if (!OSINFO_IS_HYPERVISOR(hv)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HV);
        return NULL;
    }

    if (!devType) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_DEVTYPE);
        return NULL;
    }

    gchar *driver = NULL;

    // For os, get hypervisor specific info. If not present, return NULL.
    struct __osinfoHvSection *hvSection = NULL;
    hvSection = g_tree_lookup(os->priv->hypervisors, (OSINFO_ENTITY(hv))->priv->id);
    if (!hvSection)
        return NULL;

    // Check for info for type of devices in <os,hv>. If not found, return NULL.
    GTree *section = NULL;
    section = g_tree_lookup(hvSection->sections, devType);
    if (!section)
        return NULL;

    // Check device section for device. If not found, return NULL.
    struct __osinfoDeviceLink *deviceLink = NULL;
    deviceLink = g_tree_lookup(section, (OSINFO_ENTITY(self))->priv->id);
    if (!deviceLink)
        return NULL;

    if (!deviceLink->driver)
        return NULL;

    driver = g_strdup(deviceLink->driver);
    if (!driver) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    return driver;
}
