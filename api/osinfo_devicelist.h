/*
 * libosinfo
 *
 * osinfo_devicelist.h
 */

#ifndef __OSINFO_DEVICELIST_H__
#define __OSINFO_DEVICELIST_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_DEVICELIST                  (osinfo_devicelist_get_type ())
#define OSINFO_DEVICELIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_DEVICELIST, OsinfoDeviceList))
#define OSINFO_IS_DEVICELIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_DEVICELIST))
#define OSINFO_DEVICELIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_DEVICELIST, OsinfoDeviceListClass))
#define OSINFO_IS_DEVICELIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_DEVICELIST))
#define OSINFO_DEVICELIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_DEVICELIST, OsinfoDeviceListClass))

typedef struct _OsinfoDeviceList        OsinfoDeviceList;

typedef struct _OsinfoDeviceListClass   OsinfoDeviceListClass;

typedef struct _OsinfoDeviceListPrivate OsinfoDeviceListPrivate;

/* object */
struct _OsinfoDeviceList
{
    GObject parent_instance;

    /* public */

    /* private */
    OsinfoDeviceListPrivate *priv;
};

/* class */
struct _OsinfoDeviceListClass
{
    GObjectClass parent_class;

    /* class members */
};

OsinfoDeviceList *osinfoDeviceListFilter(OsinfoDeviceList *self, OsinfoFilter *filter, GError **err);
OsinfoDevice *osinfoGetDeviceAtIndex(OsinfoDeviceList *self, gint idx, GError **err);
OsinfoDeviceList *osinfoDeviceListIntersect(OsinfoDeviceList *self, OsinfoDeviceList *otherDeviceList, GError **err);
OsinfoDeviceList *osinfoDeviceListUnion(OsinfoDeviceList *self, OsinfoDeviceList *otherDeviceList, GError **err);

#endif /* __OSINFO_DEVICELIST_H__ */
