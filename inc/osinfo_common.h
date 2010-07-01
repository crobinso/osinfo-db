/*
 * libosinfo
 *
 * osinfo_objects.h
 * Contains forward declarations of our main object types, and definitions
 * of our internal data structures not exposed in the API as libosinfo objects.
 */

#ifndef __OSINFO_OBJECTS_H__
#define __OSINFO_OBJECTS_H__

#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include <errno.h>
#include "osinfo_entity.h"

#define FREE_N(x) free(x); x = null;

typedef struct _OsinfoDb         OsinfoDb;
typedef struct _OsinfoDevice     OsinfoDevice;
typedef struct _OsinfoHypervisor OsinfoHypervisor;
typedef struct _OsinfoOs         OsinfoOs;
typedef struct _OsinfoFilter     OsinfoFilter;
typedef struct _OsinfoList       OsinfoList;

typedef enum OSI_RELATIONSHIP {
    RELATIONSHIP_MIN = 0,
    DERIVES_FROM,
    UPGRADES,
    CLONES,
    RELATIONSHIP_MAX
} osinfoRelationship;


/** ****************************************************************************
 * Internal data structures
 ******************************************************************************/

struct __osinfoDeviceLink {
    OsinfoDevice *dev;
    gchar *driver;
};

struct __osinfoHvSection {
    OsinfoHypervisor *hv;
    OsinfoOs *os;

    GTree *sections; // Mapping GString key (device type) to GTree of deviceLink structs
    GTree *sectionsAsList; // Mapping GString key (device type) to Array of deviceLink structs
};

struct __osinfoOsLink {
    /* <subject_os> 'verbs' <direct_object_os>
     * fedora11 upgrades fedora10
     * centos clones rhel
     * scientificlinux derives from rhel
     */
    OsinfoOs *subjectOs;
    osinfoRelationship verb;
    OsinfoOs *directObjectOs;
};

struct __osinfoPtrArrayErr {
    GPtrArray *array;
    int err;
};

struct __osinfoPopulateListArgs {
    GError **err;
    int errcode;
    OsinfoFilter *filter;
    OsinfoList *list;
};

struct __osinfoPopulateValuesArgs {
    GError **err;
    int errcode;
    GTree *values;
    gchar *property;
};

struct __osinfoOsCheckRelationshipArgs {
    OsinfoList *list;
    int errcode;
    GError **err;
    osinfoRelationship relshp;
};

struct __osinfoFilterPassArgs {
    int passed;
    OsinfoEntity *entity;
};

/** ****************************************************************************
 *      Convenience methods
 ******************************************************************************/

gint __osinfoIntCompareBase(gconstpointer a,
                            gconstpointer b);
gint __osinfoIntCompare(gconstpointer a,
                        gconstpointer b,
                        gpointer data);
gint __osinfoStringCompareBase(gconstpointer a,
                               gconstpointer b);
gint __osinfoStringCompare(gconstpointer a,
                           gconstpointer b,
                           gpointer data);

void __osinfoFreePtrArray(gpointer ptrarray);
void __osinfoFreeRelationship(gpointer ptrarray);
void __osinfoFreeParamVals(gpointer ptrarray);
void __osinfoFreeDeviceSection(gpointer tree);
void __osinfoFreeHvSection(gpointer hvSection);
void __osinfoFreeDeviceLink(gpointer ptr);
void __osinfoFreeOsLink(gpointer ptr);

void __osinfoListAdd(OsinfoList *self, OsinfoEntity *entity);

int __osinfoAddDeviceToSection(GTree *allSections, GTree *allSectionsAsList, gchar *sectionName, gchar *id, gchar *driver);
void __osinfoClearDeviceSection(GTree *allSections, GTree *allSectionsAsList, gchar *section);

gboolean __osinfoGetKeys(gpointer key, gpointer value, gpointer data);
void __osinfoDupArray(gpointer data, gpointer user_data);

int __osinfoEntityPassesFilter(OsinfoFilter *filter, OsinfoEntity *device);
int __osinfoDevicePassesFilter(OsinfoFilter *filter, OsinfoDevice *device);
int __osinfoOsPassesFilter(OsinfoFilter *filter, OsinfoOs *device);
int __osinfoHypervisorPassesFilter(OsinfoFilter *filter, OsinfoHypervisor *device);

int __osinfoCheckGErrorParamValid(GError **err);
int __osinfoCheckRelationshipValid(osinfoRelationship relshp);

/** ****************************************************************************
 *      Private structures for objects
 ******************************************************************************/

struct _OsinfoFilterPrivate
{
    // Key: Constraint name
    // Value: Array of constraint values
    GTree *propertyConstraints;

