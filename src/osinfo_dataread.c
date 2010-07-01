#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <libxml/xmlreader.h>

#include <osinfo.h>

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

struct __osinfoDbRet {
    OsinfoDb *db;
    int *ret;
};

static gboolean __osinfoResolveDeviceLink(gpointer key, gpointer value, gpointer data)
{
    gchar *id = (gchar *) key;
    struct __osinfoDeviceLink *devLink = (struct __osinfoDeviceLink *) value;
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;

    OsinfoDevice *dev = g_tree_lookup(db->priv->devices, id);
    if (!dev) {
        *ret = -EINVAL;
        return TRUE;
    }

    devLink->dev = dev;
    *ret = 0;
    return FALSE;
}

static gboolean __osinfoResolveSectionDevices(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;
    GTree *section = (GTree *) value;
    if (!section) {
        *ret = -EINVAL;
        return TRUE;
    }

    g_tree_foreach(section, __osinfoResolveDeviceLink, dbRet);
    if (*ret)
        return TRUE;
    return FALSE;
}

static gboolean __osinfoResolveHvLink(gpointer key, gpointer value, gpointer data)
{
    gchar *hvId = (gchar *) key;
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;
    struct __osinfoHvSection *hvSection = (struct __osinfoHvSection *) value;
    OsinfoHypervisor *hv;

    g_tree_foreach(hvSection->sections, __osinfoResolveSectionDevices, dbRet);
    if (*ret)
        return TRUE;

    hv = g_tree_lookup(db->priv->hypervisors, hvId);
    if (!hv) {
        *ret = -EINVAL;
        return TRUE;
    }

    hvSection->hv = hv;
    *ret = 0;
    return FALSE;
}

static gboolean __osinfoResolveOsLink(gpointer key, gpointer value, gpointer data)
{
    gchar *targetOsId = (gchar *) key;
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;
    struct __osinfoOsLink *osLink = (struct __osinfoOsLink *) value;

    OsinfoOs *targetOs;
    targetOs = g_tree_lookup(db->priv->oses, targetOsId);
    if (!targetOs) {
        *ret = -EINVAL;
        return TRUE;
    }

    osLink->directObjectOs = targetOs;
    *ret = 0;
    return FALSE;
}

static gboolean __osinfoFixOsLinks(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;
    OsinfoOs *os = OSINFO_OS(value);
    if (!os) {
        *ret = -EINVAL;
        return TRUE;
    }

    g_tree_foreach(os->priv->sections, __osinfoResolveSectionDevices, dbRet);
    if (*ret)
        return TRUE;

    g_tree_foreach(os->priv->relationshipsByOs, __osinfoResolveOsLink, dbRet);
    if (*ret)
        return TRUE;

    g_tree_foreach(os->priv->hypervisors, __osinfoResolveHvLink, dbRet);
    if (*ret)
        return TRUE;

    *ret = 0;
    return FALSE;
}

static gboolean __osinfoFixHvLinks(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoDbRet *dbRet = (struct __osinfoDbRet *) data;
    OsinfoDb *db = dbRet->db;
    int *ret = dbRet->ret;
    OsinfoHypervisor *hv = OSINFO_HYPERVISOR(value);
    if (!hv) {
        *ret = -EINVAL;
        return TRUE;
    }

    g_tree_foreach(hv->priv->sections, __osinfoResolveSectionDevices, dbRet);
    if (*ret)
        return TRUE;
    return FALSE;
}

static int __osinfoFixObjLinks(OsinfoDb *db)
{
    OsinfoHypervisor *hv;
    OsinfoOs *os;
    int ret;

    if (!OSINFO_IS_DB(db))
        return -EINVAL;

    struct __osinfoDbRet dbRet = {db, &ret};

    g_tree_foreach(db->priv->hypervisors, __osinfoFixHvLinks, &dbRet);
    if (ret)
        return ret;
    g_tree_foreach(db->priv->oses, __osinfoFixOsLinks, &dbRet);

    return ret;
}

static int __osinfoProcessTag(xmlTextReaderPtr reader, char** ptr_to_key, char** ptr_to_val)
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

static int __osinfoProcessDevSection(xmlTextReaderPtr reader,
                                     GTree *section, GTree *sectionAsList)
{
    int err, empty, node_type;
    char * sectionType, * id, * key = NULL, * driver = NULL;
    const char * name;

    if (!section)
        return -EINVAL;

    sectionType = xmlTextReaderGetAttribute(reader, "type");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!sectionType)
        return -EINVAL;

    /* If no devices in section then we are done */
    if (empty)
        return 0;

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
            err = __osinfoProcessTag(reader, &key, &driver);
            if (err != 0 || !key || !driver)
                goto error;
            free(key);
            key = NULL; /* In case the next malloc fails, avoid a double free */
        }

        // Alright, we have the id and driver
        err = __osinfoAddDeviceToSection(section, sectionAsList, sectionType, id, driver);
        free (driver);
        driver = NULL;
        free (id);
        id = NULL;
        if (err != 0)
            goto error;
    }
    free(sectionType);

finished:
    return 0;

error:
    free(sectionType);
    free(key);
    free(driver);
    return err;
}

static int __osinfoProcessOsHvLink(xmlTextReaderPtr reader,
                                   OsinfoOs *os)
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
    struct __osinfoHvSection *hvSection;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    hvSection = __osinfoAddHypervisorSectionToOs(os, id);
    free(id);
    if (!hvSection)
        return -EINVAL;

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
        err = __osinfoProcessDevSection(reader, hvSection->sections, hvSection->sectionsAsList);
        if (err != 0)
            goto error;
    }

finished:
    return 0;

error:
    return err;
}

