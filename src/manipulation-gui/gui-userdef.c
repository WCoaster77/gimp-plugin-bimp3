#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <gegl.h>
#include <stdlib.h>
#include <string.h>
#include "gui-userdef.h"
#include "../bimp.h"
#include "../bimp-gui.h"
#include "../bimp-manipulations.h"
#include "../bimp-utils.h"
#include "../plugin-intl.h"

static void init_procedure_list(void);
static int  fill_procedure_list(char *, char *);
static void search_procedure(GtkEditable *, gpointer);
static gboolean select_procedure(GtkTreeSelection *, GtkTreeModel *,
                                  GtkTreePath *, gboolean, gpointer);
static void update_selected_procedure(gchar *);
static void update_procedure_box(userdef_settings);

static GtkWidget *treeview_procedures;
static GtkWidget *scroll_procparam;
static GtkWidget *parent_dialog;

static userdef_settings temp_settings;
static GtkWidget **param_widget;

static GtkTreeSelection *treesel_proc;

enum { LIST_ITEM = 0, N_COLUMNS };

GtkWidget *
bimp_userdef_gui_new(userdef_settings settings, GtkWidget *parent)
{
    GtkWidget *gui, *grid_chooser;
    GtkWidget *scroll_procedures;
    GtkWidget *label_help, *label_search, *entry_search;

    parent_dialog = parent;

    gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    label_help = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_help),
        _("Choose a supported GIMP procedure from the list on the left\n"
          "and define its parameters on the right."));
    gtk_label_set_justify(GTK_LABEL(label_help), GTK_JUSTIFY_CENTER);

    grid_chooser = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_chooser), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid_chooser), 5);

    scroll_procedures = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_procedures),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_procedures, PROCLIST_W, PROCLIST_H);

    label_search = gtk_label_new(g_strconcat(_("Search"), ":", NULL));
    entry_search = gtk_entry_new();

    treeview_procedures = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview_procedures), FALSE);
    init_procedure_list();
    int sel_index = fill_procedure_list(NULL, settings->procedure);
    treesel_proc = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_procedures));
    gtk_tree_selection_set_select_function(treesel_proc, select_procedure, NULL, NULL);

    scroll_procparam = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_procparam),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_grid_attach(GTK_GRID(grid_chooser), label_search,      0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_chooser), entry_search,      1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_chooser), scroll_procparam,  2, 0, 1, 2);
    gtk_widget_set_hexpand(scroll_procparam, TRUE);
    gtk_widget_set_vexpand(scroll_procparam, TRUE);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procedures),
                                  treeview_procedures);
    gtk_grid_attach(GTK_GRID(grid_chooser), scroll_procedures, 0, 1, 2, 1);
    gtk_widget_set_vexpand(scroll_procedures, TRUE);

    gtk_box_append(GTK_BOX(gui), label_help);
    gtk_box_append(GTK_BOX(gui), grid_chooser);
    gtk_widget_set_vexpand(grid_chooser, TRUE);

    if (settings->procedure != NULL) {
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

    gtk_window_set_default_size(GTK_WINDOW(parent), PROCLIST_W * 3, -1);

    update_procedure_box(settings);

    g_signal_connect(G_OBJECT(entry_search), "changed",
                     G_CALLBACK(search_procedure), NULL);

    return gui;
}

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
            g_value_set_double(&val,
                strcmp(name, "opacity") == 0 ? 100.0 : 0.0);
        } else if (type == G_TYPE_STRING) {
            g_value_set_string(&val,
                strcmp(name, "font") == 0 ? "Sans 16px" : "");
        } else if (type == GEGL_TYPE_COLOR) {
            GeglColor *black = gegl_color_new("black");
            g_value_set_object(&val, black);
            g_object_unref(black);
        }

        gimp_value_array_append(temp_settings->params, &val);
        g_value_unset(&val);
    }
    g_free(args);

    update_procedure_box(temp_settings);
}