    // Key: relationship type
    // Value: Array of OsinfoOs *
    // Note: Only used when filtering OsinfoOs objects
    GTree *relationshipConstraints;
};

struct _OsinfoDbPrivate
{
    int ready;

    gchar *backing_dir;
    gchar *libvirt_ver;

    GTree *devices;
    GTree *hypervisors;
    GTree *oses;

    GError *error;
};

struct _OsinfoDevicePrivate
{
    int tmp;
};

struct _OsinfoHypervisorPrivate
{
    // Key: gchar* (device type)
    // Value: Tree of device_link structs (multiple devices per type)
    GTree *sections;
    GTree *sectionsAsList; // Mapping GString key (device type) to Array of deviceLink structs
};

struct _OsinfoOsPrivate
{
    // OS-Hypervisor specific information
    // Key: gchar* (hypervisor id)
    // Value: hv_section struct
    GTree *hypervisors;

    // Key: gchar* (device type)
    // Value: Tree of device_link structs (multiple devices per type)
    GTree *sections;
    GTree *sectionsAsList; // Mapping GString key (device type) to Array of deviceLink structs

    // OS-OS relationships
    // Key: gchar* (other os id)
    // Value: Array of os_link structs
    GTree *relationshipsByOs;
    // Key: relationship type
    // Value: Array of os_link structs
    GTree *relationshipsByType;
};

struct _OsinfoEntityPrivate
{
    gchar *id;

    // Key: gchar*
    // Value: Array of gchar* values for key (multiple values allowed)
    GTree *params;

    OsinfoDb *db; // Backpointer to db object
};

/** ****************************************************************************
 *      Error strings
 ******************************************************************************/

gchar *__osinfoErrorToString(int err);

#define OSINFO_OTHER "An unspecified error occured"
#define OSINFO_OBJ_NOT_DB "Object is wrong type (expected OsInfoDb)"
#define OSINFO_OBJ_NOT_ENTITY "Object is wrong type (expected OsInfoEntity)"
#define OSINFO_OBJ_NOT_DEVICE "Object is wrong type (expected OsInfoDevice)"
#define OSINFO_OBJ_NOT_OS "Object is wrong type (expected OsInfoOs)"
#define OSINFO_OBJ_NOT_HV "Object is wrong type (expected OsInfoHypervisor)"
#define OSINFO_OBJ_NOT_LIST "Object is wrong type (expected OsInfoList)"
#define OSINFO_OBJ_NOT_HYPERVISORLIST "Object is wrong type (expected OsInfoHypervisorList)"
#define OSINFO_OBJ_NOT_DEVICELIST "Object is wrong type (expected OsInfoDeviceList)"
#define OSINFO_OBJ_NOT_OSLIST "Object is wrong type (expected OsInfoOsList)"
#define OSINFO_OBJ_NOT_FILTER "Object is wrong type (expected OsInfoFilter)"
#define OSINFO_NO_ID "No object id specified"
#define OSINFO_NO_DEVTYPE "No device type specified"
#define OSINFO_NO_MEM "Not enough memory to complete operation"
#define OSINFO_NO_PROPNAME "No property specified"
#define OSINFO_NO_PROPVAL "No value specified for property"
#define OSINFO_INVALID_RELATIONSHIP "Invalid relationship specified"
#endif /* __OSINFO_OBJECTS_H__ */

/** ****************************************************************************
 *      Private Methods
 ******************************************************************************/

void __osinfoAddDeviceToDb(OsinfoDb *db, OsinfoDevice *dev);
void __osinfoAddHypervisorToDb(OsinfoDb *db, OsinfoHypervisor *hv);
void __osinfoAddOsToDb(OsinfoDb *db, OsinfoOs *os);

// Private
int __osinfoAddDeviceToSectionOs(OsinfoOs *self, gchar *section, gchar *id, gchar *driver);
void __osinfoClearDeviceSectionOs(OsinfoOs *self, gchar *section);

int __osinfoAddOsRelationship (OsinfoOs *self, gchar *otherOsId, osinfoRelationship rel);
void __osinfoClearOsRelationships (OsinfoOs *self, gchar *otherOsId);

struct __osinfoHvSection *__osinfoAddHypervisorSectionToOs(OsinfoOs *self, gchar *hvId);
void __osinfoRemoveHvSectionFromOs(OsinfoOs *self, gchar *hvId);

// Private
int __osinfoAddDeviceToSectionHv(OsinfoHypervisor *self, gchar *section, gchar *id, gchar *driver);
void __osinfoClearDeviceSectionHv(OsinfoHypervisor *self, gchar *section);

// Private
int __osinfoAddParam(OsinfoEntity *self, gchar *key, gchar *value);
void __osinfoClearParam(OsinfoEntity *self, gchar *key);
