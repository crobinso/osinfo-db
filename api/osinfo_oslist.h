/*
 * libosinfo
 *
 * osinfo_oslist.h
 * All entities represented in libosinfo are derived from this class.
 */

#ifndef __OSINFO_OSLIST_H__
#define __OSINFO_OSLIST_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_OSLIST                  (osinfo_oslist_get_type ())
#define OSINFO_OSLIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_OSLIST, OsinfoOsList))
#define OSINFO_IS_OSLIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_OSLIST))
#define OSINFO_OSLIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_OSLIST, OsinfoOsListClass))
#define OSINFO_IS_OSLIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_OSLIST))
#define OSINFO_OSLIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_OSLIST, OsinfoOsListClass))

typedef struct _OsinfoOsList        OsinfoOsList;

typedef struct _OsinfoOsListClass   OsinfoOsListClass;

typedef struct _OsinfoOsListPrivate OsinfoOsListPrivate;

/* object */
struct _OsinfoOsList
{
    GObject parent_instance;

    /* public */

    /* private */
    OsinfoOsListPrivate *priv;
};

/* class */
struct _OsinfoOsListClass
{
    GObjectClass parent_class;

    /* class members */
};

OsinfoOsList *osinfoOsListFilter(OsinfoOsList *self, OsinfoFilter *filter, GError **err);
OsinfoOs *osinfoGetOsAtIndex(OsinfoOsList *self, gint idx, GError **err);
OsinfoOsList *osinfoOsListIntersect(OsinfoOsList *self, OsinfoOsList *otherOsList, GError **err);
OsinfoOsList *osinfoOsListUnion(OsinfoOsList *self, OsinfoOsList *otherOsList, GError **err);

#endif /* __OSINFO_OSLIST_H__ */
