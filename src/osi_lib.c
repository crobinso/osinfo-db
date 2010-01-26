#include <libosinfo.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "list.h"
#include "osi_internal_types.h"

osi_lib_t osi_get_lib_handle(int* err, char* data_dir)
{
    int ret;
    struct osi_generic_type * external_lib;
    struct osi_internal_lib * internal_lib;

    /* If err is NULL, return NULL but we can't return the error... */
    if (!err)
        return NULL;

    /* Allocate structures */
    external_lib = malloc(sizeof(*external_lib));
    internal_lib = malloc(sizeof(*internal_lib));
    data_dir = strdup(data_dir);
    if (!external_lib || !internal_lib || !data_dir) {
        *err = -ENOMEM;
        free(external_lib);
        free(internal_lib);
        free(data_dir);
        return NULL;
    }

    /* Initial setup of structures */
    internal_lib->lib_state = UNINITIALIZED;
    internal_lib->libvirt_version = NULL;
    internal_lib->backing_data_location = data_dir;
    internal_lib->lib_hypervisor = NULL;

    INIT_LIST_HEAD(&internal_lib->param_list);
    internal_lib->num_params = 0;

    INIT_LIST_HEAD(&internal_lib->hypervisor_list);
    internal_lib->num_hypervisors = 0;

    INIT_LIST_HEAD(&internal_lib->device_list);
    internal_lib->num_devices = 0;

    INIT_LIST_HEAD(&internal_lib->os_list);
    internal_lib->num_os = 0;

    external_lib->backing_object = internal_lib;
    external_lib->initialized = 0;
    external_lib->type = OSI_LIB_T;
    external_lib->error = 0;

    /* Read in the backing data */
    ret = osi_initialize_data(internal_lib, data_dir);
    if (ret != 0) {
        *err = ret;
        free(external_lib);
        /* Don't free internal lib since it was cleaned up for us on failure */
        return NULL;
    }

    /* Success, return the handle */
    *err = 0;
    return external_lib;
}

int osi_init_lib(osi_lib_t lib)
{
    struct osi_internal_lib * internal_lib;

    if (!osi_check_lib(lib) || lib->initialized)
        return -EINVAL;

    internal_lib = lib->backing_object;
    internal_lib->lib_state = INITIALIZED;
    lib->initialized = 1;

    return 0;
}

int osi_close_lib(osi_lib_t lib)
{
    struct osi_generic_type * external_lib;
    struct osi_internal_lib * internal_lib;
    struct osi_keyval * pair, * tmp;

    if (!osi_check_lib(lib) || !lib->initialized)
        return -EINVAL;

    external_lib = lib;
    internal_lib = external_lib->backing_object;

    if (internal_lib->lib_state != INITIALIZED)
        return -EINVAL;

    __osi_free_lib(internal_lib);
    free(external_lib);

    return 0;
}

int osi_set_lib_param(osi_lib_t lib, char* key, char* val)
{
    struct osi_internal_lib * internal_lib;
    struct osi_keyval * pair, * test_pair;
    char * key_copy, * val_copy;
    struct list_head * cursor;

    if (!osi_check_lib(lib) || !key)
        return -EINVAL;

    internal_lib = lib->backing_object;
    if (lib->initialized || internal_lib->lib_state == INITIALIZED)
        return -EBUSY;

    /* If val is null, then remove value for given key. Else,
     * make copy of the value so we can set it for the given key.
     */
    if (!val)
        val_copy = NULL;
    else {
        val_copy = strdup(val);
        if (!val_copy)
            return -ENOMEM;
    }

    /* Short circuit for libvirt version */
    if (strcmp(key, "libvirt-version") == 0) {
        free(internal_lib->libvirt_version);
        internal_lib->libvirt_version = val_copy;
        return 0;
    }

    /* Short circuit for backing data location */
    if (strcmp(key, "backing-data-location") == 0) {
        free(internal_lib->backing_data_location);
        internal_lib->backing_data_location = val_copy;
        return 0;
    }

    /* Find the existing node containing the property if it exists */
    pair = NULL;
    list_for_each(cursor, &internal_lib->param_list) {
        test_pair = list_entry(cursor, struct osi_keyval, list);
        if (strcmp(key, test_pair->key) == 0) {
            pair = test_pair;
            break;
        }
    }

    /* If we were deleting the property, go ahead and remove the key/val pair */
    if (!val_copy) {

        if (!pair) /* Deleting value that was never there, so return success */
            return 0;

        /* Remove the pair */
        list_del(&pair->list);
        free(pair->key);
        free(pair->value);
        free(pair);

        internal_lib->num_params -= 1;

        return 0;
    }

    /* Else we are adding or updating the value for the property */

    /* If we did not find an existing value, make room for the new one */
    if (!pair) {
        pair = malloc(sizeof(*pair));
        key_copy = strdup(key);

        if (!key_copy || !pair) {
            free(key_copy);
            free(val_copy);
            free(pair);
            return -ENOMEM;
        }

        pair->key = key_copy;
        list_add(&pair->list, &internal_lib->param_list);
        internal_lib->num_params += 1;
    }

    /* Free the existing value and set the new one */
    free(pair->value);
    pair->value = val_copy;

    return 0;
}

