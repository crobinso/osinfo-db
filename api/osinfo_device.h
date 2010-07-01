/*
 * libosinfo
 *
 * osinfo_device.h
 * Represents a device in libosinfo.
 */

#ifndef __OSINFO_DEVICE_H__
#define __OSINFO_DEVICE_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_DEVICE                  (osinfo_device_get_type ())
#define OSINFO_DEVICE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_DEVICE, OsinfoDevice))
#define OSINFO_IS_DEVICE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_DEVICE))
#define OSINFO_DEVICE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_DEVICE, OsinfoDeviceClass))
#define OSINFO_IS_DEVICE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_DEVICE))
#define OSINFO_DEVICE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_DEVICE, OsinfoDeviceClass))

//typedef struct _OsinfoDevice        OsinfoDevice;
// (defined in osinfo_objects.h)

typedef struct _OsinfoDeviceClass   OsinfoDeviceClass;

typedef struct _OsinfoDevicePrivate OsinfoDevicePrivate;

/* object */
struct _OsinfoDevice
{
    OsinfoEntity parent_instance;

    /* public */

    /* private */
    OsinfoDevicePrivate *priv;
};

/* class */
struct _OsinfoDeviceClass
{
    OsinfoEntityClass parent_class;

    /* class members */
};

gchar *osinfoGetDeviceDriver(OsinfoDevice *self, gchar *devType, OsinfoOs *os, OsinfoHypervisor *hv, GError **err);

#endif /* __OSINFO_DEVICE_H__ */
