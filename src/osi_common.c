#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libosinfo.h>
#include "list.h"
#include "osi_internal_types.h"

void __osi_free_keyval(struct osi_keyval_multi * kv)
{
    struct list_head * cursor, * tmp;
    struct osi_value * value;

    if (!kv)
        return;

    list_for_each_safe(cursor, tmp, &kv->values_list) {
        value = list_entry(cursor, struct osi_value, list);
        list_del(cursor);
        free(value->val);
        free(value);
    }
    free(kv->key);
    free(kv);
}

struct osi_keyval_multi * __osi_find_kv(char* key, struct list_head * search_list)
{
    struct list_head * cursor;
    struct osi_keyval_multi * kv;

    if (!key || !search_list)
        return NULL;

    list_for_each (cursor, search_list) {
        kv = list_entry(cursor, struct osi_keyval_multi, list);
        if (strcmp(key, kv->key) == 0)
            return kv;
    }
    return NULL;
}

struct osi_keyval_multi * __osi_create_kv(char* key, struct list_head * dest_list)
{
    struct osi_keyval_multi * kv;
    char* kv_key;

    if (!key || !dest_list)
        return NULL;

    kv = malloc(sizeof(*kv));
    kv_key = strdup(key);
    if (!kv || !kv_key) {
        free(kv);
        free(kv_key);
        return NULL;
    }

    kv->key = kv_key;
    kv->num_values = 0;
    INIT_LIST_HEAD(&kv->values_list);
    list_add_tail(&kv->list, dest_list);

    return kv;
}

struct osi_keyval_multi * __osi_find_or_create_kv(char* key, struct list_head * list, int* num)
{
    struct osi_keyval_multi * kv = __osi_find_kv(key, list);
    if (!kv) {
        kv = __osi_create_kv(key, list);
        *num += 1;
    }
    return kv;
}

int __osi_add_keyval_multi_value(struct osi_keyval_multi * kv, char* value)
{
    char* val_copy;
    struct osi_value * value_container;

    if (!kv || !value)
        return -EINVAL;

    val_copy = strdup(value);
    value_container = malloc(sizeof(*value_container));
    if (!val_copy || !value_container) {
        free(val_copy);
        free(value_container);
        return -ENOMEM;
    }

    value_container->val = val_copy;
    list_add_tail(&value_container->list, &kv->values_list);
    kv->num_values += 1;
    return 0;
}

int __osi_store_keyval(char* key, char* value, struct list_head * list, int* num)
{
    struct osi_keyval_multi * kv;

    if (!key || !value || !list || !num)
        return -EINVAL;

    kv = __osi_find_or_create_kv(key, list, num);
    if (!kv)
        return -ENOMEM;

    return __osi_add_keyval_multi_value(kv, value);
}

void __osi_free_kv_pair(struct osi_keyval * kv_pair)
{
    if (!kv_pair)
        return;
    free(kv_pair->key);
    free(kv_pair->value);
    free(kv_pair);
}

void __osi_free_device_section(struct osi_device_section * section)
{
    struct list_head * cursor, * tmp;
    struct osi_device_link * dev_link;

    if (!section)
        return;
    list_for_each_safe(cursor, tmp, &section->devices_list) {
        dev_link = list_entry(cursor, struct osi_device_link, list);
        list_del(cursor);
        free(dev_link->driver);
        free(dev_link->id);
        free(dev_link);
    }
    free(section->type);
    free(section);
}

void __osi_free_hv(struct osi_internal_hv * hv)
{
    struct list_head * cursor, * tmp;
    struct osi_device_section * section;
    struct osi_keyval_multi * kv;

    if (!hv)
        return;

    list_for_each_safe(cursor, tmp, &hv->param_list) {
        kv = list_entry(cursor, struct osi_keyval_multi, list);
        list_del(cursor);
        __osi_free_keyval(kv);
    }

    list_for_each_safe(cursor, tmp, &hv->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        list_del(cursor);
        __osi_free_device_section(section);
    }

    free(hv->id);
    free(hv);
}

void __osi_free_device(struct osi_internal_dev * dev)
{
    struct list_head * cursor, * tmp;
    struct osi_keyval_multi * kv;

    if (!dev)
        return;

    list_for_each_safe(cursor, tmp, &dev->param_list) {
        kv = list_entry(cursor, struct osi_keyval_multi, list);
        list_del(&kv->list);
        __osi_free_keyval(kv);
    }
    free(dev->id);
    free(dev);
}

void __osi_free_hypervisor_link(struct osi_hypervisor_link * hv_link)
{
    struct list_head * cursor, * tmp;
    struct osi_device_section * section;

    if (!hv_link)
        return;

    list_for_each_safe(cursor, tmp, &hv_link->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        list_del(cursor);
        __osi_free_device_section(section);
    }
    free(hv_link->hv_id);
    free(hv_link);
}

