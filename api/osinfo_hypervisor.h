/*
 * libosinfo
 *
 * osinfo_hypervisor.h
 * Represents a hypervisor in libosinfo.
 */

#ifndef __OSINFO_HYPERVISOR_H__
#define __OSINFO_HYPERVISOR_H__

#include "osinfo_devicelist.h"

/*
 * Type macros.
 */
#define OSINFO_TYPE_HYPERVISOR                  (osinfo_hypervisor_get_type ())
#define OSINFO_HYPERVISOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_HYPERVISOR, OsinfoHypervisor))
#define OSINFO_IS_HYPERVISOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_HYPERVISOR))
#define OSINFO_HYPERVISOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_HYPERVISOR, OsinfoHypervisorClass))
#define OSINFO_IS_HYPERVISOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_HYPERVISOR))
#define OSINFO_HYPERVISOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_HYPERVISOR, OsinfoHypervisorClass))

//typedef struct _OsinfoHypervisor        OsinfoHypervisor;
// (defined in osinfo_objects.h)

typedef struct _OsinfoHypervisorClass   OsinfoHypervisorClass;

typedef struct _OsinfoHypervisorPrivate OsinfoHypervisorPrivate;

/* object */
struct _OsinfoHypervisor
{
    OsinfoEntity parent_instance;

    /* public */

    /* private */
    OsinfoHypervisorPrivate *priv;
};

/* class */
struct _OsinfoHypervisorClass
{
    OsinfoEntityClass parent_class;

    /* class members */
};

GPtrArray *osinfoGetHypervisorDeviceTypes(OsinfoHypervisor *self, GError **err);
OsinfoDeviceList *osinfoGetHypervisorDevicesByType(OsinfoHypervisor *self, gchar *devType, OsinfoFilter *filter, GError **err);

#endif /* __OSINFO_HYPERVISOR_H__ */
