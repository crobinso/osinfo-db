#include <osinfo.h>

static int __osinfoAddDeviceToList(GTree *allSectionsAsList,
                                   gchar *sectionName,
                                   struct __osinfoDeviceLink *deviceLink)
{
    if (!allSectionsAsList || !sectionName || !deviceLink)
        return -EINVAL;

    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *sectionList;
    gchar *sectionNameDup = NULL;

    found = g_tree_lookup_extended(allSectionsAsList, sectionName, &origKey, &foundValue);
    if (!found) {
        sectionList = g_ptr_array_new();
        sectionNameDup = g_strdup(sectionName);

        if (!sectionList)
            goto error_free;
        if (!sectionNameDup) {
            g_ptr_array_free(sectionList, TRUE);
            goto error_free;
        }

        g_tree_insert(allSectionsAsList, sectionNameDup, sectionList);
    }
    else
        sectionList = (GPtrArray *) foundValue;

    g_ptr_array_add(sectionList, deviceLink);
    return 0;

error_free:
    g_free(sectionNameDup);
    return -ENOMEM;
}

int __osinfoAddDeviceToSection(GTree *allSections, GTree *allSectionsAsList, gchar *sectionName, gchar *id, gchar *driver)
{
    if (!allSections || !sectionName || !id)
        return -EINVAL;

    gboolean found;
    gpointer origKey, foundValue;
    gchar *sectionNameDup = NULL, *idDup = NULL, *driverDup = NULL;
    GTree *section;
    struct __osinfoDeviceLink *deviceLink;
    int ret;

    idDup = g_strdup(id);
    driverDup = g_strdup(driver);
    deviceLink = g_malloc(sizeof(*deviceLink));

    if (!idDup || g_strcmp0(driverDup, driver) != 0 || !deviceLink)
        goto error_free;

    found = g_tree_lookup_extended(allSections, sectionName, &origKey, &foundValue);
    if (!found) {
        section = g_tree_new_full(__osinfoStringCompare, NULL, g_free, __osinfoFreeDeviceLink);
        sectionNameDup = g_strdup(sectionName);

        if (!section)
            goto error_free;
        if (!sectionNameDup) {
            g_tree_destroy(section);
            goto error_free;
        }

        g_tree_insert(allSections, sectionNameDup, section);
    }
    else
        section = (GTree *) foundValue;

    deviceLink->driver = driverDup;
    g_tree_insert(section, idDup, deviceLink);

    ret = 0;
    if (allSectionsAsList)
        ret = __osinfoAddDeviceToList(allSectionsAsList, sectionName, deviceLink);

    return ret;

error_free:
    g_free(sectionNameDup);
    g_free(idDup);
    g_free(driverDup);
    g_free(deviceLink);
    return -ENOMEM;
}

void __osinfoClearDeviceSection(GTree *allSections, GTree *allSectionsAsList, gchar *section)
{
    if (!allSections || !section)
        return;

    g_tree_remove(allSections, section);
    g_tree_remove(allSectionsAsList, section);
}

void __osinfoFreeDeviceLink(gpointer ptr)
{
    if (!ptr)
        return;
    struct __osinfoDeviceLink *devLink = (struct __osinfoDeviceLink *) ptr;
    g_free(devLink->driver);
    g_free(devLink);
}

void __osinfoFreeDeviceSection(gpointer tree)
{
    if (!tree)
        return;
    g_tree_destroy((GTree *)tree);
}

gint __osinfoStringCompare(gconstpointer a,
                           gconstpointer b,
                           gpointer data)
{
    // a and b are each gchar *, data is ignored
    gchar *str1 = (gchar *) a;
    gchar *str2 = (gchar *) b;
    return g_strcmp0(str1, str2);
}

gint __osinfoStringCompareBase(gconstpointer a,
                               gconstpointer b)
{
    // a and b are each gchar *, data is ignored
    gchar *str1 = (gchar *) a;
    gchar *str2 = (gchar *) b;
    return g_strcmp0(str1, str2);
}

gint __osinfoIntCompareBase(gconstpointer a,
                            gconstpointer b)
{
    // a and b are each gchar *, data is ignored
    unsigned long int1 = (unsigned long) a;
    unsigned long int2 = (unsigned long) b;
    return a - b;
}

gint __osinfoIntCompare(gconstpointer a,
                        gconstpointer b,
                        gpointer data)
{
    // a and b are each gchar *, data is ignored
    unsigned long int1 = (unsigned long) a;
    unsigned long int2 = (unsigned long) b;
    return a - b;
}

void __osinfoFreePtrArray(gpointer ptrarray)
{
    g_ptr_array_free(ptrarray, TRUE);
}

void __osinfoFreeRelationship(gpointer ptrarray)
{
    if (!ptrarray)
        return;
    __osinfoFreePtrArray(ptrarray);
}

void __osinfoFreeParamVals(gpointer ptrarray)
{
    if (!ptrarray)
        return;
    __osinfoFreePtrArray(ptrarray);
}