static void
update_procedure_box(userdef_settings settings)
{
    GtkWidget *old = gtk_scrolled_window_get_child(
        GTK_SCROLLED_WINDOW(scroll_procparam));
    if (old != NULL)
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam), NULL);

    GtkWidget *vbox_procparam = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    if (settings->procedure == NULL) {
        GtkWidget *label_noproc = gtk_label_new(
            _("Can't save because\nno procedure has been selected"));
        gtk_label_set_justify(GTK_LABEL(label_noproc), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(vbox_procparam), label_noproc);
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent_dialog),
                                          GTK_RESPONSE_ACCEPT, FALSE);
    } else {
        GimpPDB       *pdb  = gimp_get_pdb();
        GimpProcedure *proc = gimp_pdb_lookup_procedure(pdb, settings->procedure);
        if (!proc) {
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam),
                                          vbox_procparam);
            return;
        }

        const char *proc_blurb  = gimp_procedure_get_blurb(proc);
        const char *proc_author = gimp_procedure_get_authors(proc);

        gint        n_args;
        GParamSpec **args = gimp_procedure_get_arguments(proc, &n_args);

        GtkWidget *label_procname = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label_procname),
            g_strconcat("<b>", settings->procedure, "</b>", NULL));
        gtk_label_set_xalign(GTK_LABEL(label_procname), 0.0f);

        GtkWidget *label_procdescr = gtk_label_new(NULL);
        if (proc_blurb) {
            gtk_label_set_markup(GTK_LABEL(label_procdescr),
                g_strconcat("<i>", g_markup_escape_text(proc_blurb, -1), "</i>", NULL));
            gtk_label_set_xalign(GTK_LABEL(label_procdescr), 0.0f);
        }

        GtkWidget *label_procauthor = gtk_label_new(NULL);
        if (proc_author) {
            gtk_label_set_markup(GTK_LABEL(label_procauthor),
                g_strconcat(_("Author"), ": ",
                            g_markup_escape_text(proc_author, -1), NULL));
            gtk_label_set_xalign(GTK_LABEL(label_procauthor), 0.0f);
        }

        gtk_box_append(GTK_BOX(vbox_procparam), label_procname);
        gtk_box_append(GTK_BOX(vbox_procparam), label_procdescr);
        gtk_box_append(GTK_BOX(vbox_procparam), label_procauthor);

        g_free(param_widget);
        param_widget = g_new(GtkWidget *, n_args);

        gint nparams = settings->params
            ? gimp_value_array_length(settings->params) : 0;
        gint loop_count = MIN(n_args, nparams);

        int show_count = 0;
        GRegex *reg_combobox = g_regex_new(
            "([A-Za-z\\d-_]+)\\s\\((\\d+)\\)", 0, 0, NULL);
        GRegex *reg_minmax = g_regex_new(
            "(?:(-?[\\d,\\.]+)\\s([<|>]{1}=?)\\s)?([\\w|-]+)\\s([<|>]{1}=?)\\s(-?[\\d,\\.]+)",
            0, 0, NULL);

        for (int param_i = 0; param_i < loop_count; param_i++) {
            GParamSpec *spec  = args[param_i];
            GType       type  = G_PARAM_SPEC_VALUE_TYPE(spec);
            const char *name  = g_param_spec_get_name(spec);
            const char *desc  = g_param_spec_get_blurb(spec);
            GValue     *val   = gimp_value_array_index(settings->params, param_i);
            gboolean    editable = TRUE;

            param_widget[param_i] = NULL;

            if (strcmp(name, "run-mode") == 0 ||
                g_type_is_a(type, GIMP_TYPE_IMAGE) ||
                g_type_is_a(type, GIMP_TYPE_DRAWABLE) ||
                g_type_is_a(type, GIMP_TYPE_ITEM)) {
                editable = FALSE;

            } else if (type == G_TYPE_BOOLEAN) {
                param_widget[param_i] = gtk_check_button_new();
                gtk_check_button_set_active(GTK_CHECK_BUTTON(param_widget[param_i]),
                    val ? g_value_get_boolean(val) : FALSE);

            } else if (type == G_TYPE_INT) {
                GMatchInfo *match_info;
                if (g_regex_match(reg_combobox, desc ? desc : "", 0, &match_info)) {
                    GtkTreeIter   selected;
                    GtkListStore *combo_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
                    gint cur_val = val ? g_value_get_int(val) : 0;

                    while (g_match_info_matches(match_info)) {
                        GtkTreeIter  it;
                        gtk_list_store_append(combo_store, &it);
                        const char *iname = g_match_info_fetch(match_info, 1);
                        const char *id_s  = g_match_info_fetch(match_info, 2);
                        int id = id_s && strlen(id_s) > 0 ? atoi(id_s) : 0;
                        gtk_list_store_set(combo_store, &it, 0, iname, 1, id, -1);
                        if (id == cur_val) selected = it;
                        g_match_info_next(match_info, NULL);
                    }
                    g_match_info_free(match_info);

                    param_widget[param_i] = gtk_combo_box_new_with_model(
                        GTK_TREE_MODEL(combo_store));
                    g_object_unref(G_OBJECT(combo_store));
                    GtkCellRenderer *cell = gtk_cell_renderer_text_new();
                    gtk_cell_layout_pack_start(
                        GTK_CELL_LAYOUT(param_widget[param_i]), cell, TRUE);
                    gtk_cell_layout_set_attributes(
                        GTK_CELL_LAYOUT(param_widget[param_i]), cell, "text", 0, NULL);
                    gtk_combo_box_set_active_iter(
                        GTK_COMBO_BOX(param_widget[param_i]), &selected);
                } else {
                    g_match_info_free(match_info);
                    param_widget[param_i] = gtk_spin_button_new(NULL, 1, 0);
                    int min = G_MININT, max = G_MAXINT;
                    GMatchInfo *mi2;
                    if (g_regex_match(reg_minmax, desc ? desc : "", 0, &mi2)) {
                        while (g_match_info_matches(mi2)) {
                            gchar *lv = g_match_info_fetch(mi2, 1);
                            if (lv && strlen(lv) > 0) min = atoi(lv);
                            gchar *ls = g_match_info_fetch(mi2, 2);
                            if (ls && strlen(ls) > 0 && strcmp(ls, "<") == 0) min++;
                            gchar *rs = g_match_info_fetch(mi2, 4);
                            gchar *rv = g_match_info_fetch(mi2, 5);
                            if (rs && rv) {
                                if (rs[0] == '>') { min = atoi(rv); if (strcmp(rs, ">=") != 0) min++; }
                                else              { max = atoi(rv); if (strcmp(rs, "<=") != 0) max--; }
                            }
                            g_match_info_next(mi2, NULL);
                        }
                        g_match_info_free(mi2);
                    }
                    gtk_spin_button_configure(GTK_SPIN_BUTTON(param_widget[param_i]),
                        GTK_ADJUSTMENT(gtk_adjustment_new(
                            val ? (gdouble)g_value_get_int(val) : 0.0,
                            min, max, 1, 1, 0)), 0, 0);
                }

            } else if (type == G_TYPE_UINT) {
                param_widget[param_i] = gtk_spin_button_new(NULL, 1, 0);
                gtk_spin_button_configure(GTK_SPIN_BUTTON(param_widget[param_i]),
                    GTK_ADJUSTMENT(gtk_adjustment_new(
                        val ? (gdouble)g_value_get_uint(val) : 0.0,
                        0, 255, 1, 1, 0)), 0, 0);

            } else if (type == G_TYPE_DOUBLE) {
                param_widget[param_i] = gtk_spin_button_new(NULL, 1, 1);
                double fmin = -G_MAXDOUBLE, fmax = G_MAXDOUBLE;
                GMatchInfo *mi3;
                if (g_regex_match(reg_minmax, desc ? desc : "", 0, &mi3)) {
                    while (g_match_info_matches(mi3)) {
                        gchar *lv = g_match_info_fetch(mi3, 1);
                        if (lv && strlen(lv) > 0) fmin = atof(lv);
                        gchar *ls = g_match_info_fetch(mi3, 2);
                        if (ls && strlen(ls) > 0 && strcmp(ls, "<") == 0) fmin += 0.1;
                        gchar *rs = g_match_info_fetch(mi3, 4);
                        gchar *rv = g_match_info_fetch(mi3, 5);
                        if (rs && rv) {
                            if (rs[0] == '>') { fmin = atof(rv); if (strcmp(rs, ">=") != 0) fmin += 0.1; }
                            else              { fmax = atof(rv); if (strcmp(rs, "<=") != 0) fmax -= 0.1; }
                        }
                        g_match_info_next(mi3, NULL);
                    }
                    g_match_info_free(mi3);
                } else {
                    g_match_info_free(mi3);
                    if (strcmp(name, "opacity") == 0 ||
                        (desc && (str_contains_cins((char*)desc, "%") ||
                                  str_contains_cins((char*)desc, "percent")))) {
                        fmin = 0.0; fmax = 100.0;
                    }
                }
                gtk_spin_button_configure(GTK_SPIN_BUTTON(param_widget[param_i]),
                    GTK_ADJUSTMENT(gtk_adjustment_new(
                        val ? g_value_get_double(val) : 0.0,
                        fmin, fmax, 0.1, 1, 0)), 0, 1);

            } else if (type == G_TYPE_STRING) {
                const char *cur = val ? g_value_get_string(val) : "";
                if (strcmp(name, "font") == 0) {
                    param_widget[param_i] = gtk_font_button_new_with_font(
                        cur ? cur : "Sans 16px");
                } else if (strstr(settings->procedure, "-save") != NULL &&
                           (strcmp(name, "filename") == 0 ||
                            strcmp(name, "raw-filename") == 0)) {
                    editable = FALSE;
                } else {
                    param_widget[param_i] = gtk_entry_new();
                    gtk_editable_set_text(GTK_EDITABLE(param_widget[param_i]),
                                          cur ? cur : "");
                }

            } else if (type == GEGL_TYPE_COLOR || g_type_is_a(type, GEGL_TYPE_COLOR)) {
                GeglColor *gc = val ? GEGL_COLOR(g_value_get_object(val)) : NULL;
                GdkRGBA rgba = {0, 0, 0, 1};
                if (gc) gegl_color_get_rgba(gc, &rgba.red, &rgba.green,
                                                &rgba.blue, &rgba.alpha);
                param_widget[param_i] = gtk_color_button_new_with_rgba(&rgba);

            } else {
                editable = FALSE;
            }

            if (editable && param_widget[param_i]) {
                GtkWidget *label_widget_desc = gtk_label_new(desc);
                gtk_widget_set_tooltip_text(label_widget_desc, name);
                gtk_label_set_xalign(GTK_LABEL(label_widget_desc), 0.0f);

                GtkWidget *hbox_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
                gtk_box_append(GTK_BOX(hbox_row), param_widget[param_i]);
                gtk_box_append(GTK_BOX(hbox_row), label_widget_desc);
                gtk_box_append(GTK_BOX(vbox_procparam), hbox_row);
                show_count++;
            } else {
                param_widget[param_i] = NULL;
            }
        }

        if (show_count == 0) {
            GtkWidget *label_noparams = gtk_label_new(
                _("This procedure takes no editable params"));
            gtk_label_set_justify(GTK_LABEL(label_noparams), GTK_JUSTIFY_LEFT);
            gtk_box_append(GTK_BOX(vbox_procparam), label_noparams);
        }

        g_regex_unref(reg_combobox);
        g_regex_unref(reg_minmax);
        g_free(args);

        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent_dialog),
                                          GTK_RESPONSE_ACCEPT, TRUE);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam),
                                  vbox_procparam);
}

