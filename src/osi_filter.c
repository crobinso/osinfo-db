#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libosinfo.h>
#include "list.h"
#include "osi_internal_types.h"

osi_filter_t osi_get_filter(osi_lib_t lib, int* err)
{
    struct osi_generic_type * filter_container;
    struct osi_internal_filter * filter;
    struct osi_internal_lib * internal_lib;

    if (!osi_check_lib(lib) || !lib->initialized) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    filter = malloc(sizeof(*filter));
    filter_container = malloc(sizeof(*filter_container));
    if (!filter || !filter_container) {
        free(filter);
        free(filter_container);
        *err = -ENOMEM;
        return NULL;
    }

    filter->num_constraints = 0;
    INIT_LIST_HEAD(&filter->constraints_list);
    filter->num_relationship_constraints = 0;
    INIT_LIST_HEAD(&filter->relationship_constraints_list);
    filter->lib = internal_lib;

    filter_container->backing_object = filter;
    filter_container->type = OSI_FILTER_T;
    filter_container->error = 0;
    filter_container->initialized = 1;

    *err = 0;
    return filter_container;
}

static void clear_constraints(struct osi_internal_filter * filter)
{
    struct osi_filter_relationship * filter_relationship;
    struct osi_keyval * kv_pair;
    struct list_head * cursor, * tmp;

    list_for_each_safe(cursor, tmp, &filter->constraints_list) {
        kv_pair = list_entry(cursor, struct osi_keyval, list);
        list_del(&kv_pair->list);
        __osi_free_kv_pair(kv_pair);
    }
    filter->num_constraints = 0;

    list_for_each_safe(cursor, tmp, &filter->relationship_constraints_list) {
        filter_relationship = list_entry(cursor, struct osi_filter_relationship, list);
        list_del(&filter_relationship->list);
        __osi_free_filter_relationship(filter_relationship);
    }
    filter->num_relationship_constraints = 0;
}

int osi_free_filter(osi_filter_t filter)
{
    struct osi_internal_filter * internal_filter;

    if (!osi_check_filter(filter))
        return -EINVAL;

    internal_filter = filter->backing_object;
    clear_constraints(internal_filter);

    free(internal_filter);
    free(filter);
    return 0;
}

int osi_add_filter_constraint(osi_filter_t filter, char* propname, char* propval)
{
    struct osi_keyval * kv_pair = NULL, * test_kv_pair;
    struct osi_internal_filter * internal_filter;
    char * key_copy, * val_copy;
    struct list_head * cursor;

    if (!osi_check_filter(filter) || !propname || !propval)
        return -EINVAL;

    if (strlen(propname) == 0 || strlen(propval) == 0)
        return -EINVAL;

    /* Will definitely need to copy the value */
    val_copy = strdup(propval);
    if (!val_copy)
        return -ENOMEM;

    /* Check if some value is already set for given key so we can reuse slot */
    internal_filter = filter->backing_object;
    list_for_each(cursor, &internal_filter->constraints_list) {
        test_kv_pair = list_entry(cursor, struct osi_keyval, list);
        if (strcmp(test_kv_pair->key, propname) == 0) {
            kv_pair = test_kv_pair;
            break;
        }
    }

    if (!kv_pair) { /* Need to make new slot for <key, val> */
        kv_pair = malloc(sizeof(*kv_pair));
        key_copy = strdup(propname);
        if (!kv_pair || !key_copy) {
            free(kv_pair);
            free(key_copy);
            free(val_copy);
            return -ENOMEM;
        }
        kv_pair->key = key_copy;
        internal_filter->num_constraints += 1;
        list_add_tail(&kv_pair->list, &internal_filter->constraints_list);
    }
    else /* Need to free existing value before replacing with new */
        free(kv_pair->value);

    kv_pair->value = val_copy;
    return 0;
}

int osi_add_relation_constraint(osi_filter_t filter, osi_relationship relationship, osi_os_t os)
{
    struct osi_filter_relationship * filter_relationship = NULL, * test_fr;
    struct osi_internal_filter * internal_filter;
    struct osi_internal_os * internal_os;
    struct list_head * cursor;

    if (!osi_check_filter(filter) || !osi_check_os(os))
        return -EINVAL;

    internal_filter = filter->backing_object;
    internal_os = os->backing_object;

    list_for_each(cursor, &internal_filter->relationship_constraints_list) {
        test_fr = list_entry(cursor, struct osi_filter_relationship, list);
        if (test_fr->relationship == relationship) {
            filter_relationship = test_fr;
            break;
        }
    }

    if (!filter_relationship) {
        filter_relationship = malloc(sizeof(*filter_relationship));
        if (!filter_relationship)
            return -ENOMEM;
        list_add_tail(&filter_relationship->list, &internal_filter->relationship_constraints_list);
        internal_filter->num_relationship_constraints += 1;
    }

    filter_relationship->os = internal_os;
    filter_relationship->relationship = relationship;
    return 0;
}

