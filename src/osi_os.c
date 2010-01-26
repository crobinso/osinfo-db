#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libosinfo.h>
#include "list.h"
#include "osi_internal_types.h"

static char** os_property_all_values(struct osi_internal_os * os,
                                     char* propname,
                                     int* num,
                                     int* err)
{
    int i;
    char** property_values;
    struct osi_keyval_multi * kv, * test_kv;
    struct list_head * cursor;
    struct osi_value * value;

    /* First find the appropriate node based on propname */
    list_for_each(cursor, &os->param_list) {
        test_kv = list_entry(cursor, struct osi_keyval_multi, list);
        if (strcmp(propname, test_kv->key) == 0) {
            kv = test_kv;
            break;
        }
    }
    if (!kv) {
       *err = -EINVAL;
       return NULL;
    }

    /* Found it, now copy all the values over */
    property_values = malloc(sizeof(*property_values) * kv->num_values);
    if (!property_values) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(value, &kv->values_list, list) {
        if (i == kv->num_values)
            goto error; /* should *never* happen */

        property_values[i] = strdup(value->val);
        if (!property_values[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = kv->num_values;
    return property_values;

error:
    for (i -= 1; i >= 0; i--)
       free(property_values[i]);
    free(property_values);
    *err = -ENOMEM;
    return NULL;
}

int append_in_place(char** dest, int dest_curr_len, char** src, int src_len)
{
    int i;
    for (i = 0; i < src_len; i++) {
        dest[dest_curr_len + i] = strdup(src[i]);
        if (!dest[dest_curr_len + i])
            return -ENOMEM;
    }
    return dest_curr_len + src_len;
}

char** append(char*** arrays, int* lengths, int numarrays,
              int* newlength, int* err)
{
    char** combined;
    int total_len, i, j;

    total_len = 0;
    for (i = 0; i < numarrays; i++)
        total_len += lengths[i];

    combined = malloc(sizeof(*combined) * total_len);
    if (!combined) {
        *err = -ENOMEM;
        return NULL;
    }

    j = 0;
    for (i = 0; i < numarrays; i++) {
        j = append_in_place(combined, j, arrays[i], lengths[i]);
        if (j < 0)
            goto error;
    }

    *newlength = j;
    return combined;

error:
    for (j -= 1; j >= 0; j--)
       free(combined[j]);
    free(combined);
    *err = -ENOMEM;
    return NULL;
}

char** copy_array(char** arr, int len, int* err)
{
    int i;
    char** dup;

    dup = malloc(sizeof(*dup) * len);
    if (!dup) {
        *err = -ENOMEM;
        return NULL;
    }

    for (i = 0; i < len; i++) {
        dup[i] = strdup(arr[i]);
        if (!dup[i])
            goto error;
    }

    *err = 0;
    return dup;

error:
    for (i -= 1; i >= 0; i--)
       free(dup[i]);
    free(dup);
    return NULL;
}

int strcmp_wrapper(const void * a, const void * b)
{
    return strcmp(*(char **)a, *(char **)b);
}

char** remove_duplicates(char** arr, int len, int* newlen, int* err)
{
    int i, j;
    char ** dup, ** pruned;

    dup = copy_array(arr, len, err);
    if (!dup || *err != 0) {
        return NULL;
    }

    /* Make space for pruned array */
    pruned = malloc(sizeof(*pruned) * len);
    if (!pruned) {
        for (i = 0; i < len; i++)
            free(dup[i]);
        free(dup);

        *err = -ENOMEM;
        return NULL;
    }

    /* Sort and copy over non-duplicate elements */
    qsort(dup, len, sizeof(*dup), strcmp_wrapper);

    j = 0;
    for (i = 0; i < len; i++) {
        pruned[j] = strdup(dup[i]);
        if (!pruned[j])
            goto error;
        while(i <= len - 2 && strcmp(dup[i], dup[i+1]) == 0)
            i += 1;
        j += 1;
    }

    /* Free the pre-pruned array since we are done with it */
    for (i = 0; i < len; i++)
        free(dup[i]);
    free(dup);

    /* Get rid of excess space */
    pruned = realloc(pruned, sizeof(*pruned)*j);
    if (!pruned) {
        *err = -ENOMEM;
        return NULL;
    }

    *newlen = j;
    return pruned;

error:
    for (j -= 1; j >= 0; j--)
       free(pruned[j]);
    free(pruned);
    return NULL;
}

char** osi_unique_property_values(osi_lib_t lib, char* propname, int* num, int* err)
{
    int i, j, len;
    char *** os_values, ** unpruned, ** pruned, ** retval = NULL;
    int * list_lengths;
    struct osi_internal_lib * internal_lib;
    struct osi_internal_os * os;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !propname || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    os_values = malloc(sizeof(*os_values) * internal_lib->num_os);
    list_lengths = malloc(sizeof(*list_lengths) * internal_lib->num_os);
    if (!os_values || !list_lengths) {
        free(os_values);
        free(list_lengths);
        *err = -ENOMEM;
        return NULL;
    }

    /* First, for each os, get all values for the given propname */
    i = 0;
    list_for_each_entry(os, &internal_lib->os_list, list) {
        os_values[i] = os_property_all_values(os, propname, &list_lengths[i], err);
        if (!os_values[i] || *err != 0)
            goto out;
        i += 1;
    }

    /* Then, append all these lists together */
    unpruned = append(os_values, list_lengths, internal_lib->num_os, &len, err);
    if (!unpruned || *err != 0)
        goto out;

    /* Then, remove duplicates from the list  and return */
    pruned = remove_duplicates(unpruned, len, num, err);

    /* Done with 'unpruned' sorted list */
    for (j = 0; j < len; j++)
            free(unpruned[j]);
        free(unpruned);

    if (!pruned || *err != 0)
        goto out;

    else
        retval = pruned;

out:
    for (i -= 1; i >= 0; i--) {
        for (j = 0; j < list_lengths[i]; j++)
            free(os_values[i][j]);
        free(os_values[i]);
    }
    free(os_values);
    free(list_lengths);
    return retval;
}

static struct osi_internal_os * get_related_os(struct osi_internal_os * os,
                                               osi_relationship relationship,
                                               int* err)
{
    struct osi_os_link * os_link = NULL, * test_os_link;
    struct list_head * cursor;

    list_for_each(cursor, &os->relationships_list) {
        test_os_link = list_entry(cursor, struct osi_os_link, list);
        if (test_os_link->verb == relationship) {
            os_link = test_os_link;
            break;
        }
    }

    if (!os_link) {
        *err = -EINVAL;
        return NULL;
    }

    *err = 0;
    return os_link->direct_object_os;
}

static int os_passes_filter(struct osi_internal_os * os,
                            struct osi_internal_filter * filter)
{
    struct osi_keyval * kv;
    struct osi_filter_relationship * fr;
    struct osi_internal_os * target_os;
    char** values;
    int num, err, i, match;

    /* Any os passes the null filter */
    if (!filter)
        return 1;

    /* Check if it passes all normal constraints */
    list_for_each_entry(kv, &filter->constraints_list, list) {
        values = os_property_all_values(os, kv->key, &num, &err);
        if (!values || err != 0)
            return 0;

        /* Check to see if one of the values is a match */
        match = 0;
        for (i = 0; i < num; i++) {
            if (strcmp(values[i], kv->value) == 0) {
                match = 1;
                break;
            }
        }
        /* Free values we got since we're done with them */
        for (i = 0; i < num; i++)
            free(values[i]);
        free(values);

        /* If no match then fail, else continue */
        if (!match)
            return 0;
    }

    /* Check if it passes all relationship constraints */
    list_for_each_entry(fr, &filter->relationship_constraints_list, list) {
        target_os = get_related_os(os, fr->relationship, &err);
        if (!target_os || err != 0)
            return 0;

        if (target_os != fr->os)
            return 0;
    }

    /* And if it does, then it passes the filter */
    return 1;
}

static int os_participates_in_relationship(struct osi_internal_os * os,
                                           osi_relationship relationship)
{
    struct osi_os_link * os_link;
    struct list_head * cursor;

    list_for_each_entry(os_link, &os->relationships_list, list)
        if (os_link->verb == relationship)
            return 1;

    return 0;
}

static osi_os_list_t wrapped_os_list(struct osi_os_list * os_list)
{
    struct osi_generic_type * os_list_container;

    os_list_container = malloc(sizeof(*os_list_container));
    if (!os_list_container) {
        return NULL;
    }

    os_list_container->backing_object = os_list;
    os_list_container->initialized = 1;
    os_list_container->type = OSI_OS_LIST_T;
    os_list_container->error = 0;

    return os_list_container;
}

osi_os_list_t osi_unique_relationship_values(osi_lib_t lib, osi_relationship relationship, int* err)
{
    int len;
    struct osi_generic_type * os_list_container;
    struct osi_os_list * os_list;
    struct osi_internal_os ** os_refs, * os;
    struct osi_internal_lib * internal_lib;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !osi_valid_os_relationship(relationship)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;
    len = 0;

    os_list = malloc(sizeof(*os_list));
    os_refs = malloc(sizeof(*os_refs) * internal_lib->num_os);
    if (!os_list) {
        free(os_list);
        free(os_refs);
        *err = -ENOMEM;
        return NULL;
    }

    /* Iterate os list and check against filter (adding to list if passed) */
    list_for_each_entry(os, &internal_lib->os_list, list) {
        if (os_participates_in_relationship(os, relationship)) {
            os_refs[len] = os;
            len += 1;
        }
    }

    /* Resize the list so we're not wasting memory */
    os_refs = realloc(os_refs, sizeof(*os_refs) * len);
    if (!os_refs && len != 0) {
        free(os_list);
        *err = -ENOMEM;
        return NULL;
    }
    os_list->os_refs = os_refs;
    os_list->length = len;

    os_list_container = wrapped_os_list(os_list);
    if (!os_list_container) {
        free(os_list);
        free(os_refs);
        *err = -ENOMEM;
        return NULL;
    }
    return os_list_container;
}

static osi_os_t wrapped_os(struct osi_internal_os * os, int* err)
{
    struct osi_generic_type * os_container;

    os_container = malloc(sizeof(*os_container));
    if (!os_container) {
        *err = -ENOMEM;
        return NULL;
    }

    os_container->backing_object = os;
    os_container->initialized = 1;
    os_container->type = OSI_OS_T;
    os_container->error = 0;

    *err = 0;
    return os_container;
}

osi_os_list_t osi_get_os_list(osi_lib_t lib, osi_filter_t filter, int* err)
{
    int len;
    struct osi_generic_type * os_list_container;
    struct osi_os_list * os_list;
    struct osi_internal_os ** os_refs, * os;
    struct osi_internal_lib * internal_lib;
    struct osi_internal_filter * internal_filter;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) ||
        (filter!= NULL && !osi_check_filter(filter)) ) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    if (filter != NULL)
        internal_filter = filter->backing_object;
    else
        internal_filter = NULL;

    len = 0;

    os_list = malloc(sizeof(*os_list));
    os_refs = malloc(sizeof(*os_refs) * internal_lib->num_os);
    if (!os_list || !os_refs) {
        free(os_list);
        free(os_refs);
        *err = -ENOMEM;
        return NULL;
    }

    /* Iterate os list and check against filter (adding to list if passed) */
    list_for_each_entry(os, &internal_lib->os_list, list) {
        if (os_passes_filter(os, internal_filter)) {
            os_refs[len] = os;
            len += 1;
        }
    }

    /* Resize the list so we're not wasting memory */
    os_refs = realloc(os_refs, sizeof(*os_refs) * len);
    if (!os_refs && len != 0) {
        free(os_list);
        *err = -ENOMEM;
        return NULL;
    }
    os_list->os_refs = os_refs;
    os_list->length = len;

    os_list_container = wrapped_os_list(os_list);
    if (!os_list_container) {
        free(os_list);
        free(os_refs);
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return os_list_container;
}

int osi_free_os_list(osi_os_list_t list)
{
    struct osi_os_list * os_list;

    if (!osi_check_os_list(list))
        return -EINVAL;

    os_list = list->backing_object;

    free(os_list->os_refs);
    free(os_list);
    free(list);
    return 0;
}

int osi_os_list_length(osi_os_list_t list)
{
    struct osi_os_list * os_list;

    if (!osi_check_os_list(list))
        return -EINVAL;

    os_list = list->backing_object;
    return os_list->length;
}

osi_os_t osi_get_os_by_index(osi_os_list_t list, int index, int* err)
{
    struct osi_os_list * os_list;
    struct osi_internal_os * os;

    if (!err)
        return NULL;

    if (!osi_check_os_list(list)) {
        *err = -EINVAL;
        return NULL;
    }

    os_list = list->backing_object;

    /* Check bounds */
    if (index < 0 || index >= os_list->length) {
        *err = -EINVAL;
        return NULL;
    }

    os = os_list->os_refs[index];
    return wrapped_os(os, err);
}

char* osi_get_os_id(osi_os_t os, int* err)
{
    struct osi_internal_os * internal_os;
    char* id;

    if (!err)
        return NULL;

    if (!osi_check_os(os)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;

    id = strdup(internal_os->id);
    if (!id) {
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return id;
}

osi_os_t osi_get_os_by_id(osi_lib_t lib, char* id, int* err)
{
    struct osi_internal_os * os = NULL, * test_os;
    struct osi_internal_lib * internal_lib;
    struct list_head * cursor;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !id) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    list_for_each(cursor, &internal_lib->os_list) {
        test_os = list_entry(cursor, struct osi_internal_os, list);
        if (strcmp(test_os->id, id) == 0) {
            os = test_os;
            break;
        }
    }

    if (!os) {
        *err = -EINVAL;
        return NULL;
    }

    return wrapped_os(os, err);
}

char** osi_get_all_os_property_keys(osi_os_t os, int* num, int* err)
{
    int i;
    char** property_keys;
    struct osi_internal_os * internal_os;
    struct osi_keyval_multi * kv;

    if (!err)
        return NULL;

    if (!osi_check_os(os) || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;
    property_keys = malloc(sizeof(*property_keys) * internal_os->num_params);
    if (!property_keys) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(kv, &internal_os->param_list, list) {
        if (i == internal_os->num_params)
            goto error; /* Should *never* happen */

        property_keys[i] = strdup(kv->key);
        if (!property_keys[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = internal_os->num_params;
    return property_keys;

error:
    for (i -= 1; i >= 0; i--)
       free(property_keys[i]);
    free(property_keys);
    *err = -ENOMEM;
    return NULL;
}

char** osi_get_os_property_all_values(osi_os_t os, char* propname, int* num, int* err)
{
    struct osi_internal_os * internal_os;

    if (!err)
        return NULL;

    if (!osi_check_os(os) || !propname || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;
    return os_property_all_values(internal_os, propname, num, err);
}

char* osi_get_os_property_first_value(osi_os_t os, char* propname, int* err)
{
    int i;
    char* value;
    struct osi_internal_os * internal_os;
    struct osi_keyval_multi * kv, * test_kv;
    struct list_head * cursor;
    struct osi_value * value_container;

    if (!err)
        return NULL;

    if (!osi_check_os(os) || !propname) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;

    /* First find the appropriate node based on propname */
    list_for_each(cursor, &internal_os->param_list) {
        test_kv = list_entry(cursor, struct osi_keyval_multi, list);
        if (strcmp(propname, test_kv->key) == 0) {
            kv = test_kv;
            break;
        }
    }
    if (!kv) {
       *err = -EINVAL;
       return NULL;
    }

    /* Found it, and there is at least one value set */
    cursor = kv->values_list.next;
    value_container = list_entry(cursor, struct osi_value, list);
    value = strdup(value_container->val);
    if (!value) {
        *err = -ENOMEM;
        return NULL;
    }
    return value;
}

osi_os_t osi_get_related_os(osi_os_t os, osi_relationship relationship, int* err)
{
    struct osi_internal_os * internal_os, * target_os;

    if (!err)
        return NULL;

    if (!osi_check_os(os) || !osi_valid_os_relationship(relationship)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;
    target_os = get_related_os(internal_os, relationship, err);
    if (*err != 0)
        return NULL;

    return wrapped_os(target_os, err);
}