void
bimp_userdef_save(userdef_settings orig_settings)
{
    g_free(orig_settings->procedure);
    orig_settings->procedure = g_strdup(temp_settings->procedure);

    GimpPDB       *pdb  = gimp_get_pdb();
    GimpProcedure *proc = gimp_pdb_lookup_procedure(pdb, orig_settings->procedure);
    if (!proc) return;

    gint        n_args;
    GParamSpec **args = gimp_procedure_get_arguments(proc, &n_args);

    if (orig_settings->params) gimp_value_array_unref(orig_settings->params);
    orig_settings->params = gimp_value_array_new(n_args);

    for (int param_i = 0; param_i < n_args; param_i++) {
        GParamSpec *spec = args[param_i];
        GType       type = G_PARAM_SPEC_VALUE_TYPE(spec);
        const char *name = g_param_spec_get_name(spec);
        const char *desc = g_param_spec_get_blurb(spec);
        GValue      val  = G_VALUE_INIT;
        g_value_init(&val, type);

        if (param_widget[param_i] == NULL) {
            if (strcmp(name, "run-mode") == 0 && G_TYPE_IS_ENUM(type))
                g_value_set_enum(&val, GIMP_RUN_NONINTERACTIVE);

        } else if (type == G_TYPE_BOOLEAN) {
            g_value_set_boolean(&val,
                gtk_check_button_get_active(GTK_CHECK_BUTTON(param_widget[param_i])));

        } else if (type == G_TYPE_INT) {
            if (GTK_IS_COMBO_BOX(param_widget[param_i])) {
                GtkComboBox *combo = GTK_COMBO_BOX(param_widget[param_i]);
                GtkTreeIter  iter;
                int selected = 0;
                if (gtk_combo_box_get_active_iter(combo, &iter)) {
                    GtkTreeModel *model = gtk_combo_box_get_model(combo);
                    gtk_tree_model_get(model, &iter, 1, &selected, -1);
                }
                g_value_set_int(&val, selected);
            } else if (GTK_IS_SPIN_BUTTON(param_widget[param_i])) {
                g_value_set_int(&val,
                    (gint)gtk_spin_button_get_value(
                        GTK_SPIN_BUTTON(param_widget[param_i])));
            }

        } else if (type == G_TYPE_UINT) {
            g_value_set_uint(&val,
                (guint)gtk_spin_button_get_value(
                    GTK_SPIN_BUTTON(param_widget[param_i])));

        } else if (type == G_TYPE_DOUBLE) {
            g_value_set_double(&val,
                gtk_spin_button_get_value(
                    GTK_SPIN_BUTTON(param_widget[param_i])));

        } else if (type == G_TYPE_STRING) {
            if (strcmp(name, "font") == 0) {
                g_value_set_string(&val,
                    gtk_font_chooser_get_font(
                        GTK_FONT_CHOOSER(param_widget[param_i])));
            } else {
                g_value_set_string(&val,
                    gtk_editable_get_text(GTK_EDITABLE(param_widget[param_i])));
            }

        } else if (type == GEGL_TYPE_COLOR || g_type_is_a(type, GEGL_TYPE_COLOR)) {
            GdkRGBA rgba;
            gtk_color_chooser_get_rgba(
                GTK_COLOR_CHOOSER(param_widget[param_i]), &rgba);
            GeglColor *gc = gegl_color_new(NULL);
            gegl_color_set_rgba(gc, rgba.red, rgba.green, rgba.blue, rgba.alpha);
            g_value_set_object(&val, gc);
            g_object_unref(gc);
        }

        gimp_value_array_append(orig_settings->params, &val);
        g_value_unset(&val);
    }

    g_free(args);
}