int osi_clear_filter_constraint(osi_filter_t filter, char* propname)
{
    struct osi_keyval * kv_pair = NULL, * test_kv_pair;
    struct osi_internal_filter * internal_filter;
    struct list_head * cursor;

    if (!osi_check_filter(filter) || !propname)
        return -EINVAL;

    /* Search for slot containing given key */
    internal_filter = filter->backing_object;
    list_for_each(cursor, &internal_filter->constraints_list) {
        test_kv_pair = list_entry(cursor, struct osi_keyval, list);
        if (strcmp(test_kv_pair->key, propname) == 0) {
            kv_pair = test_kv_pair;
            break;
        }
    }

    /* Didn't find it, so we're done */
    if (!kv_pair)
        return -EINVAL;

    /* Found it, so delete it */
    list_del(&kv_pair->list);
    internal_filter->num_constraints -= 1;
    __osi_free_kv_pair(kv_pair);
    return 0;
}

int osi_clear_relation_constraint(osi_filter_t filter, osi_relationship relationship)
{
    struct osi_filter_relationship * filter_relationship = NULL, * test_fr;
    struct osi_internal_filter * internal_filter;
    struct list_head * cursor;

    if (!osi_check_filter(filter))
        return -EINVAL;

    internal_filter = filter->backing_object;

    list_for_each(cursor, &internal_filter->relationship_constraints_list) {
        test_fr = list_entry(cursor, struct osi_filter_relationship, list);
        if (test_fr->relationship == relationship) {
            filter_relationship = test_fr;
            break;
        }
    }

    if (!filter_relationship)
        return 0;

    list_del(&filter_relationship->list);
    internal_filter->num_relationship_constraints -= 1;
    __osi_free_filter_relationship(filter_relationship);
    return 0;
}

int osi_clear_all_constraints(osi_filter_t filter)
{
    if (!osi_check_filter(filter))
        return -EINVAL;

    clear_constraints(filter->backing_object);
    return 0;
}

char** osi_get_filter_constraint_keys(osi_filter_t filter, int* len, int* err)
{
    struct osi_internal_filter * internal_filter;
    struct osi_keyval * kv_pair;
    char** keys;
    int i;

    if (!err)
        return NULL;

    if (!osi_check_filter(filter) || !len) {
        *err = -EINVAL;
        return NULL;
    }

    internal_filter = filter->backing_object;
    keys = malloc(sizeof(*keys) * internal_filter->num_constraints);
    if (!keys) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(kv_pair, &internal_filter->constraints_list, list) {
        if (i == internal_filter->num_constraints)
            goto error; /* Should *never* happen */

        keys[i] = strdup(kv_pair->key);
        if (!keys[i])
            goto error;
        i += 1;
    }
    *err = 0;
    *len = internal_filter->num_constraints;
    return keys;

error:
    for (i -= 1; i >= 0; i--)
       free(keys[i]);
    free(keys);
    *err = -ENOMEM;
    return NULL;
}

char* osi_get_filter_constraint_value(osi_filter_t filter, char* propname, int* err)
{
    struct osi_keyval * kv_pair = NULL, * test_kv_pair;
    struct osi_internal_filter * internal_filter;
    struct list_head * cursor;
    char* val_copy;

    if (!err)
        return NULL;

    if (!osi_check_filter(filter) || !propname) {
        *err = -EINVAL;
        return NULL;
    }

    /* Search for slot containing given key */
    internal_filter = filter->backing_object;
    list_for_each(cursor, &internal_filter->constraints_list) {
        test_kv_pair = list_entry(cursor, struct osi_keyval, list);
        if (strcmp(test_kv_pair->key, propname) == 0) {
            kv_pair = test_kv_pair;
            break;
        }
    }

    /* Didn't find it, so we're done */
    if (!kv_pair) {
        *err = 0;
        return NULL;
    }

    /* Found it; duplicate value and return */
    val_copy = strdup(kv_pair->value);
    if (!val_copy)
        *err = -ENOMEM;
    else
        *err = 0;
    return val_copy;
}

osi_os_t osi_get_relationship_constraint_value(osi_filter_t filter, osi_relationship relationship, int* err)
{
    struct osi_generic_type * os_container;
    struct osi_filter_relationship * filter_relationship = NULL, * test_fr;
    struct osi_internal_filter * internal_filter;
    struct list_head * cursor;

    if (!err)
        return NULL;

    if (!osi_check_filter(filter)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_filter = filter->backing_object;

    list_for_each(cursor, &internal_filter->relationship_constraints_list) {
        test_fr = list_entry(cursor, struct osi_filter_relationship, list);
        if (test_fr->relationship == relationship) {
            filter_relationship = test_fr;
            break;
        }
    }

    if (!filter_relationship) {
        *err = 0;
        return NULL;
    }

    os_container = malloc(sizeof(*os_container));
    if (!os_container) {
        *err = -ENOMEM;
        return NULL;
    }

    os_container->backing_object = filter_relationship->os;
    os_container->type = OSI_OS_T;
    os_container->error = 0;
    os_container->initialized = 1;

    *err = 0;
    return os_container;
}
