#include "custom-list.h"


/* boring declarations of local functions */

static void         custom_list_init            (CustomList      *pkg_tree);

static void         custom_list_class_init      (CustomListClass *klass);

static void         custom_list_tree_model_init (GtkTreeModelIface *iface);

static void         custom_list_finalize        (GObject           *object);

static GtkTreeModelFlags custom_list_get_flags  (GtkTreeModel      *tree_model);

static gint         custom_list_get_n_columns   (GtkTreeModel      *tree_model);

static GType        custom_list_get_column_type (GtkTreeModel      *tree_model,
                                                 gint               index);

static gboolean     custom_list_get_iter        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreePath       *path);

static GtkTreePath *custom_list_get_path        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static void         custom_list_get_value       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 gint               column,
                                                 GValue            *value);

static gboolean     custom_list_iter_next       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gboolean     custom_list_iter_children   (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent);

static gboolean     custom_list_iter_has_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gint         custom_list_iter_n_children (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);

static gboolean     custom_list_iter_nth_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent,
                                                 gint               n);

static gboolean     custom_list_iter_parent     (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *child);



static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */


/*****************************************************************************
 *
 *  custom_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

GType
custom_list_get_type (void)
{
  static GType custom_list_type = 0;

  /* Some boilerplate type registration stuff */
  if (custom_list_type == 0)
  {
    static const GTypeInfo custom_list_info =
    {
      sizeof (CustomListClass),
      NULL,                                         /* base_init */
      NULL,                                         /* base_finalize */
      (GClassInitFunc) custom_list_class_init,
      NULL,                                         /* class finalize */
      NULL,                                         /* class_data */
      sizeof (CustomList),
      0,                                           /* n_preallocs */
      (GInstanceInitFunc) custom_list_init
    };
    static const GInterfaceInfo tree_model_info =
    {
      (GInterfaceInitFunc) custom_list_tree_model_init,
      NULL,
      NULL
    };

    /* First register the new derived type with the GObject type system */
    custom_list_type = g_type_register_static (G_TYPE_OBJECT, "CustomList",
                                               &custom_list_info, (GTypeFlags)0);

    /* Now register our GtkTreeModel interface with the type system */
    g_type_add_interface_static (custom_list_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
  }

  return custom_list_type;
}


/*****************************************************************************
 *
 *  custom_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void
custom_list_class_init (CustomListClass *klass)
{
  GObjectClass *object_class;

  parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
  object_class = (GObjectClass*) klass;

  object_class->finalize = custom_list_finalize;
}

/*****************************************************************************
 *
 *  custom_list_tree_model_init: init callback for the interface registration
 *                               in custom_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void
custom_list_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = custom_list_get_flags;
  iface->get_n_columns   = custom_list_get_n_columns;
  iface->get_column_type = custom_list_get_column_type;
  iface->get_iter        = custom_list_get_iter;
  iface->get_path        = custom_list_get_path;
  iface->get_value       = custom_list_get_value;
  iface->iter_next       = custom_list_iter_next;
  iface->iter_children   = custom_list_iter_children;
  iface->iter_has_child  = custom_list_iter_has_child;
  iface->iter_n_children = custom_list_iter_n_children;
  iface->iter_nth_child  = custom_list_iter_nth_child;
  iface->iter_parent     = custom_list_iter_parent;
}


/*****************************************************************************
 *
 *  custom_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in custom_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void
custom_list_init (CustomList *custom_list)
{
  custom_list->n_columns       = CUSTOM_LIST_N_COLUMNS;

  custom_list->column_types[0] = G_TYPE_POINTER;  /* CUSTOM_LIST_COL_RECORD    */
  custom_list->column_types[1] = G_TYPE_STRING;   /* CUSTOM_LIST_COL_NAME      */
//  custom_list->column_types[2] = G_TYPE_UINT;     /* CUSTOM_LIST_COL_YEAR_BORN */

  g_assert (CUSTOM_LIST_N_COLUMNS == 2);

  custom_list->num_rows = 0;
  custom_list->rows     = NULL;

//  custom_list->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

}


/*****************************************************************************
 *
 *  custom_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void
custom_list_finalize (GObject *object)
{
/*  CustomList *custom_list = CUSTOM_LIST(object); */

  /* free all records and free all memory used by the list */
  #warning IMPLEMENT

  /* must chain up - finalize parent */
  (* parent_class->finalize) (object);
}


