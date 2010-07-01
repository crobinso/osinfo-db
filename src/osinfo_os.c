#include <osinfo.h>

G_DEFINE_TYPE (OsinfoOs, osinfo_os, OSINFO_TYPE_ENTITY);

#define OSINFO_OS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_OS, OsinfoOsPrivate))

static void osinfo_os_finalize (GObject *object);

static void
osinfo_os_finalize (GObject *object)
{
    OsinfoOs *self = OSINFO_OS (object);

    g_tree_destroy (self->priv->sections);
    g_tree_destroy (self->priv->sectionsAsList);
    g_tree_destroy (self->priv->hypervisors);
    g_tree_destroy (self->priv->relationshipsByOs);
    g_tree_destroy (self->priv->relationshipsByType);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_os_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_os_class_init (OsinfoOsClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_os_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoOsPrivate));
}

static void
osinfo_os_init (OsinfoOs *self)
{
    OsinfoOsPrivate *priv;
    self->priv = priv = OSINFO_OS_GET_PRIVATE(self);

    self->priv->sections = g_tree_new_full(__osinfoStringCompare,
                                           NULL,
                                           g_free,
                                           __osinfoFreeDeviceSection);

    self->priv->sectionsAsList = g_tree_new_full(__osinfoStringCompare,
                                                 NULL,
                                                 g_free,
                                                 __osinfoFreePtrArray);

    self->priv->relationshipsByOs = g_tree_new_full(__osinfoStringCompare,
                                                NULL,
                                                g_free,
                                                __osinfoFreeRelationship);
    self->priv->relationshipsByType = g_tree_new(__osinfoIntCompareBase);

    self->priv->hypervisors = g_tree_new_full(__osinfoStringCompare,
                                              NULL,
                                              g_free,
                                              __osinfoFreeHvSection);
}

static int __osinfoAddOsRelationshipByOs(OsinfoOs *self,
                                         gchar *otherOsId,
                                         osinfoRelationship rel,
                                         struct __osinfoOsLink *osLink)
{
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *relationshipsForOs;
    gchar *otherOsIdDup = NULL;

    found = g_tree_lookup_extended(self->priv->relationshipsByOs, otherOsId, &origKey, &foundValue);
    if (!found) {
        otherOsIdDup = g_strdup(otherOsId);
        relationshipsForOs = g_ptr_array_new_with_free_func(__osinfoFreeOsLink);

        if (!relationshipsForOs)
            return -ENOMEM;
        if (!otherOsIdDup) {
            g_ptr_array_free(relationshipsForOs, TRUE);
            return -ENOMEM;
        }
        g_tree_insert(self->priv->relationshipsByOs, otherOsIdDup, relationshipsForOs);
    }
    else
        relationshipsForOs = (GPtrArray *) foundValue;

    g_ptr_array_add(relationshipsForOs, osLink);
    return 0;
}

static int __osinfoAddOsRelationshipByType(OsinfoOs *self,
                                           osinfoRelationship relshp,
                                           struct __osinfoOsLink *osLink)
{
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *relationshipsForType;

    found = g_tree_lookup_extended(self->priv->relationshipsByType, (gpointer) relshp, &origKey, &foundValue);
    if (!found) {
        relationshipsForType = g_ptr_array_new();
        if (!relationshipsForType)
            return -ENOMEM;

        g_tree_insert(self->priv->relationshipsByType, (gpointer) relshp, relationshipsForType);
    }
    else
        relationshipsForType = (GPtrArray *) foundValue;

    g_ptr_array_add(relationshipsForType, osLink);
    return 0;
}

static void __osinfoRemoveOsLink(OsinfoOs *self,
                                 gchar *otherOsId,
                                 osinfoRelationship relshp,
                                 struct __osinfoOsLink *osLink)
{
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *relationshipsForOs;
    GPtrArray *relationshipsForType;

    // First from by-os list
    found = g_tree_lookup_extended(self->priv->relationshipsByOs, otherOsId, &origKey, &foundValue);
    if (found) {
        relationshipsForOs = (GPtrArray *) foundValue;
        g_ptr_array_remove(relationshipsForOs, osLink);
    }

    // Now from by-relshp list
    found = g_tree_lookup_extended(self->priv->relationshipsByType, (gpointer) relshp, &origKey, &foundValue);
    if (found) {
        relationshipsForType = (GPtrArray *) foundValue;
        g_ptr_array_remove(relationshipsForType, osLink);
    }
}

int __osinfoAddOsRelationship (OsinfoOs *self, gchar *otherOsId, osinfoRelationship rel)
{
    if ( !OSINFO_IS_OS(self) || !otherOsId)
        return -EINVAL;

    struct __osinfoOsLink *osLink = NULL;
    osLink = g_malloc(sizeof(*osLink));
    if (!osLink)
        return -ENOMEM;

    osLink->subjectOs = self;
    osLink->verb = rel;

    int ret;
    ret = __osinfoAddOsRelationshipByOs(self, otherOsId, rel, osLink);
    if (ret != 0)
        goto error_free;

    ret = __osinfoAddOsRelationshipByType(self, rel, osLink);
    if (ret != 0)
        goto error_cleanup;

    return ret;

error_cleanup:
    __osinfoRemoveOsLink(self, otherOsId, rel, osLink);
error_free:
    g_free(osLink);
    return ret;
}

int __osinfoAddDeviceToSectionOs(OsinfoOs *self, gchar *section, gchar *id, gchar *driver)
{
    if( !OSINFO_IS_OS(self) || !section || !id || !driver)
        return -EINVAL;

    return __osinfoAddDeviceToSection(self->priv->sections, self->priv->sectionsAsList, section, id, driver);
}

