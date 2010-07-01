/*
 * libosinfo
 *
 * osinfo_db.h
 * Represents the main entry point to data contained by libosinfo.
 */

#ifndef __OSINFO_DB_H__
#define __OSINFO_DB_H__

#include "osinfo_devicelist.h"
#include "osinfo_hypervisorlist.h"
#include "osinfo_oslist.h"

/*
 * Type macros.
 */
#define OSINFO_TYPE_DB                  (osinfo_db_get_type ())
#define OSINFO_DB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_DB, OsinfoDb))
#define OSINFO_IS_DB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_DB))
#define OSINFO_DB_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_DB, OsinfoDbClass))
#define OSINFO_IS_DB_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_DB))
#define OSINFO_DB_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_DB, OsinfoDbClass))

//typedef struct _OsinfoDb        OsinfoDb;
// (defined in osinfo_objects.h)

typedef struct _OsinfoDbClass   OsinfoDbClass;

typedef struct _OsinfoDbPrivate OsinfoDbPrivate;

/*
 * To get a db handle, we construct one with a construct-time only
 * backing data directory. It is already considered to be initialized
 * on return from the constructor, and ready to do work.
 *
 * To close it, we call the destructor on it.
 * Setting parameters on it will work if it's not a construct-time only
 * parameter. Reading will always work. Currently the backing directory and
 * libvirt version are the only parameters.
 *
 * The db object contains information related to three main classes of
 * objects: hypervisors, operating systems and devices.
 */

/* object */
struct _OsinfoDb
{
    GObject parent_instance;

    /* public */

    /* private */
    OsinfoDbPrivate *priv;
};

/* class */
struct _OsinfoDbClass
{
    GObjectClass parent_class;

    /* class members */
};

int osinfoInitializeDb(OsinfoDb *self, GError **err);

OsinfoHypervisor *osinfoGetHypervisorById(OsinfoDb *self, gchar *hvId, GError **err);
OsinfoDevice *osinfoGetDeviceById(OsinfoDb *self, gchar *devId, GError **err);
OsinfoOs *osinfoGetOsById(OsinfoDb *self, gchar *osId, GError **err);

OsinfoOsList *osinfoGetOsList(OsinfoDb *self, OsinfoFilter *filter, GError **err);
OsinfoHypervisorList *osinfoGetHypervisorList(OsinfoDb *self, OsinfoFilter *filter, GError **err);
OsinfoDeviceList *osinfoGetDeviceList(OsinfoDb *self, OsinfoFilter *filter, GError **err);

// Get me all unique values for property "vendor" among operating systems
GPtrArray *osinfoUniqueValuesForPropertyInOs(OsinfoDb *self, gchar *propName, GError **err);

// Get me all unique values for property "vendor" among hypervisors
GPtrArray *osinfoUniqueValuesForPropertyInHv(OsinfoDb *self, gchar *propName, GError **err);

// Get me all unique values for property "vendor" among devices
GPtrArray *osinfoUniqueValuesForPropertyInDev(OsinfoDb *self, gchar *propName, GError **err);

// Get me all OSes that 'upgrade' another OS (or whatever relationship is specified)
OsinfoOsList *osinfoUniqueValuesForOsRelationship(OsinfoDb *self, osinfoRelationship relshp, GError **err);

#endif /* __OSINFO_DB_H__ */