void __osi_free_os(struct osi_internal_os * os)
{
    struct list_head * cursor, * tmp;
    struct osi_device_section * section;
    struct osi_keyval_multi * kv;
    struct osi_hypervisor_link * hv_link;
    struct osi_os_link * os_link;

    if (!os)
        return;

    list_for_each_safe(cursor, tmp, &os->param_list) {
        kv = list_entry(cursor, struct osi_keyval_multi, list);
        list_del(&kv->list);
        __osi_free_keyval(kv);
    }

    list_for_each_safe(cursor, tmp, &os->hypervisor_info_list) {
        hv_link = list_entry(cursor, struct osi_hypervisor_link, list);
        list_del(cursor);
        __osi_free_hypervisor_link(hv_link);
    }

    list_for_each_safe(cursor, tmp, &os->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        list_del(cursor);
        __osi_free_device_section(section);
    }

    list_for_each_safe(cursor, tmp, &os->relationships_list) {
        os_link = list_entry(cursor, struct osi_os_link, list);
        list_del(cursor);
        __osi_free_os_link(os_link);
    }

    free(os->id);
    free(os);
}

void __osi_free_lib(struct osi_internal_lib * lib)
{
    struct list_head * cursor, * tmp;
    struct osi_keyval * kv_pair;
    struct osi_internal_hv * hv;
    struct osi_internal_dev * dev;
    struct osi_internal_os * os;

    if (!lib)
        return;

    list_for_each_safe(cursor, tmp, &lib->param_list) {
        kv_pair = list_entry(cursor, struct osi_keyval, list);
        list_del(&kv_pair->list);
        __osi_free_kv_pair(kv_pair);
    }

    list_for_each_safe(cursor, tmp, &lib->hypervisor_list) {
        hv = list_entry(cursor, struct osi_internal_hv, list);
        list_del(&hv->list);
        __osi_free_hv(hv);
    }
    /* Don't free lib->lib_hypervisor since we've already freed all of them */

    list_for_each_safe(cursor, tmp, &lib->device_list) {
        dev = list_entry(cursor, struct osi_internal_dev, list);
        list_del(&dev->list);
        __osi_free_device(dev);
    }

    list_for_each_safe(cursor, tmp, &lib->os_list) {
        os = list_entry(cursor, struct osi_internal_os, list);
        list_del(&os->list);
        __osi_free_os(os);
    }

    free(lib->libvirt_version);
    free(lib->backing_data_location);
    free(lib);
}

void __osi_cleanup_hv_link(struct osi_hypervisor_link * hv_link)
{
    struct list_head * cursor, * tmp;
    struct osi_device_section * section;

    list_for_each_safe(cursor, tmp, &hv_link->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        list_del(cursor);
        __osi_free_device_section(section);
    }
    free(hv_link->hv_id);
    free(hv_link);
}

struct osi_internal_dev * __osi_find_dev_by_id(struct osi_internal_lib * lib,
                                               char* id)
{
    struct list_head * cursor;
    struct osi_internal_dev * dev = NULL, * test_dev;

    if (!lib)
        return NULL;

    list_for_each(cursor, &lib->device_list) {
        test_dev = list_entry(cursor, struct osi_internal_dev, list);
        if (strcmp(id, test_dev->id) == 0) {
            dev = test_dev;
            break;
        }
    }

    return dev;
}

struct osi_internal_hv * __osi_find_hv_by_id(struct osi_internal_lib * lib,
                                             char* id)
{
    struct list_head * cursor;
    struct osi_internal_hv * hv = NULL, * test_hv;

    if (!lib)
        return NULL;

    list_for_each(cursor, &lib->hypervisor_list) {
        test_hv = list_entry(cursor, struct osi_internal_hv, list);
        if (strcmp(id, test_hv->id) == 0) {
            hv = test_hv;
            break;
        }
    }

    return hv;
}

struct osi_internal_os * __osi_find_os_by_id(struct osi_internal_lib * lib,
                                             char* id)
{
    struct list_head * cursor;
    struct osi_internal_os * os = NULL, * test_os;

    if (!lib)
        return NULL;

    list_for_each(cursor, &lib->os_list) {
        test_os = list_entry(cursor, struct osi_internal_os, list);
        if (strcmp(id, test_os->id) == 0) {
            os = test_os;
            break;
        }
    }

    return os;
}

void __osi_free_os_link(struct osi_os_link * os_link)
{
    if (!os_link)
        return;

    free(os_link->dobj_os_id);
    free(os_link);
}

void __osi_free_filter_relationship(struct osi_filter_relationship * fr)
{
    free(fr);
}