char* osi_get_lib_param(osi_lib_t lib, char* key, int* err)
{
    int ret;
    struct osi_internal_lib * internal_lib;
    struct osi_keyval * pair, * test_pair;
    struct list_head * cursor;
    char* value;

    if (!err)
        return NULL;

    if (!osi_check_lib(lib) || !key) {
        *err = -EINVAL;
        return NULL;
    }

    internal_lib = lib->backing_object;

    /* Short circuit for libvirt version */
    if (strcmp(key, "libvirt-version") == 0) {
        if (!internal_lib->libvirt_version) {
            *err = 0;
            return NULL;
        }
        value = strdup(internal_lib->libvirt_version);
        if (!value) {
            *err = -ENOMEM;
            return NULL;
        }
        *err = 0;
        return value;
    }

    /* Short circuit for backing data location */
    if (strcmp(key, "backing-data-location") == 0) {
        if (!internal_lib->backing_data_location) {
            *err = 0;
            return NULL;
        }
        value = strdup(internal_lib->backing_data_location);
        if (!value) {
            *err = -ENOMEM;
            return NULL;
        }
        *err = 0;
        return value;
    }

    /* Find the existing node containing the property if it exists */
    pair = NULL;
    list_for_each(cursor, &internal_lib->param_list) {
        test_pair = list_entry(cursor, struct osi_keyval, list);
        if (strcmp(key, test_pair->key) == 0) {
            pair = test_pair;
            break;
        }
    }

    /* Value not found for given key */
    if (!pair) {
        *err = 0;
        return NULL;
    }

    /* Duplicate value and return */
    value = strdup(pair->value);
    if (!value) {
        *err = -ENOMEM;
        return NULL;
    }

    *err = 0;
    return value;
}

int osi_is_error(osi_generic_t t)
{
    if (!t)
        return -EINVAL;
    return t->error;
}

int osi_is_null(osi_generic_t t)
{
    return (t == NULL);
}

int osi_cleanup_handle(osi_generic_t t)
{
    if (!t)
        return -EINVAL;
    free(t);
    return 0;
}

int osi_check_lib(osi_lib_t lib)
{
    return (lib && lib->type == OSI_LIB_T && lib->backing_object);
}

int osi_check_os(osi_os_t os)
{
    return (os && os->type == OSI_OS_T && os->backing_object);
}

int osi_check_hv(osi_hypervisor_t hv)
{
    return (hv && hv->type == OSI_HYPERVISOR_T && hv->backing_object);
}

int osi_check_filter(osi_filter_t filter)
{
    return (filter && filter->type == OSI_FILTER_T && filter->backing_object);
}

int osi_check_os_list(osi_os_list_t list)
{
    return (list && list->type == OSI_OS_LIST_T && list->backing_object);
}

int osi_check_dev_list(osi_device_list_t list)
{
    return (list && list->type == OSI_DEVICE_LIST_T && list->backing_object);
}

int osi_check_dev(osi_device_t dev)
{
    return (dev && dev->type == OSI_DEVICE_T && dev->backing_object);
}

int osi_valid_os_relationship(osi_relationship relationship)
{
    return (relationship == DERIVES_FROM ||
            relationship == UPGRADES ||
            relationship == CLONES);
}
