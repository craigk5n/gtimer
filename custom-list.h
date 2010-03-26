#ifndef _custom_list_h_included_
#define _custom_list_h_included_

#include <gtk/gtk.h>

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define CUSTOM_TYPE_LIST            (custom_list_get_type ())
#define CUSTOM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUSTOM_TYPE_LIST, CustomList))
#define CUSTOM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_LIST, CustomListClass))
#define CUSTOM_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_LIST))
#define CUSTOM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_LIST))
#define CUSTOM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_LIST, CustomListClass))

/* The data columns that we export via the tree model interface */

enum
{
  CUSTOM_LIST_COL_RECORD = 0,
  CUSTOM_LIST_COL_NAME,
//  CUSTOM_LIST_COL_YEAR_BORN,
  CUSTOM_LIST_N_COLUMNS,
} ;


typedef struct _CustomRecord     CustomRecord;
typedef struct _CustomList       CustomList;
typedef struct _CustomListClass  CustomListClass;



/* CustomRecord: this structure represents a row */

struct _CustomRecord
{
  /* data - you can extend this */
  gchar    *taskname;
  gchar    *name_collate_key;
//  guint     year_born;

  /* admin stuff used by the custom list model */
  guint     pos;   /* pos within the array */
};



/* CustomList: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct _CustomList
{
  GObject         parent;      /* this MUST be the first member */

  guint           num_rows;    /* number of rows that we have   */
  CustomRecord  **rows;        /* a dynamically allocated array of pointers to
                                *   the CustomRecord structure for each row    */

  /* These two fields are not absolutely necessary, but they    */
  /*   speed things up a bit in our get_value implementation    */
  gint            n_columns;
  GType           column_types[CUSTOM_LIST_N_COLUMNS];

//  gint            stamp;       /* Random integer to check whether an iter belongs to our model */
};



/* CustomListClass: more boilerplate GObject stuff */

struct _CustomListClass
{
  GObjectClass parent_class;
};


GType             custom_list_get_type (void);

CustomList       *custom_list_new (void);

GtkTreePath       *custom_list_append_record (CustomList   *custom_list,
                                             const gchar  *name);

#endif /* _custom_list_h_included_ */

