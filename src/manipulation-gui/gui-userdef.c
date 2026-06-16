#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <gegl.h>
#include <stdlib.h>
#include <string.h>
#include "gui-userdef.h"
#include "gui-userdef-priv.h"
#include "../bimp-manipulations.h"
#include "../bimp-utils.h"
#include "../plugin-intl.h"

GtkWidget        *treeview_procedures;
GtkWidget        *scroll_procparam;
GtkWidget        *parent_dialog;
userdef_settings  temp_settings;
GtkWidget       **param_widget;
GtkTreeSelection *treesel_proc;

enum { LIST_ITEM = 0, N_COLUMNS };

static void      init_procedure_list(void);
static int       fill_procedure_list(char *, char *);
static void      search_procedure(GtkEditable *, gpointer);
static gboolean  select_procedure(GtkTreeSelection *, GtkTreeModel *,
                                   GtkTreePath *, gboolean, gpointer);
static void      init_temp_settings(gchar *, gint, GParamSpec **);
static void      update_selected_procedure(gchar *);
static int       setup_procedure_chooser(GtkWidget *, userdef_settings);
static GtkWidget *build_chooser_grid(GtkWidget *);
static void      restore_from_settings(userdef_settings, int);

static void
init_procedure_list(void)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;
    GtkListStore      *store;

    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes(
        "List Items", renderer, "text", LIST_ITEM, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_procedures), column);
    store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_procedures),
                            GTK_TREE_MODEL(store));
    g_object_unref(store);
}

static int
fill_procedure_list(char *search, char *selection)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter   treeiter;
    GSList       *iter;
    int           finalcount = 0, selected_i = -1;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(
        GTK_TREE_VIEW(treeview_procedures)));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_procedures));
    if (gtk_tree_model_get_iter_first(model, &treeiter) == TRUE)
        gtk_list_store_clear(store);
    init_supported_procedures();
    for (iter = bimp_supported_procedures; iter; iter = iter->next) {
        gchar **procedure_name = iter->data;
        if (search == NULL ||
            str_contains_cins((char *)procedure_name, search)) {
            store = GTK_LIST_STORE(gtk_tree_view_get_model(
                GTK_TREE_VIEW(treeview_procedures)));
            gtk_list_store_append(store, &treeiter);
            gtk_list_store_set(store, &treeiter, LIST_ITEM, procedure_name, -1);
            if (selected_i != finalcount && selection != NULL &&
                strcmp((char *)procedure_name, selection) == 0)
                selected_i = finalcount;
            finalcount++;
        }
    }
    return selected_i;
}

static void
search_procedure(GtkEditable *editable, gpointer data)
{
    fill_procedure_list(
        (char *)gtk_editable_get_text(GTK_EDITABLE(editable)), NULL);
}

static gboolean
select_procedure(GtkTreeSelection *selection, GtkTreeModel *model,
                 GtkTreePath *path, gboolean currently_selected, gpointer data)
{
    GtkTreeIter iter;
    gchar *selected;
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, LIST_ITEM, &selected, -1);
        if (!currently_selected)
            update_selected_procedure(selected);
        return TRUE;
    }
    return FALSE;
}

static void
init_temp_settings(gchar *procedure, gint n_args, GParamSpec **args)
{
    if (temp_settings != NULL) {
        g_free(temp_settings->procedure);
        if (temp_settings->params) gimp_value_array_unref(temp_settings->params);
        g_free(temp_settings);
    }
    temp_settings = (userdef_settings)g_malloc(sizeof(struct manip_userdef_set));
    temp_settings->procedure = g_strdup(procedure);
    temp_settings->params    = gimp_value_array_new(n_args);
    for (int i = 0; i < n_args; i++) {
        GType       type = G_PARAM_SPEC_VALUE_TYPE(args[i]);
        const char *name = g_param_spec_get_name(args[i]);
        GValue      val  = G_VALUE_INIT;
        g_value_init(&val, type);
        if (type == G_TYPE_BOOLEAN) {
            g_value_set_boolean(&val, FALSE);
        } else if (type == G_TYPE_INT) {
            g_value_set_int(&val, 0);
        } else if (type == G_TYPE_UINT) {
            g_value_set_uint(&val, 0);
        } else if (type == G_TYPE_DOUBLE) {
            g_value_set_double(&val, strcmp(name, "opacity") == 0 ? 100.0 : 0.0);
        } else if (type == G_TYPE_STRING) {
            g_value_set_string(&val, strcmp(name, "font") == 0 ? "Sans 16px" : "");
        } else if (type == GEGL_TYPE_COLOR) {
            GeglColor *black = gegl_color_new("black");
            g_value_set_object(&val, black);
            g_object_unref(black);
        }
        gimp_value_array_append(temp_settings->params, &val);
        g_value_unset(&val);
    }
    g_free(args);
}

