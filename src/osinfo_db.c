#include <osinfo.h>

G_DEFINE_TYPE (OsinfoDb, osinfo_db, G_TYPE_OBJECT);

#define OSINFO_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_DB, OsinfoDbPrivate))

static void osinfo_db_set_property(GObject * object, guint prop_id,
                                         const GValue * value,
                                         GParamSpec * pspec);
static void osinfo_db_get_property(GObject * object, guint prop_id,
                                         GValue * value,
                                         GParamSpec * pspec);
static void osinfo_db_finalize (GObject *object);

enum OSI_DB_PROPERTIES {
    OSI_DB_PROP_0,

    OSI_DB_BACKING_DIR,
    OSI_DB_LIBVIRT_VER,
    OSI_DB_ERROR
};

static void
osinfo_db_finalize (GObject *object)
{
    OsinfoDb *self = OSINFO_DB (object);

    g_free (self->priv->backing_dir);
    g_free (self->priv->libvirt_ver);

    g_tree_destroy(self->priv->devices);
    g_tree_destroy(self->priv->hypervisors);
    g_tree_destroy(self->priv->oses);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_db_parent_class)->finalize (object);
}

static void
osinfo_db_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    OsinfoDb *self = OSINFO_DB (object);

    switch (property_id)
      {
      case OSI_DB_BACKING_DIR:
        g_free(self->priv->backing_dir);
        self->priv->backing_dir = g_value_dup_string (value);
        break;

      case OSI_DB_LIBVIRT_VER:
        g_free(self->priv->libvirt_ver);
        self->priv->libvirt_ver = g_value_dup_string (value);
        break;

      case OSI_DB_ERROR:
        break;

      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

static void
osinfo_db_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    OsinfoDb *self = OSINFO_DB (object);

    switch (property_id)
      {
      case OSI_DB_BACKING_DIR:
        g_value_set_string (value, self->priv->backing_dir);
        break;

      case OSI_DB_LIBVIRT_VER:
        g_value_set_string (value, self->priv->libvirt_ver);
        break;

      case OSI_DB_ERROR:
        g_value_set_pointer(value, self->priv->error);
        break;

      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

/* Init functions */
static void
osinfo_db_class_init (OsinfoDbClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;

    g_klass->set_property = osinfo_db_set_property;
    g_klass->get_property = osinfo_db_get_property;
    g_klass->finalize = osinfo_db_finalize;

    pspec = g_param_spec_string ("backing-dir",
                                 "Backing directory",
                                 "Contains backing data store.",
                                 NULL /* default value */,
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_BACKING_DIR,
                                     pspec);

    pspec = g_param_spec_string ("libvirt-ver",
                                 "Libvirt version",
                                 "Libvirt version user is interested in",
                                 NULL /* default value */,
                                 G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_LIBVIRT_VER,
                                     pspec);

    pspec = g_param_spec_pointer ("error",
                                  "Error",
                                  "GError object for db",
                                  G_PARAM_READABLE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_ERROR,
                                     pspec);

    g_type_class_add_private (klass, sizeof (OsinfoDbPrivate));
}


static void
osinfo_db_init (OsinfoDb *self)
{
    OsinfoDbPrivate *priv;
    int ret;
    self->priv = priv = OSINFO_DB_GET_PRIVATE(self);

    self->priv->devices = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);
    self->priv->hypervisors = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);
    self->priv->oses = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);

    self->priv->error = NULL;
    self->priv->ready = 0;
}

/** PUBLIC METHODS */

int osinfoInitializeDb(OsinfoDb *self, GError **err)
{
    int ret;
    // And now read in data.
    ret = __osinfoInitializeData(self);
    if (ret != 0) {
        self->priv->ready = 0;
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, "Error during reading data");
    }
    else
        self->priv->ready = 1;
    return ret;
}

OsinfoHypervisor *osinfoGetHypervisorById(OsinfoDb *self, gchar *id, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!id) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_ID);
        return NULL;
    }

    return g_tree_lookup(self->priv->hypervisors, id);
}

OsinfoDevice *osinfoGetDeviceById(OsinfoDb *self, gchar *id, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!id) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_ID);
        return NULL;
    }

    return g_tree_lookup(self->priv->devices, id);
}

