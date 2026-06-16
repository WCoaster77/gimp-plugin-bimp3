#include <gtk/gtk.h>
#include <glib.h>
#include "bimp-gui-priv.h"
#include "bimp.h"
#include "bimp-utils.h"

GtkWidget *treeview_files = NULL;

enum { LIST_ITEM = 0, N_COLUMNS };

void
init_fileview(void)
{
    GtkCellRenderer   *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column   = gtk_tree_view_column_new_with_attributes(
        "Files", renderer, "text", LIST_ITEM, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_files), column);

    GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_files), GTK_TREE_MODEL(store));
    g_object_unref(store);
}

static void
add_to_fileview(char *str)
{
    GtkListStore *store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files)));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}

void
bimp_refresh_fileview(void)
{
    GtkListStore *store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files)));
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter))
        gtk_list_store_clear(store);
    for (GSList *node = bimp_input_filenames; node; node = node->next)
        add_to_fileview(node->data);
}

GSList *
get_treeview_selection(void)
{
    GtkTreeModel *model;
    GList        *rows;
    GSList       *out = NULL;

    rows = gtk_tree_selection_get_selected_rows(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_files)), &model);
    for (GList *i = rows; i; i = g_list_next(i)) {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath *)i->data)) {
            char *val;
            gtk_tree_model_get(model, &iter, LIST_ITEM, &val, -1);
            out = g_slist_append(out, val);
        }
    }
    g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(rows);
    return out;
}

void
remove_input_file(GtkWidget *widget, gpointer data)
{
    GSList *sel = get_treeview_selection();
    for (GSList *i = sel; i; i = g_slist_next(i)) {
        GSList *found = g_slist_find_custom(bimp_input_filenames,
                                            i->data, (GCompareFunc)strcmp);
        if (found)
            bimp_input_filenames = g_slist_delete_link(bimp_input_filenames, found);
    }
    g_slist_free(sel);
    bimp_refresh_fileview();
    update_selection(NULL);
}

void
remove_all_input_files(GtkWidget *widget, gpointer data)
{
    g_slist_free_full(bimp_input_filenames, g_free);
    bimp_input_filenames = NULL;
    bimp_refresh_fileview();
    update_selection(NULL);
}

void
select_filename(GtkTreeSelection *sel, gpointer data)
{
    GSList *selection = get_treeview_selection();
    if (selection && g_slist_length(selection) == 1)
        update_selection((char *)selection->data);
    else
        update_selection(NULL);
    g_slist_free(selection);
}