/*****************************************************************************
 *
 *  custom_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags
custom_list_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (CUSTOM_IS_LIST(tree_model), (GtkTreeModelFlags)0);

  return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}


/*****************************************************************************
 *
 *  custom_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint
custom_list_get_n_columns (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (CUSTOM_IS_LIST(tree_model), 0);

  return CUSTOM_LIST(tree_model)->n_columns;
}


/*****************************************************************************
 *
 *  custom_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType
custom_list_get_column_type (GtkTreeModel *tree_model,
                             gint          index)
{
  g_return_val_if_fail (CUSTOM_IS_LIST(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index < CUSTOM_LIST(tree_model)->n_columns && index >= 0, G_TYPE_INVALID);

  return CUSTOM_LIST(tree_model)->column_types[index];
}


/*****************************************************************************
 *
 *  custom_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our CustomRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean
custom_list_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter,
                      GtkTreePath  *path)
{
  CustomList    *custom_list;
  CustomRecord  *record;
  gint          *indices, n, depth;

  g_assert(CUSTOM_IS_LIST(tree_model));
  g_assert(path!=NULL);

  custom_list = CUSTOM_LIST(tree_model);

  indices = gtk_tree_path_get_indices(path);
  depth   = gtk_tree_path_get_depth(path);

  /* we do not allow children */
  g_assert(depth == 1); /* depth 1 = top level; a list only has top level nodes and no children */

  n = indices[0]; /* the n-th top level row */

  if ( n >= custom_list->num_rows || n < 0 )
    return FALSE;

  record = custom_list->rows[n];

  g_assert(record != NULL);
  g_assert(record->pos == n);

  /* We simply store a pointer to our custom record in the iter */
//  iter->stamp      = custom_list->stamp;
  iter->user_data  = record;
  iter->user_data2 = NULL;   /* unused */
  iter->user_data3 = NULL;   /* unused */

  return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *
custom_list_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter)
{
  GtkTreePath  *path;
  CustomRecord *record;
  CustomList   *custom_list;

  g_return_val_if_fail (CUSTOM_IS_LIST(tree_model), NULL);
  g_return_val_if_fail (iter != NULL,               NULL);
  g_return_val_if_fail (iter->user_data != NULL,    NULL);

  custom_list = CUSTOM_LIST(tree_model);

  record = (CustomRecord*) iter->user_data;

  path = gtk_tree_path_new();
  gtk_tree_path_append_index(path, record->pos);

  return path;
}


/*****************************************************************************
 *
 *  custom_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
custom_list_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter  *iter,
                       gint          column,
                       GValue       *value)
{
  CustomRecord  *record;
  CustomList    *custom_list;

  g_return_if_fail (CUSTOM_IS_LIST (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (column < CUSTOM_LIST(tree_model)->n_columns);

  g_value_init (value, CUSTOM_LIST(tree_model)->column_types[column]);

  custom_list = CUSTOM_LIST(tree_model);

  record = (CustomRecord*) iter->user_data;

  g_return_if_fail ( record != NULL );

  if(record->pos >= custom_list->num_rows)
   g_return_if_reached();

  switch(column)
  {
    case CUSTOM_LIST_COL_RECORD:
      g_value_set_pointer(value, record);
      break;

    case CUSTOM_LIST_COL_NAME:
      g_value_set_string(value, record->taskname);
      break;

//    case CUSTOM_LIST_COL_YEAR_BORN:
//      g_value_set_uint(value, record->year_born);
//      break;
  }
}


/*****************************************************************************
 *
 *  custom_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_next (GtkTreeModel  *tree_model,
                       GtkTreeIter   *iter)
{
  CustomRecord  *record, *nextrecord;
  CustomList    *custom_list;

  g_return_val_if_fail (CUSTOM_IS_LIST (tree_model), FALSE);

  if (iter == NULL || iter->user_data == NULL)
    return FALSE;

  custom_list = CUSTOM_LIST(tree_model);

  record = (CustomRecord *) iter->user_data;

  /* Is this the last record in the list? */
  if ((record->pos + 1) >= custom_list->num_rows)
    return FALSE;

  nextrecord = custom_list->rows[(record->pos + 1)];

  g_assert ( nextrecord != NULL );
  g_assert ( nextrecord->pos == (record->pos + 1) );

