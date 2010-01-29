#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <libosinfo.h>
#include <libxml/xmlreader.h>

#include "list.h"
#include "osi_internal_types.h"

#ifdef LIBXML_READER_ENABLED

#define TEXT_NODE 3
#define ELEMENT_NODE 1
#define END_NODE 15
#define WHITESPACE_NODE 14
#define COMMENT_NODE 8

/*
 * TODO:
 * 1. More robust handling of files that are in bad format
 * 2. Error messages for parsing
 */

/*
 * Notes regarding parsing XML Data:
 *
 * The top level tag is <libosinfo>. The next highest level consists of
 * <device>, <hypervisor> and <os>. Each tag may be empty (of the form <tag />)
 * or containing data (of the form <tag>...</tag>). After parsing an object, the
 * cursor will be located at the closing tag for the object (which, for an empty
 * object, is the same as the starting tag for the object).
 */


static int resolve_section_devices(struct osi_internal_lib * lib,
                                   struct osi_device_section * section)
{
    struct list_head * cursor;
    struct osi_device_link * dev_link;
    struct osi_internal_dev * dev;

    if (!section)
        return -EINVAL;

    list_for_each(cursor, &section->devices_list) {
        dev_link = list_entry(cursor, struct osi_device_link, list);
        dev = __osi_find_dev_by_id(lib, dev_link->id);
        if (!dev)
            return -EINVAL;
        dev_link->dev = dev;
    }

    return 0;
}

static int resolve_hv_link(struct osi_internal_lib * lib,
                           struct osi_hypervisor_link * hv_link)
{
    int ret;
    struct list_head * cursor;
    struct osi_device_section * section;
    struct osi_internal_hv * hv;

    list_for_each(cursor, &hv_link->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        ret = resolve_section_devices(lib, section);
        if (ret != 0)
            return ret;
    }

    hv = __osi_find_hv_by_id(lib, hv_link->hv_id);
    if (!hv)
        return -EINVAL;

    hv_link->hv = hv;
    return 0;
}

static int resolve_os_link(struct osi_internal_lib * lib,
                           struct osi_os_link * os_link)
{
    struct osi_internal_os * target_os;

    target_os = __osi_find_os_by_id(lib, os_link->dobj_os_id);
    if (!target_os)
        return -EINVAL;

    os_link->direct_object_os = target_os;
    return 0;
}

static int fix_os_links(struct osi_internal_lib * lib,
                        struct osi_internal_os * os)
{
    int ret;
    struct list_head * cursor;
    struct osi_device_section * section;
    struct osi_hypervisor_link * hv_link;
    struct osi_os_link * os_link;

    list_for_each(cursor, &os->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        ret = resolve_section_devices(lib, section);
        if (ret != 0)
            return ret;
    }

    list_for_each(cursor, &os->hypervisor_info_list) {
        hv_link = list_entry(cursor, struct osi_hypervisor_link, list);
        ret = resolve_hv_link(lib, hv_link);
        if (ret != 0)
            return ret;
    }

    list_for_each(cursor, &os->relationships_list) {
        os_link = list_entry(cursor, struct osi_os_link, list);
        ret = resolve_os_link(lib, os_link);
        if (ret != 0)
            return ret;
    }

    return 0;
}

static int fix_hv_links(struct osi_internal_lib * lib,
                        struct osi_internal_hv * hv)
{
    int ret;
    struct list_head * cursor;
    struct osi_device_section * section;

    list_for_each(cursor, &hv->dev_sections_list) {
        section = list_entry(cursor, struct osi_device_section, list);
        ret = resolve_section_devices(lib, section);
        if (ret != 0)
            return ret;
    }

    return 0;
}

static int fix_obj_links(struct osi_internal_lib * lib)
{
    struct list_head * cursor;
    struct osi_internal_hv * hv;
    struct osi_internal_os * os;
    int ret;

    if (!lib)
        return -EINVAL;

    list_for_each(cursor, &lib->hypervisor_list) {
        hv = list_entry(cursor, struct osi_internal_hv, list);
        ret = fix_hv_links(lib, hv);
        if (ret != 0)
            return ret;
    }

    list_for_each(cursor, &lib->os_list) {
        os = list_entry(cursor, struct osi_internal_os, list);
        ret = fix_os_links(lib, os);
        if (ret != 0)
            return ret;
    }

    return 0;
}