static int __osinfoProcessOsRelationship(xmlTextReaderPtr reader,
                                         OsinfoOs *os,
                                         osinfoRelationship relationship)
{
    int empty, ret;
    char* id;
    struct osi_os_link * os_link;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);
    if (!empty || !id) {
        free(id);
        return -EINVAL;
    }

    ret = __osinfoAddOsRelationship (os, id, relationship);
    free(id);
    return ret;
}

static int __osinfoProcessOs(OsinfoDb *db,
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
    OsinfoOs *os;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    os = g_object_new(OSINFO_TYPE_OS, "id", id, "db", db, NULL);
    free(id);
    if (!os)
        return -ENOMEM;

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
            err = __osinfoProcessDevSection(reader, (OSINFO_OS(os))->priv->sections, (OSINFO_OS(os))->priv->sectionsAsList);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "hypervisor") == 0) {
            err = __osinfoProcessOsHvLink(reader, os);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "upgrades") == 0) {
            err = __osinfoProcessOsRelationship(reader, os, UPGRADES);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "clones") == 0) {
            err = __osinfoProcessOsRelationship(reader, os, CLONES);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "derives-from") == 0) {
            err = __osinfoProcessOsRelationship(reader, os, DERIVES_FROM);
            if (err != 0)
                goto cleanup_error;
        }
        else {
            /* Node is start of element of known name */
            err = __osinfoProcessTag(reader, &key, &val);
            if (err != 0 || !key || !val)
                goto cleanup_error;

            err = __osinfoAddParam(OSINFO_ENTITY(os), key, val);
            if (err != 0)
                goto cleanup_error;

            free(key);
            free(val);
        }
    }

finished:
    __osinfoAddOsToDb(db, os);
    return 0;
    /* At end, cursor is at end of os node */

cleanup_error:
    g_object_unref(os);
    return err;
}

static int __osinfoProcessHypervisor(OsinfoDb *db,
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
    char* id;
    const xmlChar* name;
    OsinfoHypervisor *hv;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;


    hv = g_object_new(OSINFO_TYPE_HYPERVISOR, "id", id, "db", db, NULL);
    free(id);
    if (!hv) {
        return -ENOMEM;
    }

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
            err = __osinfoProcessDevSection(reader, (OSINFO_HYPERVISOR(hv))->priv->sections, (OSINFO_HYPERVISOR(hv))->priv->sectionsAsList);
            if (err != 0)
                goto cleanup_error;
        }
        else {
            /* Node is start of element of known name */
            char *key = NULL, *val = NULL;
            err = __osinfoProcessTag(reader, &key, &val);
            if (err != 0)
                goto cleanup_error;


            err = __osinfoAddParam(OSINFO_ENTITY(hv), key, val);
            free(key);
            free(val);
            if (err != 0)
                goto cleanup_error;
        }
    }

finished:
    __osinfoAddHypervisorToDb(db, hv);
    return 0;
    /* At end, cursor is at end of hv node */

cleanup_error:
    g_object_unref(hv);
    return err;
}

static int __osinfoProcessDevice(OsinfoDb *db,
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
    OsinfoDevice *dev;

    id = xmlTextReaderGetAttribute(reader, "id");
    empty = xmlTextReaderIsEmptyElement(reader);

    if (!id)
        return -EINVAL;

    dev = g_object_new(OSINFO_TYPE_DEVICE, "id", id, "db", db, NULL);
    free(id);
    if (!dev) {
        // TODO: How do errors in gobject creation manifest themselves?
        return -ENOMEM;
    }

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
        err = __osinfoProcessTag(reader, &key, &val);
        if (err != 0 || !key || !val)
            goto cleanup_error;

        err = __osinfoAddParam(OSINFO_ENTITY(dev), key, val);
        if (err != 0)
            goto cleanup_error;

        free(key);
        free(val);
    }

finished:
    // Add dev to db
    __osinfoAddDeviceToDb(db, dev);
    return 0;
    /* At end, cursor is at end of device node */

cleanup_error:
    free(key);
    free(val);
    g_object_unref(dev);
    return err;
}

static int __osinfoProcessFile(OsinfoDb *db,
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
            err = __osinfoProcessDevice(db, reader);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "hypervisor") == 0) {
            err = __osinfoProcessHypervisor(db, reader);
            if (err != 0)
                goto cleanup_error;
        }
        else if (strcmp(name, "os") == 0) {
            err = __osinfoProcessOs(db, reader);
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
    // Db will be unsatisfactorily initiated, caller will call unref to clean it
    return err;
}

static int __osinfoReadDataFile(OsinfoDb *db,
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
    ret = __osinfoProcessFile(db, reader);
    xmlFreeTextReader(reader);
    return ret;
}

int __osinfoInitializeData(OsinfoDb *db)
{
    int ret;
    DIR* dir;
    struct dirent *dp;

    char *backingDir;
    g_object_get(G_OBJECT(db), "backing-dir", &backingDir, NULL);

    /* Initialize library and check version */
    LIBXML_TEST_VERSION

    /* Get directory with backing data. Defaults to CWD */
    if (!backingDir)
      backingDir = ".";

    /* Get XML files in directory */
    dir = opendir(backingDir);
    if (!dir) {
        ret = errno;
        goto cleanup;
    }

    while ((dp=readdir(dir)) != NULL) {
        if (dp->d_type != DT_REG)
            continue;
        ret = __osinfoReadDataFile(db, backingDir, dp->d_name);
        if (ret != 0)
            break;
    }
    closedir(dir);
    if (ret == 0)
        ret = __osinfoFixObjLinks(db);

cleanup:
    xmlCleanupParser();
    g_free(backingDir);
    return ret;
}

#else
int __osinfoInitializeData(OsinfoDb *db)
{
    return -ENOSYS;
}
#endif