void __osinfoFreeOsLink(gpointer ptr)
{
    if (!ptr)
        return;
    struct __osinfoOsLink *osLink = (struct __osinfoOsLink *) ptr;
    g_free(osLink);
}

void __osinfoFreeHvSection(gpointer ptr)
{
    if (!ptr)
        return;
    struct __osinfoHvSection * hvSection = (struct __osinfoHvSection *) ptr;
    g_tree_destroy(hvSection->sections);
    g_tree_destroy(hvSection->sectionsAsList);
    g_free(hvSection);
}

gboolean __osinfoFilterCheckProperty(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoFilterPassArgs *args;
    args = (struct __osinfoFilterPassArgs *) data;
    OsinfoEntity *entity = args->entity;

    // Key is property to filter on
    // Value is array of values for property

    GPtrArray *filterValues = (GPtrArray *) value;
    GPtrArray *entityValues = NULL;

    entityValues = g_tree_lookup(entity->priv->params, key);
    if (!entityValues && filterValues->len > 0) {
        args->passed = 0;
        return TRUE; // not passed
    }

    int i;
    for (i = 0; i < filterValues->len; i++) {
        gchar *currValue = g_ptr_array_index(filterValues, i);
        int j, found = 0;
        for (j = 0; j < entityValues->len; j++) {
            gchar *testValue = g_ptr_array_index(entityValues, j);
            if (g_strcmp0(currValue, testValue) == 0) {
                found = 1;
                break;
            }
        }
        if (found)
            continue;
        else {
            args->passed = 0;
            return TRUE; // not passed
        }
    }

    args->passed = 1;
    return FALSE; // continue iterating
}

int __osinfoEntityPassesFilter(OsinfoFilter *filter, OsinfoEntity *entity)
{
    if (!OSINFO_IS_ENTITY(entity))
        return 0;

    if (!filter)
        return 1;

    if (!OSINFO_IS_FILTER(filter))
        return 0;

    struct __osinfoFilterPassArgs args = {0, entity};
    g_tree_foreach(filter->priv->propertyConstraints, __osinfoFilterCheckProperty, &args);
    if (args.passed)
        return 1;
    else
        return 0;
}

int __osinfoDevicePassesFilter(OsinfoFilter *filter, OsinfoDevice *device)
{
    // Check is device
    return __osinfoEntityPassesFilter(filter, OSINFO_ENTITY (device));
}

gboolean __osinfoFilterCheckRelationships(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoFilterPassArgs *args;
    args = (struct __osinfoFilterPassArgs *) data;
    OsinfoOs *os = (OsinfoOs *) args->entity;

    // Key is relationship to filter on
    // Value is array of oses for relationship

    GPtrArray *filterOses = (GPtrArray *) value;
    GPtrArray *oses = NULL;

    oses = g_tree_lookup(os->priv->relationshipsByType, key);
    if (!oses && filterOses->len > 0) {
        args->passed = 0;
        return TRUE; // not passed
    }

    int i;
    for (i = 0; i < filterOses->len; i++) {
        OsinfoOs *currOs = g_ptr_array_index(filterOses, i);
        int j, found = 0;
        for (j = 0; j < oses->len; j++) {
            OsinfoOs *testOs = g_ptr_array_index(oses, j);
            if (testOs == currOs) {
                found = 1;
                break;
            }
        }
        if (found)
            continue;
        else {
            args->passed = 0;
            return TRUE; // not passed
        }
    }

    args->passed = 1;
    return FALSE; // continue iterating
}

int __osinfoOsPassesFilter(OsinfoFilter *filter, OsinfoOs *os)
{
    if (!OSINFO_IS_OS(os))
        return 0;

    if (!filter)
        return 1;

    if (!OSINFO_IS_FILTER(filter))
        return 0;

    int pass = __osinfoEntityPassesFilter(filter, OSINFO_ENTITY (os));
    if (!pass)
        return 0;

    struct __osinfoFilterPassArgs args = {0, (OsinfoEntity *) os};
    g_tree_foreach(filter->priv->relationshipConstraints, __osinfoFilterCheckRelationships, &args);
    if (args.passed)
        return 1;
    else
        return 0;
}

int __osinfoHypervisorPassesFilter(OsinfoFilter *filter, OsinfoHypervisor *hypervisor)
{
    return __osinfoEntityPassesFilter(filter, OSINFO_ENTITY (hypervisor));
}

int __osinfoCheckGErrorParamValid(GError **err)
{
    // If err is not null and *err is not null, then invalid
    if (err && *err)
        return 0;
    else return 1;
}

int __osinfoCheckRelationshipValid(osinfoRelationship relshp)
{
    return (relshp > RELATIONSHIP_MIN && relshp < RELATIONSHIP_MAX);
}

gchar *__osinfoErrorToString(int err)
{
    switch (err) {
        case -ENOMEM:
            return OSINFO_NO_MEM;
        default:
            return OSINFO_OTHER;
    }
}