OsinfoOs *osinfoGetOsById(OsinfoDb *self, gchar *id, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!id) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_ID);
        return NULL;
    }

    return g_tree_lookup(self->priv->oses, id);
}

static gboolean __osinfoFilteredAddToList(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoPopulateListArgs *args;
    args = (struct __osinfoPopulateListArgs *) data;
    OsinfoFilter *filter = args->filter;
    OsinfoList *list = args->list;
    GError **err = args->err;

    // Key is string ID, value is pointer to entity
    OsinfoEntity *entity = (OsinfoEntity *) value;
    if (__osinfoEntityPassesFilter(filter, entity)) {
        __osinfoListAdd(list, entity);
    }

    args->errcode = 0;

    return FALSE; // continue iteration
}

static int __osinfoPopulateList(GTree *entities, OsinfoList *newList, OsinfoFilter *filter, GError **err)
{
    struct __osinfoPopulateListArgs args = {err, 0, filter, newList};
    g_tree_foreach(entities, __osinfoFilteredAddToList, &args);
    return args.errcode;
}

OsinfoOsList *osinfoGetOsList(OsinfoDb *self, OsinfoFilter *filter, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // Create list
    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    int ret;
    ret = __osinfoPopulateList(self->priv->oses, OSINFO_LIST (newList), filter, err);
    if (ret != 0) {
        g_object_unref(newList);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, __osinfoErrorToString(ret));
        return NULL;
    }

    return newList;
}

OsinfoHypervisorList *osinfoGetHypervisorList(OsinfoDb *self, OsinfoFilter *filter, GError **err)
{
    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // Create list
    OsinfoHypervisorList *newList = g_object_new(OSINFO_TYPE_HYPERVISORLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    int ret;
    ret = __osinfoPopulateList(self->priv->hypervisors, OSINFO_LIST (newList), filter, err);
    if (ret != 0) {
        g_object_unref(newList);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, __osinfoErrorToString(ret));
        return NULL;
    }

    return newList;
}

OsinfoDeviceList *osinfoGetDeviceList(OsinfoDb *self, OsinfoFilter *filter, GError **err)
{
    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (filter && !OSINFO_IS_FILTER(filter)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    // Create list
    OsinfoDeviceList *newList = g_object_new(OSINFO_TYPE_DEVICELIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    int ret;
    ret = __osinfoPopulateList(self->priv->devices, OSINFO_LIST (newList), filter, err);
    if (ret != 0) {
        g_object_unref(newList);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, __osinfoErrorToString(ret));
        return NULL;
    }

    return newList;
}

gboolean __osinfoGetPropertyValuesInEntity(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoPopulateValuesArgs *args;
    args = (struct __osinfoPopulateValuesArgs *) data;
    GTree *values = args->values;
    GError **err = args->err;
    gchar *property = args->property;

    OsinfoEntity *entity = OSINFO_ENTITY (value);
    GPtrArray *valueArray = NULL;

    valueArray = g_tree_lookup(entity->priv->params, property);
    if (valueArray)
        return FALSE; // No values here, skip

    int i;
    for (i = 0; i < valueArray->len; i++) {
        gchar *currValue = g_ptr_array_index(valueArray, i);
        void *test = g_tree_lookup(values, currValue);
        if (test)
            continue;
        gchar *dupValue = g_strdup(currValue);
        if (!dupValue) {
            g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
            args->errcode = -ENOMEM;
            return TRUE;
        }

        // Add to tree with dummy value
        g_tree_insert(values, dupValue, (gpointer) 1);
    }

    return FALSE; // Continue iterating
}

static gboolean __osinfoPutKeysInList(gpointer key, gpointer value, gpointer data)
{
    gchar *currValue = (gchar *) key;
    GPtrArray *valuesList = (GPtrArray *) data;

    g_ptr_array_add(valuesList, currValue);
    return FALSE; // keep iterating
}

static gboolean __osinfoFreeKeys(gpointer key, gpointer value, gpointer data)
{
    g_free(key);
    return FALSE; // keep iterating
}

static GPtrArray *__osinfoUniqueValuesForPropertyInEntity(GTree *entities, gchar *propName, GError **err)
{
    GTree *values = g_tree_new(__osinfoStringCompareBase);
    if (!values) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    struct __osinfoPopulateValuesArgs args = {err, 0, values, propName};
    g_tree_foreach(entities, __osinfoGetPropertyValuesInEntity, &args);

    if (args.errcode != 0) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), args.errcode, __osinfoErrorToString(args.errcode));
        g_tree_foreach(values, __osinfoFreeKeys, NULL);
        g_tree_destroy(values);
        return NULL;
    }

    // For each key in tree, add to gptrarray
    GPtrArray *valuesList = g_ptr_array_sized_new(g_tree_nnodes(values));
    if (!valuesList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        g_tree_foreach(values, __osinfoFreeKeys, NULL);
        g_tree_destroy(values);
        return NULL;
    }

    g_tree_foreach(values, __osinfoPutKeysInList, valuesList);
    g_tree_destroy(values);
    return valuesList;
}