static int process_tag(xmlTextReaderPtr reader, char** ptr_to_key, char** ptr_to_val)
{
    int node_type, ret, err = 0;
    char* key = NULL;
    char* val = NULL;
    const xmlChar* node_name, * end_node_name, * xml_value;

    node_name = xmlTextReaderConstName(reader);
    if (!node_name)
        goto error;

    key = strdup(node_name);
    if (!key) {
        err = -ENOMEM;
        goto error;
    }

    /* Advance to next node */
    ret = xmlTextReaderRead(reader);
    if (ret != 1)
        goto error;

    /* Ensure node is a text node */
    node_type = xmlTextReaderNodeType(reader);
    if (node_type != TEXT_NODE)
        goto error;

    /* Store the value of the text node */
    xml_value = xmlTextReaderConstValue(reader);
    if (!xml_value)
        goto error;

    val = strdup(xml_value);
    if (!val) {
        err = -ENOMEM;
        goto error;
    }

    /* Advance to the next node */
    ret = xmlTextReaderRead(reader);
    if (ret != 1)
        goto error;

    /* Ensure the node is an end node for the tracked start node */
    node_type = xmlTextReaderNodeType(reader);
    end_node_name = xmlTextReaderConstName(reader);
    if (node_type != END_NODE ||
        !end_node_name ||
        strcmp(end_node_name, node_name) != 0)
            goto error;

    /* Now we have key and val with no errors so we return with success */
    *ptr_to_key = key;
    *ptr_to_val = val;
    return 0;

error:
    free(key);
    free(val);
    *ptr_to_key = NULL;
    *ptr_to_val = NULL;
    if (err == 0)
        err = -EINVAL;
    return err;
}

static int process_dev_section(xmlTextReaderPtr reader,
                               struct list_head * head,
                               int * num)
{
    int err, empty, node_type;
    char * type, * id, * key = NULL, * driver = NULL;
    const char * name;
    struct osi_device_section * section;
    struct osi_device_link * dev_link;

    type = xmlTextReaderGetAttribute(reader, "type");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!type)
        return -EINVAL;

    /* Alloc structure for section and store type */
    section = malloc(sizeof(*section));
    if (!section) {
        free(type);
        return -ENOMEM;
    }

    section->type = type;
    section->num_devices = 0;
    INIT_LIST_HEAD(&section->devices_list);

    /* If no devices in section then we are done */
    if (empty)
        goto finished;

    /* Otherwise, read in devices and add to section */
    for (;;) {
        /* Advance to next node */
        err = xmlTextReaderRead(reader);
        if (err != 1) {
            err = -EINVAL;
            goto error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);

        /* If end of section, break */
        if (node_type == END_NODE && strcmp(name, "section") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        /* Element within section needs to be of type device */
        if (strcmp(name, "device") != 0) {
            err = -EINVAL;
            goto error;
        }

        id = xmlTextReaderGetAttribute(reader, "id");
        empty = xmlTextReaderIsEmptyElement(reader);

        if (!id) {
            err = -EINVAL;
            goto error;
        }

        if (!empty) {
            err = process_tag(reader, &key, &driver);
            if (err != 0 || !key || !driver)
                goto error;
            free(key);
            key = NULL; /* In case the next malloc fails, avoid a double free */
        }

        /* Store link to device in structure */
        dev_link = malloc(sizeof(*dev_link));
        if (!dev_link) {
            err = -ENOMEM;
            free(id);
            goto error;
        }

        dev_link->id = id;
        dev_link->driver = driver;
        dev_link->dev = NULL;
        section->num_devices += 1;
        list_add_tail(&dev_link->list, &section->devices_list);
    }

finished:
    list_add_tail(&section->list, head);
    *num += 1;
    return 0;

error:
    free(key);
    free(driver);
    __osi_free_device_section(section);
    return err;
}

static int process_os_hv_link(xmlTextReaderPtr reader,
                              struct osi_internal_os *os)
{
    /*
     * Get id for hypervisor else fail
     * While true:
     *   Advance to next node
     *   If node is end of hypervisor break
     *   If node is not element type continue
     *   If node is element type and not section fail
     *   Else handle section and add to hv_link
     * If fail delete hv_link so far
     * On success add hv_link to os
     */
    int empty, node_type, err;
    char* id;
    const xmlChar* name;
    struct osi_hypervisor_link * hv_link;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    hv_link = malloc(sizeof(*hv_link));
    if (!hv_link) {
        free(id);
        return -EINVAL;
    }

    hv_link->hv_id = id;
    hv_link->os = os;
    hv_link->hv = NULL; /* Will resolve link later */
    hv_link->num_dev_sections = 0;
    INIT_LIST_HEAD(&hv_link->dev_sections_list);

    if (empty)
        goto finished;

