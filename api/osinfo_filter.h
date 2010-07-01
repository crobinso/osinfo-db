/*
 * libosinfo
 *
 * osinfo_filter.h
 * Represents a filter in libosinfo.
 */

#ifndef __OSINFO_FILTER_H__
#define __OSINFO_FILTER_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_FILTER                  (osinfo_filter_get_type ())
#define OSINFO_FILTER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_FILTER, OsinfoFilter))
#define OSINFO_IS_FILTER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_FILTER))
#define OSINFO_FILTER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_FILTER, OsinfoFilterClass))
#define OSINFO_IS_FILTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_FILTER))
#define OSINFO_FILTER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_FILTER, OsinfoFilterClass))

//typedef struct _OsinfoFilter        OsinfoFilter;
// (defined in osinfo_objects.h)

#include "osinfo_oslist.h"

typedef struct _OsinfoFilterClass  OsinfoFilterClass;

typedef struct _OsinfoFilterPrivate OsinfoFilterPrivate;

/* object */
struct _OsinfoFilter
{
    OsinfoEntity parent_instance;

    /* public */

    /* private */
    OsinfoFilterPrivate *priv;
};

/* class */
struct _OsinfoFilterClass
{
    OsinfoEntityClass parent_class;

    /* class members */
};

void osinfoFreeFilter(OsinfoFilter *self);

gint osinfoAddFilterContstraint(OsinfoFilter *self, gchar *propName, gchar *propVal, GError **err);

// Only applicable to OSes, ignored by other types of objects
gint osinfoAddRelationConstraint(OsinfoFilter *self, osinfoRelationship relshp, OsinfoOs *os, GError **err);

void osinfoClearFilterConstraint(OsinfoFilter *self, gchar *propName);
void osinfoClearRelationshipConstraint(OsinfoFilter *self, osinfoRelationship relshp);
void osinfoClearAllFilterConstraints(OsinfoFilter *self);

GPtrArray *osinfoGetFilterConstraintKeys(OsinfoFilter *self, GError **err);
gchar *osinfoGetFilterConstraintValue(OsinfoFilter *self, GError **err);
OsinfoOsList *osinfoGetRelationshipConstraintValue(OsinfoFilter *self, osinfoRelationship relshp, GError **err);

#endif /* __OSINFO_FILTER_H__ */
