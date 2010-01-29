#include <libosinfo.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "list.h"
#include "osi_internal_types.h"

char** osi_get_all_hypervisor_ids(osi_lib_t lib, int* num, int* err)
{
    int i;
    char** ids;
    struct osi_internal_hv * hv;
    struct osi_internal_lib * internal_lib;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    ids = malloc(sizeof(*ids) * internal_lib->num_hypervisors);
    if (!ids) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(hv, &internal_lib->hypervisor_list, list) {
        if (i == internal_lib->num_hypervisors)
            goto error; /* This should *never* happen */
        ids[i] = strdup(hv->id);
        if (!ids[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = internal_lib->num_hypervisors;
    return ids;

error:
    for (i -= 1; i >= 0; i--) {
        free(ids[i-1]);
    }
    free(ids);
    *err = -ENOMEM;
    return NULL;
}

osi_hypervisor_t osi_get_hypervisor_by_id(osi_lib_t lib, char* hypervisor_id, int* err)
{
    struct osi_generic_type * hv_container;
    struct osi_internal_hv * hv, * test_hv;
    struct list_head * cursor;
    struct osi_internal_lib * internal_lib;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !hypervisor_id) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    hv = NULL;
    list_for_each(cursor, &internal_lib->hypervisor_list) {
        test_hv = list_entry(cursor, struct osi_internal_hv, list);
        if (strcmp(hypervisor_id, test_hv->id) == 0) {
            hv = test_hv;
            break;
        }
    }

    /* Hypervisor not found */
    if (!hv) {
        *err = -EINVAL;
        return NULL;
    }

    hv_container = malloc(sizeof(*hv_container));
    if (!hv_container) {
        *err = -ENOMEM;
        return NULL;
    }

    hv_container->backing_object = hv;
    hv_container->type = OSI_HYPERVISOR_T;
    hv_container->error = 0;
    hv_container->initialized = 1;

    *err = 0;
    return hv_container;
}

char* osi_get_hv_id(osi_hypervisor_t hv, int* err)
{
    struct osi_internal_hv * internal_hv;
    char* id;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;

    id = strdup(internal_hv->id);
    if (!id) {
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return id;
}

int osi_set_lib_hypervisor(osi_lib_t lib, char* hypervisor_id)
{
    struct osi_internal_hv * hv, * test_hv;
    struct list_head * cursor;
    struct osi_internal_lib * internal_lib;

    if (!osi_check_lib(lib))
        return -EINVAL;

    if (lib->initialized)
        return -EBUSY;

    internal_lib = lib->backing_object;

    /* If hv id is NULL, unset any existing hv so lib uses 'default' hv */
    if (!hypervisor_id) {
        internal_lib->lib_hypervisor = NULL;
        return 0;
    }

    /* Otherwise find hypervisor and set accordingly, or error if not found */
    hv = NULL;
    list_for_each(cursor, &internal_lib->hypervisor_list) {
        test_hv = list_entry(cursor, struct osi_internal_hv, list);
        if (strcmp(hypervisor_id, test_hv->id) == 0) {
            hv = test_hv;
            break;
        }
    }

    if (!hv)
        return -EINVAL;

    internal_lib->lib_hypervisor = hv;
    return 0;
}

osi_hypervisor_t osi_get_lib_hypervisor(osi_lib_t lib, int* err)
{
    struct osi_generic_type * hv_container;
    struct osi_internal_lib * internal_lib;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    if (!internal_lib->lib_hypervisor) {
        *err = 0;
        return NULL;
    }

    hv_container = malloc(sizeof(*hv_container));
    if (!hv_container) {
        *err = -ENOMEM;
        return NULL;
    }

    hv_container->backing_object = internal_lib->lib_hypervisor;
    hv_container->type = OSI_HYPERVISOR_T;
    hv_container->error = 0;
    hv_container->initialized = 1;

    *err = 0;
    return hv_container;
}

char** osi_get_hv_device_types(osi_hypervisor_t hv, int* num, int* err)
{
    int i;
    struct osi_internal_hv * internal_hv;
    struct osi_device_section * dev_section;
    char** device_types;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv) || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;
    device_types = malloc(sizeof(*device_types) * internal_hv->num_dev_sections);
    if (!device_types) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(dev_section, &internal_hv->dev_sections_list, list) {
        if (i == internal_hv->num_dev_sections)
            goto error; /* Should *never* happen */

        device_types[i] = strdup(dev_section->type);
        if (!device_types[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = internal_hv->num_dev_sections;

    return device_types;

error:
    for (i -= 1; i >= 0; i--)
       free(device_types[i]);
    free(device_types);
    *err = -ENOMEM;
    return NULL;
}


char** osi_get_hv_all_property_keys(osi_hypervisor_t hv, int* num, int* err)
{
    int i;
    char** property_keys;
    struct osi_internal_hv * internal_hv;
    struct osi_keyval_multi * kv;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv) || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;
    property_keys = malloc(sizeof(*property_keys) * internal_hv->num_params);
    if (!property_keys) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(kv, &internal_hv->param_list, list) {
        if (i == internal_hv->num_params)
            goto error; /* Should *never* happen */

        property_keys[i] = strdup(kv->key);
        if (!property_keys[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = internal_hv->num_params;
    return property_keys;

error:
    for (i -= 1; i >= 0; i--)
       free(property_keys[i]);
    free(property_keys);
    *err = -ENOMEM;
    return NULL;
}

char** osi_get_hv_property_all_values(osi_hypervisor_t hv, char* propname, int* num, int* err)
{
    int i;
    char** property_values;
    struct osi_internal_hv * internal_hv;
    struct osi_keyval_multi * kv, * test_kv;
    struct list_head * cursor;
    struct osi_value * value;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv) || !propname || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;

    /* First find the appropriate node based on propname */
    list_for_each(cursor, &internal_hv->param_list) {
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


char* osi_get_hv_property_first_value(osi_hypervisor_t hv, char* propname, int* err)
{
    char* value;
    struct osi_internal_hv * internal_hv;
    struct osi_keyval_multi * kv, * test_kv;
    struct list_head * cursor;
    struct osi_value * value_container;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv) || !propname) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;

    /* First find the appropriate node based on propname */
    list_for_each(cursor, &internal_hv->param_list) {
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
