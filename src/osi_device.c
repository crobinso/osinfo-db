#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libosinfo.h>
#include "list.h"
#include "osi_internal_types.h"

static struct list_head * get_device_section_list(struct osi_internal_os * os)
{
    struct osi_hypervisor_link * hv_link;

    /* Check if hypervisor is set and if os has specific rules for the hv */
    if (os->lib->lib_hypervisor)
        list_for_each_entry(hv_link, &os->hypervisor_info_list, list) {
            if (hv_link->hv == os->lib->lib_hypervisor)
                return &hv_link->dev_sections_list;
        }

    /* Either no hv set for lib, or specific rules for hv not found for os */
    return &os->dev_sections_list;
}

static char** dev_property_all_values(struct osi_internal_dev * dev,
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
    list_for_each(cursor, &dev->param_list) {
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

static osi_device_list_t wrapped_dev_list(struct osi_dev_list * dev_list)
{
    struct osi_generic_type * dev_list_container;

    dev_list_container = malloc(sizeof(*dev_list_container));
    if (!dev_list_container) {
        return NULL;
    }

    dev_list_container->backing_object = dev_list;
    dev_list_container->initialized = 1;
    dev_list_container->type = OSI_DEVICE_LIST_T;
    dev_list_container->error = 0;

    return dev_list_container;
}

static int dev_passes_filter(struct osi_internal_dev * dev,
                             struct osi_internal_filter * filter)
{
    struct osi_keyval * kv;
    char** values;
    int num, err, i, match;

    /* Any device passes the null filter */
    if (!filter)
        return 1;

    /* Check if it passes all normal constraints */
    list_for_each_entry(kv, &filter->constraints_list, list) {
        values = dev_property_all_values(dev, kv->key, &num, &err);
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

    /* And if it does, then it passes the filter */
    return 1;
}

osi_device_list_t osi_hypervisor_devices(osi_hypervisor_t hv, char* type, osi_filter_t filter, int* err)
{
    int len;
    struct osi_generic_type * dev_list_container;
    struct osi_dev_list * dev_list;
    struct osi_internal_dev ** dev_refs, * dev;
    struct osi_internal_hv * internal_hv;
    struct osi_internal_filter * internal_filter;
    struct osi_device_section * dev_section = NULL, * test_dev_section;
    struct osi_device_link * device_link;
    struct list_head * cursor;

    if (!err)
        return NULL;

    if (!osi_check_hv(hv) ||
        !type ||
        (filter!= NULL && !osi_check_filter(filter)) ) {
        *err = -EINVAL;
        return NULL;
    }

    internal_hv = hv->backing_object;

    if (filter != NULL)
        internal_filter = filter->backing_object;
    else
        internal_filter = NULL;

    len = 0;

    /* Find the device section */
    list_for_each(cursor, &internal_hv->dev_sections_list) {
        test_dev_section = list_entry(cursor, struct osi_device_section, list);
        if (strcmp(test_dev_section->type, type) == 0) {
            dev_section = test_dev_section;
            break;
        }
    }
    /* Could not find the section for the given device type */
    if (!dev_section) {
        *err = -EINVAL;
        return NULL;
    }

    dev_list = malloc(sizeof(*dev_list));
    dev_refs = malloc(sizeof(*dev_refs) * dev_section->num_devices);
    if (!dev_list || !dev_refs) {
        free(dev_list);
        free(dev_refs);
        *err = -ENOMEM;
        return NULL;
    }

    /* Iterate dev list and check against filter (adding to list if passed) */
    list_for_each_entry(device_link, &dev_section->devices_list, list) {
        dev = device_link->dev;
        if (dev_passes_filter(dev, internal_filter)) {
            dev_refs[len] = dev;
            len += 1;
        }
    }

    /* Resize the list so we're not wasting memory */
    dev_refs = realloc(dev_refs, sizeof(*dev_refs) * len);
    if (!dev_refs) {
        free(dev_list);
        *err = -ENOMEM;
        return NULL;
    }

    dev_list->dev_refs = dev_refs;
    dev_list->length = len;
    dev_list_container = wrapped_dev_list(dev_list);
    if (!dev_list_container) {
        free(dev_list);
        free(dev_refs);
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return dev_list_container;
}

osi_device_list_t osi_os_devices(osi_os_t os, char* type, osi_filter_t filter, int* err)
{
    int len;
    struct osi_generic_type * dev_list_container;
    struct osi_dev_list * dev_list;
    struct osi_internal_dev ** dev_refs, * dev;
    struct osi_internal_os * internal_os;
    struct osi_internal_filter * internal_filter;
    struct osi_device_section * dev_section = NULL, * test_dev_section;
    struct osi_device_link * device_link;
    struct list_head * cursor, * dev_sections_list = NULL;
    struct osi_hypervisor_link * hv_link;

    if (!err)
        return NULL;

    if (!osi_check_os(os) ||
        !type ||
        (filter!= NULL && !osi_check_filter(filter)) ) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;

    if (filter != NULL)
        internal_filter = filter->backing_object;
    else
        internal_filter = NULL;

    len = 0;

    /* Ensure we check best possible data list for <os, hv> */
    dev_sections_list = get_device_section_list(internal_os);

    /* Find the device section */
    list_for_each(cursor, dev_sections_list) {
        test_dev_section = list_entry(cursor, struct osi_device_section, list);
        if (strcmp(test_dev_section->type, type) == 0) {
            dev_section = test_dev_section;
            break;
        }
    }
    /* Could not find the section for the given device type */
    if (!dev_section) {
        *err = -EINVAL;
        return NULL;
    }

    dev_list = malloc(sizeof(*dev_list));
    dev_refs = malloc(sizeof(*dev_refs) * dev_section->num_devices);
    if (!dev_list || !dev_refs) {
        free(dev_list);
        free(dev_refs);
        *err = -ENOMEM;
        return NULL;
    }

    /* Iterate dev list and check against filter (adding to list if passed) */
    list_for_each_entry(device_link, &dev_section->devices_list, list) {
        dev = device_link->dev;
        if (dev_passes_filter(dev, internal_filter)) {
            dev_refs[len] = dev;
            len += 1;
        }
    }

    /* Resize the list so we're not wasting memory */
    dev_refs = realloc(dev_refs, sizeof(*dev_refs) * len);
    if (!dev_refs) {
        free(dev_list);
        *err = -ENOMEM;
        return NULL;
    }

    dev_list->dev_refs = dev_refs;
    dev_list->length = len;
    dev_list_container = wrapped_dev_list(dev_list);
    if (!dev_list_container) {
        free(dev_list);
        free(dev_refs);
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return dev_list_container;
}

int osi_free_devices_list(osi_device_list_t list)
{
    struct osi_dev_list * dev_list;

    if (!osi_check_dev_list(list))
        return -EINVAL;

    dev_list = list->backing_object;

    free(dev_list->dev_refs);
    free(dev_list);
    free(list);
    return 0;
}

int osi_devices_list_length(osi_device_list_t list)
{
    struct osi_dev_list * dev_list;

    if (!osi_check_dev_list(list))
        return -EINVAL;

    dev_list = list->backing_object;
    return dev_list->length;
}

static osi_device_t wrapped_dev(struct osi_internal_dev * dev, int* err)
{
    struct osi_generic_type * dev_container;

    dev_container = malloc(sizeof(*dev_container));
    if (!dev_container) {
        *err = -ENOMEM;
        return NULL;
    }

    dev_container->backing_object = dev;
    dev_container->initialized = 1;
    dev_container->type = OSI_DEVICE_T;
    dev_container->error = 0;

    *err = 0;
    return dev_container;
}

osi_device_t osi_get_device_by_index(osi_device_list_t list, int index, int* err)
{
    struct osi_dev_list * dev_list;
    struct osi_internal_dev * dev;

    if (!err)
        return NULL;

    if (!osi_check_dev_list(list)) {
        *err = -EINVAL;
        return NULL;
    }

    dev_list = list->backing_object;

    /* Check bounds */
    if (index < 0 || index >= dev_list->length) {
        *err = -EINVAL;
        return NULL;
    }

    dev = dev_list->dev_refs[index];
    return wrapped_dev(dev, err);
}

osi_device_t osi_get_preferred_device(osi_os_t os, char* type, osi_filter_t filter, int* err)
{
    struct osi_internal_dev * dev = NULL, * test_dev;
    struct osi_internal_os * internal_os;
    struct osi_internal_filter * internal_filter;
    struct osi_device_section * dev_section = NULL, * test_dev_section;
    struct osi_device_link * device_link;
    struct list_head * cursor, * dev_sections_list;

    if (!err)
        return NULL;

    if (!osi_check_os(os) ||
        !type ||
        (filter!= NULL && !osi_check_filter(filter)) ) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;

    if (filter != NULL)
        internal_filter = filter->backing_object;
    else
        internal_filter = NULL;

    /* Ensure we check best possible data list for <os, hv> */
    dev_sections_list = get_device_section_list(internal_os);

    /* Find the device section */
    list_for_each(cursor, dev_sections_list) {
        test_dev_section = list_entry(cursor, struct osi_device_section, list);
        if (strcmp(test_dev_section->type, type) == 0) {
            dev_section = test_dev_section;
            break;
        }
    }
    /* Could not find the section for the given device type */
    if (!dev_section) {
        *err = -EINVAL;
        return NULL;
    }

    /* Iterate dev list and check against filter (adding to list if passed) */
    list_for_each_entry(device_link, &dev_section->devices_list, list) {
        test_dev = device_link->dev;
        if (dev_passes_filter(test_dev, internal_filter)) {
            dev = test_dev;
            break;
        }
    }

    if (!dev) {
        *err = -EINVAL;
        return NULL;
    }

    return wrapped_dev(dev, err);
}

char* osi_get_device_driver(osi_device_t dev, char* type, osi_os_t os, int* err)
{
    /* Search class, for the given hv, for the device */
    /* If not found, return NULL (no driver) for given class */
    /* Else return driver */

    char* driver;
    struct osi_internal_dev * internal_dev;
    struct osi_internal_os * internal_os;
    struct osi_device_section * dev_section = NULL, * test_dev_section;
    struct osi_device_link * device_link = NULL, *test_device_link;
    struct list_head * cursor, * dev_sections_list;

    if (!err)
        return NULL;

    if (!osi_check_os(os) || !osi_check_dev(dev) || !type) {
        *err = -EINVAL;
        return NULL;
    }

    internal_os = os->backing_object;
    internal_dev = dev->backing_object;

    /* Ensure we check best possible data list for <os, hv> */
    dev_sections_list = get_device_section_list(internal_os);

    /* Find the device section */
    list_for_each(cursor, dev_sections_list) {
        test_dev_section = list_entry(cursor, struct osi_device_section, list);
        if (strcmp(test_dev_section->type, type) == 0) {
            dev_section = test_dev_section;
            break;
        }
    }
    /* Could not find the section for the given device type */
    if (!dev_section) {
        *err = -EINVAL;
        return NULL;
    }

    /* Iterate dev list to find device link with driver */
    list_for_each_entry(test_device_link, &dev_section->devices_list, list) {
        if (test_device_link->dev == internal_dev) {
            device_link = test_device_link;
            break;
        }
    }
    if (!device_link) {
        *err = -EINVAL;
        return NULL;
    }

    driver = strdup(device_link->driver);
    if (!driver) {
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return driver;
}

char* osi_device_id(osi_device_t dev, int* err)
{
    struct osi_internal_dev * internal_dev;
    char* id;

    if (!err)
        return NULL;

    if (!osi_check_dev(dev)) {
        *err = -EINVAL;
        return NULL;
    }

    internal_dev = dev->backing_object;

    id = strdup(internal_dev->id);
    if (!id) {
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return id;
}

osi_device_t osi_get_device_by_id(osi_lib_t lib, char* device_id, int* err)
{
    struct osi_generic_type * dev_container;
    struct osi_internal_dev * dev, * test_dev;
    struct list_head * cursor;
    struct osi_internal_lib * internal_lib;
    int i;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !device_id) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    dev = NULL;
    list_for_each(cursor, &internal_lib->device_list) {
        test_dev = list_entry(cursor, struct osi_internal_dev, list);
        if (strcmp(device_id, test_dev->id) == 0) {
            dev = test_dev;
            break;
        }
    }

    /* Hypervisor not found */
    if (!dev) {
        *err = -EINVAL;
        return NULL;
    }

    dev_container = malloc(sizeof(*dev_container));
    if (!dev_container) {
        *err = -ENOMEM;
        return NULL;
    }

    dev_container->backing_object = dev;
    dev_container->type = OSI_DEVICE_T;
    dev_container->error = 0;
    dev_container->initialized = 1;

    *err = 0;
    return dev_container;
}

char** osi_get_all_device_property_keys(osi_device_t dev, int* num, int* err)
{
    int i;
    char** property_keys;
    struct osi_internal_dev * internal_dev;
    struct osi_keyval_multi * kv;

    if (!err)
        return NULL;

    if (!osi_check_dev(dev) || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_dev = dev->backing_object;
    property_keys = malloc(sizeof(*property_keys) * internal_dev->num_params);
    if (!property_keys) {
        *err = -ENOMEM;
        return NULL;
    }

    i = 0;
    list_for_each_entry(kv, &internal_dev->param_list, list) {
        if (i == internal_dev->num_params)
            goto error; /* Should *never* happen */

        property_keys[i] = strdup(kv->key);
        if (!property_keys[i])
            goto error;
        i += 1;
    }

    *err = 0;
    *num = internal_dev->num_params;
    return property_keys;

error:
    for (i -= 1; i > 0; i--)
       free(property_keys[i]);
    free(property_keys);
    *err = -ENOMEM;
    return NULL;
}

char** osi_get_device_property_all_values(osi_device_t dev, char* propname, int* num, int* err)
{
    struct osi_internal_dev * internal_dev;

    if (!err)
        return NULL;

    if (!osi_check_dev(dev) || !propname || !num) {
        *err = -EINVAL;
        return NULL;
    }

    internal_dev = dev->backing_object;
    return dev_property_all_values(internal_dev, propname, num, err);
}

char* osi_get_device_property_value(osi_device_t dev, char* propname, int* err)
{
    int i;
    char* value;
    struct osi_internal_dev * internal_dev;
    struct osi_keyval_multi * kv, * test_kv;
    struct list_head * cursor;
    struct osi_value * value_container;

    if (!err)
        return NULL;

    if (!osi_check_dev(dev) || !propname) {
        *err = -EINVAL;
        return NULL;
    }

    internal_dev = dev->backing_object;

    /* First find the appropriate node based on propname */
    list_for_each(cursor, &internal_dev->param_list) {
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