    for (;;) {
        /* Advance to next node */
        err = xmlTextReaderRead(reader);
        if (err != 1) {
            err = -EINVAL;
            goto error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);
        if (node_type == -1 || !name) {
            err = -EINVAL;
            goto error;
        }

        /* If end of hv link, break */
        if (node_type == END_NODE && strcmp(name, "hypervisor") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        /* Ensure it is element node of type 'section' else fail */
        if (strcmp(name, "section") != 0) {
            err = -EINVAL;
            goto error;
        }

        /* Process device type info for this <os, hv> combination */
        err = process_dev_section(reader, &hv_link->dev_sections_list, &hv_link->num_dev_sections);
        if (err != 0)
            goto error;
    }

finished:
    os->num_hypervisors += 1;
    list_add_tail(&hv_link->list, &os->hypervisor_info_list);
    return 0;

error:
    __osi_cleanup_hv_link(hv_link);
    return err;
}

static int process_os_relationship(xmlTextReaderPtr reader,
                                   struct osi_internal_os * os,
                                   osi_relationship relationship)
{
    int empty;
    char* id;
    struct osi_os_link * os_link;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);
    if (!empty || !id) {
        free(id);
        return -EINVAL;
    }

    os_link = malloc(sizeof(*os_link));
    if (!os_link) {
        free(id);
        return -ENOMEM;
    }

    os_link->subject_os = os;
    os_link->verb = relationship;
    os_link->direct_object_os = NULL; /* will resolve after reading all data */
    os_link->dobj_os_id = id;

    os->num_relationships += 1;
    list_add_tail(&os_link->list, &os->relationships_list);
    return 0;
}

static int process_os(struct osi_internal_lib * lib,
                          xmlTextReaderPtr reader)
{
    /* Cursor is at start of (possibly empty) os node */
    /*
     * First, determine if hv has ID or not, and if tag is empty or not.
     * The following cases can occur:
     * 1. No ID: Return invalid. Parse fails overall.
     * 2. Empty, ID: Make hv with given ID and no data and return.
     * 3. Non-Empty, ID: Make hv, parse data till closing tag, return.
     */

    int empty, node_type, err, ret;
    char* id, * key = NULL, * val = NULL;
    const xmlChar* name;
    struct osi_internal_os * os;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    os = malloc(sizeof(*os));
    if (!os) {
        free(id);
        return -ENOMEM;
    }

    os->id = id;
    os->num_params = 0;
    INIT_LIST_HEAD(&os->param_list);
    os->num_hypervisors = 0;
    INIT_LIST_HEAD(&os->hypervisor_info_list);
    os->num_dev_sections = 0;
    INIT_LIST_HEAD(&os->dev_sections_list);
    os->num_relationships = 0;
    INIT_LIST_HEAD(&os->relationships_list);
    os->lib = lib;

    if (empty)
        goto finished;

    /* Current node is start of non empty os
     * While true:
     *   Advance to next node
     *   If node == end of os break
     *   If node is not element, continue
     *   If node is element and not section or hypervisor:
     *     Note element name
     *     Advance to next node, ensure type is text else error
     *     Store value, advance to next node
     *     Ensure node is end of current name
     *     Store <key, val> in params list for object
     *   If node is start of section handle section and track device driver
     *   If node is hypervisor handle hypervisor link
     */

    for (;;) {
        /* Advance to next node */
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            err = -EINVAL;
            goto cleanup_error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);
        if (node_type == -1 || !name) {
            err = -EINVAL;
            goto cleanup_error;
        }
        /* If end of os, break */
        if (node_type == END_NODE && strcmp(name, "os") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        if (strcmp(name, "section") == 0) {
            /* Node is start of device section for os */
            err = process_dev_section(reader, &os->dev_sections_list, &os->num_dev_sections);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "hypervisor") == 0) {
            err = process_os_hv_link(reader, os);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "upgrades") == 0) {
            err = process_os_relationship(reader, os, UPGRADES);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "clones") == 0) {
            err = process_os_relationship(reader, os, CLONES);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "derives-from") == 0) {
            err = process_os_relationship(reader, os, DERIVES_FROM);
            if (err != 0)
                goto cleanup_error;
        }
        else {
            /* Node is start of element of known name */
            err = process_tag(reader, &key, &val);
            if (err != 0 || !key || !val)
                goto cleanup_error;

            /* Store <key,val> in device param_list */
            err = __osi_store_keyval(key, val, &os->param_list, &os->num_params);
            if (err != 0)
                goto cleanup_error;

            free(key);
            free(val);
        }
    }

