/*
 * libosinfo
 *
 * osinfo_list.h
 */

#ifndef __OSINFO_LIST_H__
#define __OSINFO_LIST_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_LIST                  (osinfo_list_get_type ())
#define OSINFO_LIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_LIST, OsinfoList))
#define OSINFO_IS_LIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_LIST))
#define OSINFO_LIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_LIST, OsinfoListClass))
#define OSINFO_IS_LIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_LIST))
#define OSINFO_LIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_LIST, OsinfoListClass))

typedef struct _OsinfoListClass   OsinfoListClass;

typedef struct _OsinfoListPrivate OsinfoListPrivate;

/* object */
struct _OsinfoList
{
    GObject parent_instance;

    /* public */

    /* private */
    OsinfoListPrivate *priv;
};

/* class */
struct _OsinfoListClass
{
    GObjectClass parent_class;

    /* class members */
};

void osinfoFreeList(OsinfoList *self);
gint osinfoListLength(OsinfoList *self);

OsinfoList *osinfoListFilter(OsinfoList *self, OsinfoFilter *filter, GError **err);
OsinfoEntity *osinfoGetEntityAtIndex(OsinfoList *self, gint idx);
OsinfoList *osinfoListIntersect(OsinfoList *self, OsinfoList *otherList, GError **err);
OsinfoList *osinfoListUnion(OsinfoList *self, OsinfoList *otherList, GError **err);

#endif /* __OSINFO_LIST_H__ */