//  iter->stamp     = custom_list->stamp;
  iter->user_data = nextrecord;

  return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreeIter  *parent)
{
  CustomList  *custom_list;

  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  /* parent == NULL is a special case; we need to return the first top-level row */

  g_return_val_if_fail (CUSTOM_IS_LIST (tree_model), FALSE);

  custom_list = CUSTOM_LIST(tree_model);

  /* No rows => no first row */
  if (custom_list->num_rows == 0)
    return FALSE;

  /* Set iter to first item in list */
//  iter->stamp     = custom_list->stamp;
  iter->user_data = custom_list->rows[0];

  return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
  return FALSE;
}


/*****************************************************************************
 *
 *  custom_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint
custom_list_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter)
{
  CustomList  *custom_list;

  g_return_val_if_fail (CUSTOM_IS_LIST (tree_model), -1);
  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);

  custom_list = CUSTOM_LIST(tree_model);

  /* special case: if iter == NULL, return number of top-level rows */
  if (!iter)
    return custom_list->num_rows;

  return 0; /* otherwise, this is easy again for a list */
}


/*****************************************************************************
 *
 *  custom_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_nth_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            GtkTreeIter  *parent,
                            gint          n)
{
  CustomRecord  *record;
  CustomList    *custom_list;

  g_return_val_if_fail (CUSTOM_IS_LIST (tree_model), FALSE);

  custom_list = CUSTOM_LIST(tree_model);

  /* a list has only top-level rows */
  if(parent)
    return FALSE;

  /* special case: if parent == NULL, set iter to n-th top-level row */

  if( n >= custom_list->num_rows )
    return FALSE;

  record = custom_list->rows[n];

  g_assert( record != NULL );
  g_assert( record->pos == n );

//  iter->stamp = custom_list->stamp;
  iter->user_data = record;

  return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         GtkTreeIter  *child)
{
  return FALSE;
}


/*****************************************************************************
 *
 *  custom_list_new:  This is what you use in your own code to create a
 *                    new custom list tree model for you to use.
 *
 *****************************************************************************/

CustomList *
custom_list_new (void)
{
  CustomList *newcustomlist;

  newcustomlist = (CustomList*) g_object_new (CUSTOM_TYPE_LIST, NULL);

  g_assert( newcustomlist != NULL );

  return newcustomlist;
}


/*****************************************************************************
 *
 *  custom_list_append_record:  Empty lists are boring. This function can
 *                              be used in your own code to add rows to the
 *                              list. Note how we emit the "row-inserted"
 *                              signal after we have appended the row
 *                              internally, so the tree view and other
 *                              interested objects know about the new row.
 *
 *****************************************************************************/

GtkTreePath *
custom_list_append_record (CustomList   *custom_list,
                           const gchar  *name)
{
  GtkTreeIter   iter;
  GtkTreePath  *path;
  CustomRecord *newrecord;
  gulong        newsize;
  guint         pos;

  g_return_if_fail (CUSTOM_IS_LIST(custom_list));
  g_return_if_fail (name != NULL);

  pos = custom_list->num_rows;

  custom_list->num_rows++;

  newsize = custom_list->num_rows * sizeof(CustomRecord*);

  custom_list->rows = g_realloc(custom_list->rows, newsize);

  newrecord = g_new0(CustomRecord, 1);

  newrecord->taskname = g_strdup(name);
  newrecord->name_collate_key = g_utf8_collate_key(name,-1); /* for fast sorting, used later */
//  newrecord->year_born = year_born;

  custom_list->rows[pos] = newrecord;
  newrecord->pos = pos;

  /* inform the tree view and other interested objects
   *  (e.g. tree row references) that we have inserted
   *  a new row, and where it was inserted */

  path = gtk_tree_path_new();
  gtk_tree_path_append_index(path, newrecord->pos);

  custom_list_get_iter(GTK_TREE_MODEL(custom_list), &iter, path);

  gtk_tree_model_row_inserted(GTK_TREE_MODEL(custom_list), path, &iter);
// PV:
  return path;
//  gtk_tree_path_free(path);
}