void __osinfoClearDeviceSectionOs(OsinfoOs *self, gchar *section)
{
    if (!OSINFO_IS_OS(self) || !section)
        return;

    __osinfoClearDeviceSection(self->priv->sections, self->priv->sectionsAsList, section);
}

struct __osinfoHvSection *__osinfoAddHypervisorSectionToOs(OsinfoOs *self, gchar *hvId)
{
    if (!OSINFO_IS_OS(self) || !hvId)
        return NULL;

    gboolean found;
    gpointer origKey, foundValue;
    struct __osinfoHvSection *hvSection = NULL;
    gchar *hvIdDup = NULL;
    GTree *deviceSections;
    GTree *deviceSectionsAsList;

    found = g_tree_lookup_extended(self->priv->hypervisors, hvId, &origKey, &foundValue);
    if (!found) {
        hvSection = g_malloc(sizeof(*hvSection));
        hvIdDup = g_strdup(hvId);
        deviceSections = g_tree_new_full(__osinfoStringCompare,
                                        NULL,
                                        g_free,
                                        __osinfoFreeDeviceSection);

        if (!deviceSections)
            goto error_free;

        deviceSectionsAsList = g_tree_new_full(__osinfoStringCompare,
                                               NULL,
                                               g_free,
                                               __osinfoFreePtrArray);
        if (!deviceSectionsAsList) {
            g_tree_destroy(deviceSections);
            goto error_free;
        }

        if (!hvSection || !hvIdDup) {
            g_tree_destroy(deviceSectionsAsList);
            g_tree_destroy(deviceSections);
            goto error_free;
        }

        hvSection->os = self;
        // Will set hv link later
        hvSection->sections = deviceSections;
        hvSection->sectionsAsList = deviceSectionsAsList;

        g_tree_insert(self->priv->hypervisors, hvIdDup, hvSection);
        return hvSection;
    }
    else
        return (struct __osinfoHvSection *) foundValue;

error_free:
    g_free(hvSection);
    g_free(hvIdDup);
    return NULL;
}

void __osinfoRemoveHvSectionFromOs(OsinfoOs *self, gchar *hvId)
{
    g_tree_remove(self->priv->hypervisors, hvId);
}

OsinfoDevice *osinfoGetPreferredDeviceForOs(OsinfoOs *self, OsinfoHypervisor *hv, gchar *devType, OsinfoFilter *filter, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_OS(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OS);
        return NULL;
    }

    if (hv && !OSINFO_IS_HYPERVISOR(hv)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_HV);
        return NULL;
    }

    if (!devType) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_DEVTYPE);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // Check if device type info present for <os,hv>, else return NULL.

    GPtrArray *sectionList = NULL;
    if (hv) {
        // Check if hypervisor specific info present for Os, else return NULL.
        struct __osinfoHvSection *hvSection = NULL;
        hvSection = g_tree_lookup(self->priv->hypervisors, (OSINFO_ENTITY(hv))->priv->id);
        if (!hvSection)
            return NULL;

        sectionList = g_tree_lookup(hvSection->sectionsAsList, devType);
        if (!sectionList)
            return NULL;
    }
    else {
        sectionList = g_tree_lookup(self->priv->sectionsAsList, devType);
        if (!sectionList)
            return NULL;
    }

    // For each device in section list, apply filter. If filter passes, return device.
    int i;
    struct __osinfoDeviceLink *deviceLink;
    for (i = 0; i < sectionList->len; i++) {
        deviceLink = g_ptr_array_index(sectionList, i);
        if (__osinfoDevicePassesFilter(filter, deviceLink->dev))
            return deviceLink->dev;
    }

    // If no devices pass filter, return NULL.
    return NULL;
}

OsinfoOsList *osinfoGetRelatedOs(OsinfoOs *self, osinfoRelationship relshp, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_OS(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OS);
        return NULL;
    }

    if (!__osinfoCheckRelationshipValid(relshp)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_INVALID_RELATIONSHIP);
        return NULL;
    }

    // Create our list
    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    GPtrArray *relatedOses = NULL;
    relatedOses = g_tree_lookup(self->priv->relationshipsByType, (gpointer) relshp);
    if (relatedOses) {
        int i, len;
        len = relatedOses->len;
        for (i = 0; i < len; i++) {
            struct __osinfoOsLink *osLink = g_ptr_array_index(relatedOses, i);
            __osinfoListAdd(OSINFO_LIST (newList), OSINFO_ENTITY (osLink->directObjectOs));
        }
    }

    return newList;
}

OsinfoDeviceList *osinfoGetDevicesForOs(OsinfoOs *self, OsinfoHypervisor *hv, gchar *devType, OsinfoFilter *filter, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_OS(self)) {
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

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    GPtrArray *sectionList = NULL;

    // Create our device list
    OsinfoDeviceList *newList = g_object_new(OSINFO_TYPE_DEVICELIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    if (hv) {
        struct __osinfoHvSection *hvSection = NULL;
        hvSection = g_tree_lookup(self->priv->hypervisors, (OSINFO_ENTITY(hv))->priv->id);
        if (!hvSection)
            return newList;

        sectionList = g_tree_lookup(hvSection->sectionsAsList, devType);
        if (!sectionList)
            return newList;
    }
    else {
        sectionList = g_tree_lookup(self->priv->sectionsAsList, devType);
        if (!sectionList)
            return newList;
    }

    // For each device in section list, apply filter. If filter passes, add device to list.
    int i;
    struct __osinfoDeviceLink *deviceLink;
    for (i = 0; i < sectionList->len; i++) {
        deviceLink = g_ptr_array_index(sectionList, i);
        if (__osinfoDevicePassesFilter(filter, deviceLink->dev))
            __osinfoListAdd(OSINFO_LIST (newList), OSINFO_ENTITY (deviceLink->dev));
    }

    return NULL;
}