static void
update_selected_procedure(gchar *procedure)
{
    if (temp_settings != NULL &&
        strcmp(temp_settings->procedure, procedure) == 0)
        return;
    GimpPDB       *pdb  = gimp_get_pdb();
    GimpProcedure *proc = gimp_pdb_lookup_procedure(pdb, procedure);
    if (!proc) return;
    gint        n_args;
    GParamSpec **args = gimp_procedure_get_arguments(proc, &n_args);
    init_temp_settings(procedure, n_args, args);
    update_procedure_box(temp_settings);
}

static int
setup_procedure_chooser(GtkWidget *entry_search, userdef_settings settings)
{
    treeview_procedures = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview_procedures), FALSE);
    init_procedure_list();
    int sel = fill_procedure_list(NULL, settings->procedure);
    treesel_proc = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_procedures));
    gtk_tree_selection_set_select_function(treesel_proc, select_procedure, NULL, NULL);
    scroll_procparam = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_procparam),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    g_signal_connect(G_OBJECT(entry_search), "changed",
                     G_CALLBACK(search_procedure), NULL);
    return sel;
}

static GtkWidget *
build_chooser_grid(GtkWidget *entry_search)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    GtkWidget *scroll_procedures = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_procedures),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_procedures, PROCLIST_W, PROCLIST_H);
    GtkWidget *label_search = gtk_label_new(g_strconcat(_("Search"), ":", NULL));
    gtk_grid_attach(GTK_GRID(grid), label_search,     0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_search,     1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroll_procparam, 2, 0, 1, 2);
    gtk_widget_set_hexpand(scroll_procparam, TRUE);
    gtk_widget_set_vexpand(scroll_procparam, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procedures),
                                  treeview_procedures);
    gtk_grid_attach(GTK_GRID(grid), scroll_procedures, 0, 1, 2, 1);
    gtk_widget_set_vexpand(scroll_procedures, TRUE);
    return grid;
}

static void
restore_from_settings(userdef_settings settings, int sel_index)
{
    if (settings->procedure == NULL) return;
    temp_settings = (userdef_settings)g_malloc(sizeof(struct manip_userdef_set));
    temp_settings->procedure = g_strdup(settings->procedure);
    gint nparams = settings->params ? gimp_value_array_length(settings->params) : 0;
    temp_settings->params = gimp_value_array_new(nparams);
    for (int i = 0; i < nparams; i++) {
        GValue *src = gimp_value_array_index(settings->params, i);
        GValue copy = G_VALUE_INIT;
        g_value_init(&copy, G_VALUE_TYPE(src));
        g_value_copy(src, &copy);
        gimp_value_array_append(temp_settings->params, &copy);
        g_value_unset(&copy);
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices(sel_index, -1);
    gtk_tree_selection_select_path(treesel_proc, path);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview_procedures),
                                 path, NULL, TRUE, 0.5, 0.0);
}

GtkWidget *
bimp_userdef_gui_new(userdef_settings settings, GtkWidget *parent)
{
    parent_dialog = parent;
    GtkWidget *gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *label_help = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_help),
        _("Choose a supported GIMP procedure from the list on the left\n"
          "and define its parameters on the right."));
    gtk_label_set_justify(GTK_LABEL(label_help), GTK_JUSTIFY_CENTER);
    GtkWidget *entry_search = gtk_entry_new();
    int sel = setup_procedure_chooser(entry_search, settings);
    GtkWidget *grid = build_chooser_grid(entry_search);
    gtk_box_append(GTK_BOX(gui), label_help);
    gtk_box_append(GTK_BOX(gui), grid);
    gtk_widget_set_vexpand(grid, TRUE);
    restore_from_settings(settings, sel);
    gtk_window_set_default_size(GTK_WINDOW(parent), PROCLIST_W * 3, -1);
    update_procedure_box(settings);
    return gui;
}