finished:
    lib->num_os += 1;
    list_add_tail(&os->list, &lib->os_list);
    return 0;
    /* At end, cursor is at end of os node */

cleanup_error:
    __osi_free_os(os);
    return err;
}

static int process_hypervisor(struct osi_internal_lib * lib,
                                  xmlTextReaderPtr reader)
{
    /* Cursor is at start of (possibly empty) hypervisor node */

    /*
     * First, determine if hv has ID or not, and if tag is empty or not.
     * The following cases can occur:
     * 1. No ID: Return invalid. Parse fails overall.
     * 2. Empty, ID: Make hv with given ID and no data and return.
     * 3. Non-Empty, ID: Make hv, parse data till closing tag, return.
     */

    int empty, node_type, err, ret;
    char* id, * key = NULL, * val = NULL;
    const xmlChar* name;
    struct osi_internal_hv * hv;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    hv = malloc(sizeof(*hv));
    if (!hv) {
        free(id);
        return -ENOMEM;
    }

    hv->id = id;
    hv->num_params = 0;
    INIT_LIST_HEAD(&hv->param_list);
    hv->num_dev_sections = 0;
    INIT_LIST_HEAD(&hv->dev_sections_list);
    hv->lib = lib;

    if (empty)
        goto finished;

    /* Current node is start of non empty hv
     * While true:
     *   Advance to next node
     *   If node == end of hv break
     *   If node is not element, continue
     *   If node is element and not section:
     *     Note element name
     *     Advance to next node, ensure type is text else error
     *     Store value, advance to next node
     *     Ensure node is end of current name
     *     Store <key, val> in params list for object
     *   If node is start of section:
     *     Note section type
     *     While true:
     *       Advance to next node
     *       If node is not element continue
     *       If end of section, break
     *       If not empty device node, parse error
     *       If id not defined, parse error
     *       Store id
     *     Store all ids for given section in the HV
     */

    for (;;) {
        /* Advance to next node */
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            err = -EINVAL;
            goto cleanup_error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);
        if (node_type == -1 || !name) {
            err = -EINVAL;
            goto cleanup_error;
        }
        /* If end of hv, break */
        if (node_type == END_NODE && strcmp(name, "hypervisor") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        if (strcmp(name, "section") == 0) {
            /* Node is start of device section for hv */
            err = process_dev_section(reader, &hv->dev_sections_list, &hv->num_dev_sections);
            if (err != 0)
                goto cleanup_error;
        }
        else {
            /* Node is start of element of known name */
            err = process_tag(reader, &key, &val);
            if (err != 0 || !key || !val)
                goto cleanup_error;

            /* Store <key,val> in device param_list */
            err = __osi_store_keyval(key, val, &hv->param_list, &hv->num_params);
            if (err != 0)
                goto cleanup_error;

            free(key);
            free(val);
        }
    }

finished:
    lib->num_hypervisors += 1;
    list_add_tail(&hv->list, &lib->hypervisor_list);
    return 0;
    /* At end, cursor is at end of hv node */

cleanup_error:
    free(key);
    free(val);
    __osi_free_hv(hv);
    return err;
}

static int process_device(struct osi_internal_lib * lib,
                            xmlTextReaderPtr reader)
{
    /* Cursor is at start of (possibly empty) device node */

    /*
     * First, determine if device has ID or not, and if tag is empty or not.
     * The following cases can occur:
     * 1. No ID: Return invalid. Parse fails overall.
     * 2. Empty, ID: Make device with given ID and no data and return.
     * 3. Non-Empty, ID: Make device, parse data till closing tag, return.
     */

    int empty, node_type, err, ret;
    char* id, * key, * val;
    const xmlChar* name;
    struct osi_internal_dev * dev;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    dev = malloc(sizeof(*dev));
    if (!dev) {
        free(id);
        return -ENOMEM;
    }

    dev->id = id;
    dev->num_params = 0;
    INIT_LIST_HEAD(&dev->param_list);
    dev->lib = lib;

    if (empty)
        goto finished;

    /* Current node is start of non empty device
     * While true:
     *   Advance to next node
     *   If node == end of device break
     *   If node is not element, continue
     *   If node is element:
     *     Note element name
     *     Advance to next node, ensure type is text else error
     *     Store value, advance to next node
     *     Ensure node is end of current name
     *     Store <key, val> in params list for object
     */

    for (;;) {
        /* Advance to next node */
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            err = -EINVAL;
            goto cleanup_error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);

        if (node_type == -1 || !name) {
            err = -EINVAL;
            goto cleanup_error;
        }

        /* If end of device, break */
        if (node_type == END_NODE && strcmp(name, "device") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        /* Node is start of element of known name */
        err = process_tag(reader, &key, &val);
        if (err != 0 || !key || !val)
            goto cleanup_error;

        /* Store <key,val> in device param_list */
        err = __osi_store_keyval(key, val, &dev->param_list, &dev->num_params);
        if (err != 0)
            goto cleanup_error;

        free(key);
        free(val);
    }

finished:
    list_add_tail(&dev->list, &lib->device_list);
    lib->num_devices += 1;
    return 0;
    /* At end, cursor is at end of device node */

cleanup_error:
    free(key);
    free(val);
    __osi_free_device(dev);
    return err;
}

