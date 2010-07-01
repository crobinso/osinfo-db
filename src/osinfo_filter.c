#include <osinfo.h>

G_DEFINE_TYPE (OsinfoFilter, osinfo_filter, G_TYPE_OBJECT);

#define OSINFO_FILTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_FILTER, OsinfoFilterPrivate))

static void osinfo_filter_finalize (GObject *object);

static void
osinfo_filter_finalize (GObject *object)
{
    OsinfoFilter *self = OSINFO_FILTER (object);

    g_tree_destroy(self->priv->propertyConstraints);
    g_tree_destroy(self->priv->relationshipConstraints);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_filter_parent_class)->finalize (object);
}

/* Init functions */
static void
osinfo_filter_class_init (OsinfoFilterClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);

    g_klass->finalize = osinfo_filter_finalize;
    g_type_class_add_private (klass, sizeof (OsinfoFilterPrivate));
}

static void
osinfo_filter_init (OsinfoFilter *self)
{
    OsinfoFilterPrivate *priv;
    priv = OSINFO_FILTER_GET_PRIVATE(self);
    self->priv = priv;

    self->priv->propertyConstraints = g_tree_new_full(__osinfoStringCompare,
                                                     NULL,
                                                     g_free,
                                                     __osinfoFreePtrArray);


    self->priv->relationshipConstraints = g_tree_new_full(__osinfoIntCompare,
                                                         NULL,
                                                         NULL,
                                                         __osinfoFreePtrArray);
}

void osinfoFreeFilter(OsinfoFilter *self)
{
    g_object_unref(self);
}

gint osinfoAddFilterConstraint(OsinfoFilter *self, gchar *propName, gchar *propVal, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return -EINVAL;

    if (!OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return -EINVAL;
    }

    if (!propName) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPNAME);
        return -EINVAL;
    }

    if (!propVal) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_NO_PROPVAL);
        return -EINVAL;
    }

    // First check if there exists an array of entries for this key
    // If not, create a ptrarray of strings for this key and insert into map
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *valueArray;
    gchar *valueDup = NULL, *keyDup = NULL;

    valueDup = g_strdup(propVal);

    if (!valueDup)
        goto error_free;

    found = g_tree_lookup_extended(self->priv->propertyConstraints, propName, &origKey, &foundValue);
    if (!found) {
        keyDup = g_strdup(propName);
        valueArray = g_ptr_array_new_with_free_func(g_free);

        if (!valueArray)
            goto error_free;
        if (!keyDup) {
            g_ptr_array_free(valueArray, TRUE);
            goto error_free;
        }

        g_tree_insert(self->priv->propertyConstraints, keyDup, valueArray);
    }
    else
        valueArray = (GPtrArray *) foundValue;

    // Add a copy of the value to the array
    g_ptr_array_add(valueArray, valueDup);
    return 0;

error_free:
    g_free(keyDup);
    g_free(valueDup);
    g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
    return -ENOMEM;
}

// Only applicable to OSes, ignored by other types of objects
gint osinfoAddRelationConstraint(OsinfoFilter *self, osinfoRelationship relshp, OsinfoOs *os, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return -EINVAL;

    if (!OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return -EINVAL;
    }

    if (!__osinfoCheckRelationshipValid(relshp)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_INVALID_RELATIONSHIP);
        return -EINVAL;
    }

    if (!OSINFO_IS_OS(os)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_OS);
        return -EINVAL;
    }

    // First check if there exists an array of entries for this key
    // If not, create a ptrarray of strings for this key and insert into map
    gboolean found;
    gpointer origKey, foundValue;
    GPtrArray *valueArray;

    found = g_tree_lookup_extended(self->priv->relationshipConstraints, (gpointer) relshp, &origKey, &foundValue);
    if (!found) {
        valueArray = g_ptr_array_new();
        if (!valueArray)
            goto error_nomem;

        g_tree_insert(self->priv->relationshipConstraints, (gpointer) relshp, valueArray);
    }
    else
        valueArray = (GPtrArray *) foundValue;

    // Add to the array
    g_ptr_array_add(valueArray, os);
    return 0;

error_nomem:
    g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
    return -ENOMEM;
}

void osinfoClearFilterConstraint(OsinfoFilter *self, gchar *propName)
{
    g_tree_remove(self->priv->propertyConstraints, propName);
}

void osinfoClearRelationshipConstraint(OsinfoFilter *self, osinfoRelationship relshp)
{
    g_tree_remove(self->priv->relationshipConstraints, (gpointer) relshp);
}

static gboolean __osinfoRemoveTreeEntry(gpointer key, gpointer value, gpointer data)
{
    GTree *tree = (GTree *) data;
    g_tree_remove(tree, key);
    return FALSE; // continue iterating
}

void osinfoClearAllFilterConstraints(OsinfoFilter *self)
{
    g_tree_foreach(self->priv->propertyConstraints, __osinfoRemoveTreeEntry, self->priv->propertyConstraints);
    g_tree_foreach(self->priv->relationshipConstraints, __osinfoRemoveTreeEntry, self->priv->relationshipConstraints);
}

// get keyset for constraints map
GPtrArray *osinfoGetFilterConstraintKeys(OsinfoFilter *self, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    GPtrArray *constraints = g_ptr_array_new();
    if (!constraints) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -ENOMEM, OSINFO_NO_MEM);
        return NULL;
    }

    struct __osinfoPtrArrayErr arrayErr = {constraints, 0};
    g_tree_foreach(self->priv->propertyConstraints, __osinfoGetKeys, &arrayErr);

    // If we had an error, cleanup and return NULL
    if (arrayErr.err != 0) {
        int i;
        for (i = 0; i < constraints->len; i++)
            g_free(g_ptr_array_index(constraints, i));
        g_ptr_array_free(constraints, TRUE);
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), arrayErr.err, __osinfoErrorToString(arrayErr.err));
        return NULL;
    }

    return constraints;
}

// get values for given key
GPtrArray *osinfoGetFilterConstraintValues(OsinfoFilter *self, gchar *propName, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
        return NULL;
    }

    if (!propName) {
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

    found = g_tree_lookup_extended(self->priv->propertyConstraints, propName, &origKey, &value);
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

// get oses for given relshp
OsinfoOsList *osinfoGetRelationshipConstraintValue(OsinfoFilter *self, osinfoRelationship relshp, GError **err)
{
    if (!__osinfoCheckGErrorParamValid(err))
        return NULL;

    if (!OSINFO_IS_FILTER(self)) {
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), -EINVAL, OSINFO_OBJ_NOT_FILTER);
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
    relatedOses = g_tree_lookup(self->priv->relationshipConstraints, (gpointer) relshp);
    if (relatedOses) {
        int i, len;
        len = relatedOses->len;
        for (i = 0; i < len; i++) {
             OsinfoOs *os = g_ptr_array_index(relatedOses, i);
            __osinfoListAdd(OSINFO_LIST (newList), OSINFO_ENTITY (os));
        }
    }

    return newList;
}