// Get me all unique values for property "vendor" among operating systems
GPtrArray *osinfoUniqueValuesForPropertyInOs(OsinfoDb *self, gchar *propName, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!propName) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return NULL;
    }

    return __osinfoUniqueValuesForPropertyInEntity(self->priv->oses, propName, err);
}

// Get me all unique values for property "vendor" among hypervisors
GPtrArray *osinfoUniqueValuesForPropertyInHv(OsinfoDb *self, gchar *propName, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!propName) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return NULL;
    }

    return __osinfoUniqueValuesForPropertyInEntity(self->priv->hypervisors, propName, err);
}

// Get me all unique values for property "vendor" among devices
GPtrArray *osinfoUniqueValuesForPropertyInDev(OsinfoDb *self, gchar *propName, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!propName) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return NULL;
    }

    return __osinfoUniqueValuesForPropertyInEntity(self->priv->devices, propName, err);
}

static gboolean __osinfoAddOsIfRelationship(gpointer key, gpointer value, gpointer data)
{
    OsinfoOs *os = (OsinfoOs *) value;
    struct __osinfoOsCheckRelationshipArgs *args;
    args = (struct __osinfoOsCheckRelationshipArgs *) data;
    GError **err = args->err;
    OsinfoList *list = args->list;

    GPtrArray *relatedOses = NULL;
    relatedOses = g_tree_lookup(os->priv->relationshipsByType, (gpointer) args->relshp);
    if (relatedOses) {
        __osinfoListAdd(list, OSINFO_ENTITY (os));
    }

    return FALSE;
}

// Get me all OSes that 'upgrade' another OS (or whatever relationship is specified)
OsinfoOsList *osinfoUniqueValuesForOsRelationship(OsinfoDb *self, osinfoRelationship relshp, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_DB(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_DB);
        return NULL;
    }

    if (!__osinfoCheckRelationshipValid(relshp)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_INVALID_RELATIONSHIP);
        return NULL;
    }

    // Create list
    OsinfoOsList *newList = g_object_new(OSINFO_TYPE_OSLIST, NULL);
    if (!newList) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    struct __osinfoOsCheckRelationshipArgs args = {OSINFO_LIST (newList), 0, err, relshp};

    g_tree_foreach(self->priv->oses, __osinfoAddOsIfRelationship, &args);
    if (args.errcode != 0) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), args.errcode, __osinfoErrorToString(args.errcode));
        g_object_unref(newList);
        return NULL;
    }

    return newList;
}

/**  PRIVATE */

void __osinfoAddDeviceToDb(OsinfoDb *db, OsinfoDevice *dev)
{
    gchar *id;
    g_object_get(G_OBJECT(dev), "id", &id, NULL);
    g_tree_insert(db->priv->devices, id, dev);
}

void __osinfoAddHypervisorToDb(OsinfoDb *db, OsinfoHypervisor *hv)
{
    gchar *id;
    g_object_get(G_OBJECT(hv), "id", &id, NULL);
    g_tree_insert(db->priv->hypervisors, id, hv);
}

void __osinfoAddOsToDb(OsinfoDb *db, OsinfoOs *os)
{
    gchar *id;
    g_object_get(G_OBJECT(os), "id", &id, NULL);
    g_tree_insert(db->priv->oses, id, os);
}