static int process_file(struct osi_internal_lib * lib,
                            xmlTextReaderPtr reader)
{
    /*
     * File assumed to contain data in XML format. All data
     * is within <libosinfo>...</libosinfo>. The following steps are taken
     * to process the data within the file:
     *
     * Advance till we are at opening <libosinfo> tag.
     * While true:
     *   Advance tag
     *   If closing libosinfo tag, break
     *   If non element tag, continue
     *   If element tag, and element is not os, hypervisor or device, error
     *   Else, switch on tag type and handle reading in data
     * After loop, return success if no error
     * If there was an error, clean up lib data acquired so far
     */

    int err, node_type, ret;
    const char* name;

    /* Advance to starting libosinfo tag */
    for (;;) {
        err = xmlTextReaderRead(reader);
        if (err != 1) {
            err = -EINVAL;
            goto cleanup_error;
        }

        node_type = xmlTextReaderNodeType(reader);
        if (node_type != ELEMENT_NODE)
            continue;

        name = xmlTextReaderConstName(reader);
        if (strcmp(name, "libosinfo") == 0)
            break;
    }

    /* Now read and handle each tag of interest */
    for (;;) {
        /* Advance to next node */
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            err = -EINVAL;
            goto cleanup_error;
        }

        node_type = xmlTextReaderNodeType(reader);
        name = xmlTextReaderConstName(reader);

        if (node_type == -1 || !name) {
            err = -EINVAL;
            goto cleanup_error;
        }

        /* If end of libosinfo, break */
        if (node_type == END_NODE && strcmp(name, "libosinfo") == 0)
            break;

        /* If node is not start of an element, continue */
        if (node_type != ELEMENT_NODE)
            continue;

        /* Process element node */
        if (strcmp(name, "device") == 0) {
            err = process_device(lib, reader);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "hypervisor") == 0) {
            err = process_hypervisor(lib, reader);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "os") == 0) {
            err = process_os(lib, reader);
            if (err != 0)
                goto cleanup_error;
        }
        else {
            err = -EINVAL;
            goto cleanup_error;
        }
    }

    /* And we are done, successfully */
    return 0;

cleanup_error:
    __osi_free_lib(lib);
    return err;
}

static int read_data_file(struct osi_internal_lib *lib,
			  const char *dir,
			  const char *filename)
{
    int ret;
    xmlTextReaderPtr reader;
    char *rel_name = malloc (strlen(dir) + 1 + strlen(filename) + 1);
    if (!rel_name)
      return -errno;

    stpcpy(stpcpy(stpcpy(rel_name, dir), "/"), filename);

    reader = xmlReaderForFile(rel_name, NULL, 0);
    free(rel_name);
    if (!reader) {
        return -EINVAL;
    }
    ret = process_file(lib, reader);
    xmlFreeTextReader(reader);
    return ret;
}

int osi_initialize_data(struct osi_internal_lib * internal_lib, char* data_dir)
{
    int ret;
    DIR* dir;
    struct dirent *dp;

    /* Initialize library and check version */
    LIBXML_TEST_VERSION

    /* Get directory with backing data. Defaults to CWD */
    if (!data_dir)
      data_dir = ".";

    if (!data_dir) {
        ret = -errno;
        goto cleanup;
    }

    /* Get XML files in directory */
    dir = opendir(data_dir);
    if (!dir) {
        ret = errno;
        goto cleanup;
    }

    while ((dp=readdir(dir)) != NULL) {
        if (dp->d_type != DT_REG)
            continue;
        ret = read_data_file(internal_lib, data_dir, dp->d_name);
        if (ret != 0)
            break;
    }
    closedir(dir);
    if (ret == 0)
        ret = fix_obj_links(internal_lib);

cleanup:
    xmlCleanupParser();
    return ret;
}

#else
int osi_initialize_data(struct osi_internal_lib * internal_lib)
{
    return -ENOSYS;
}
#endif
