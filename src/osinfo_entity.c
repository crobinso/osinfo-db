#include <osinfo.h>

G_DEFINE_ABSTRACT_TYPE (OsinfoEntity, osinfo_entity, G_TYPE_OBJECT);

#define OSINFO_ENTITY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_ENTITY, OsinfoEntityPrivate))

static void osinfo_entity_finalize (GObject *object);

enum OSI_ENTITY_PROPERTIES {
    OSI_ENTITY_PROP_0,

    OSI_ENTITY_ID,
    OSI_DB_PTR
};

static void
osinfo_entity_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    OsinfoEntity *self = OSINFO_ENTITY (object);

    switch (property_id)
      {
      case OSI_ENTITY_ID:
        g_free(self->priv->id);
        self->priv->id = g_value_dup_string (value);
        break;
      case OSI_DB_PTR:
        self->priv->db = g_value_get_pointer (value);
        break;
      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

static void
osinfo_entity_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    OsinfoEntity *self = OSINFO_ENTITY (object);

    switch (property_id)
      {
      case OSI_ENTITY_ID:
        g_value_set_string (value, self->priv->id);
        break;
      case OSI_DB_PTR:
        g_value_set_pointer(value, self->priv->db);
        break;
      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

static void
osinfo_entity_finalize (GObject *object)
{
    OsinfoEntity *self = OSINFO_ENTITY (object);

    g_free (self->priv->id);
    g_tree_destroy (self->priv->params);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_entity_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_entity_class_init (OsinfoEntityClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;

    g_klass->set_property = osinfo_entity_set_property;
    g_klass->get_property = osinfo_entity_get_property;

    pspec = g_param_spec_string ("id",
                                 "ID",
                                 "Contains unique identifier for entity.",
                                 NULL /* default value */,
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_ENTITY_ID,
                                     pspec);
    pspec = g_param_spec_pointer ("db",
                                 "Db pointer",
                                 "Contains backpointer to libosinfo db object.",
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_PTR,
                                     pspec);

    g_klass->finalize = osinfo_entity_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoEntityPrivate));
}

static void
osinfo_entity_init (OsinfoEntity *self)
{
    OsinfoEntityPrivate *priv;
    self->priv = priv = OSINFO_ENTITY_GET_PRIVATE(self);

    self->priv->params = g_tree_new_full(__osinfoStringCompare, NULL, g_free, __osinfoFreeParamVals);
}

int __osinfoAddParam(OsinfoEntity *self, gchar *key, gchar *value)
{
    if (!OSINFO_IS_ENTITY(self) || !key || !value)
        return -EINVAL;

    // First check if there exists an existing array of entries for this key
    // If not, create a ptrarray of strings for this key and insert into map
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *valueArray;
    gchar *valueDup = NULL, *keyDup = NULL;

    valueDup = g_strdup(value);

    if (!valueDup)
        goto error_free;

    found = g_tree_lookup_extended(self->priv->params, key, &origKey, &foundValue);
    if (!found) {
        keyDup = g_strdup(key);
        valueArray = g_ptr_array_new_with_free_func(g_free);

        if (!valueArray)
            goto error_free;
        if (!keyDup) {
            g_ptr_array_free(valueArray, TRUE);
            goto error_free;
        }

        g_tree_insert(self->priv->params, keyDup, valueArray);
    }
    else
        valueArray = (GPtrArray *) foundValue;

    // Add a copy of the value to the array
    g_ptr_array_add(valueArray, valueDup);
    return 0;

error_free:
    g_free(keyDup);
    g_free(valueDup);
    return -ENOMEM;
}

void __osinfoClearParam(OsinfoEntity *self, gchar *key)
{
    g_tree_remove(self->priv->params, key);
}

gboolean __osinfoGetKeys(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoPtrArrayErr *arrayErr = (struct __osinfoPtrArrayErr *) data;
    GPtrArray *results = arrayErr->array;
    gchar *keyDup = g_strdup(key);

    if (!keyDup) {
        arrayErr->err = -ENOMEM;
        return TRUE;
    }
    g_ptr_array_add(results, keyDup);
    return FALSE; // Continue iterating
}

void __osinfoDupArray(gpointer data, gpointer user_data)
{
    struct __osinfoPtrArrayErr *arrayErr = (struct __osinfoPtrArrayErr *) data;
    GPtrArray *results = arrayErr->array;

    if (arrayErr->err != 0)
        return;

    gchar *valueDup = g_strdup((gchar *)data);
    if (!valueDup) {
        arrayErr->err = -ENOMEM;
        return;
    }
    g_ptr_array_add(results, valueDup);
}

gchar *osinfoGetId(OsinfoEntity *self, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_ENTITY(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_ENTITY);
        return NULL;
    }

    gchar *dupId = g_strdup(self->priv->id);
    if (!dupId) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }
    return dupId;
}

GPtrArray *osinfoGetParams(OsinfoEntity *self, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_ENTITY(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_ENTITY);
        return NULL;
    }

    GPtrArray *params = g_ptr_array_new();
    if (!params) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    struct __osinfoPtrArrayErr arrayErr = {params, 0};
    g_tree_foreach(self->priv->params, __osinfoGetKeys, &arrayErr);

    // If we had an error, cleanup and return NULL
    if (arrayErr.err != 0) {
        int i;
        for (i = 0; i < params->len; i++)
            g_free(g_ptr_array_index(params, i));
        g_ptr_array_free(params, TRUE);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), arrayErr.err, __osinfoErrorToString(arrayErr.err));
        return NULL;
    }

    return params;
}

gchar *osinfoGetParamValue(OsinfoEntity *self, gchar *key, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_ENTITY(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_ENTITY);
        return NULL;
    }

    if (!key) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return NULL;
    }

    gboolean found;
    gpointer origKey, value;
    gchar *firstValueDup;
    GPtrArray *array;

    found = g_tree_lookup_extended(self->priv->params, key, &origKey, &value);
    if (!found)
        return NULL;
    array = (GPtrArray *) value;
    if (array->len == 0)
        return NULL;

    firstValueDup = g_strdup(g_ptr_array_index(array, 0));
    if (!firstValueDup) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    return firstValueDup;
}

GPtrArray *osinfoGetParamAllValues(OsinfoEntity *self, gchar *key, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_ENTITY(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_ENTITY);
        return NULL;
    }

    if (!key) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return NULL;
    }

    gboolean found;
    gpointer origKey, value;
    GPtrArray *srcArray, *retArray;

    retArray = g_ptr_array_new();
    if (!retArray) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    found = g_tree_lookup_extended(self->priv->params, key, &origKey, &value);
    if (!found)
        return retArray;
    srcArray = (GPtrArray *) value;
    if (srcArray->len == 0)
        return retArray;

    struct __osinfoPtrArrayErr arrayErr = {retArray, 0};
    g_ptr_array_foreach(srcArray, __osinfoDupArray, &arrayErr);
    if (arrayErr.err) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), arrayErr.err, __osinfoErrorToString(arrayErr.err));
        g_ptr_array_set_free_func(retArray, g_free);
        g_ptr_array_free(retArray, TRUE);
        return NULL;
    }

    return retArray;
}
