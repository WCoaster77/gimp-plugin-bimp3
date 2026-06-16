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

static GtkWidget *build_int_combo(GParamSpec *, GValue *, GRegex *);
static GtkWidget *build_int_spin(GParamSpec *, GValue *, GRegex *);
static GtkWidget *build_int_param(GParamSpec *, GValue *, GRegex *, GRegex *);
static GtkWidget *build_double_param(GParamSpec *, GValue *, GRegex *);
static GtkWidget *make_param_widget(GParamSpec *, GValue *, userdef_settings, GRegex *, GRegex *);
static void       build_proc_info_labels(GtkWidget *, GimpProcedure *, const char *);
static int        build_param_rows(GtkWidget *, userdef_settings, GParamSpec **, gint, GRegex *, GRegex *);

static GtkWidget *
build_int_combo(GParamSpec *spec, GValue *val, GRegex *reg_combo)
{
    const char  *desc = g_param_spec_get_blurb(spec);
    GMatchInfo  *match_info;
    if (!g_regex_match(reg_combo, desc ? desc : "", 0, &match_info)) {
        g_match_info_free(match_info);
        return NULL;
    }
    GtkListStore *store   = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    GtkTreeIter   selected;
    gint          cur_val = val ? g_value_get_int(val) : 0;
    while (g_match_info_matches(match_info)) {
        GtkTreeIter it;
        gtk_list_store_append(store, &it);
        const char *iname = g_match_info_fetch(match_info, 1);
        const char *id_s  = g_match_info_fetch(match_info, 2);
        int id = id_s && strlen(id_s) > 0 ? atoi(id_s) : 0;
        gtk_list_store_set(store, &it, 0, iname, 1, id, -1);
        if (id == cur_val) selected = it;
        g_match_info_next(match_info, NULL);
    }
    g_match_info_free(match_info);
    GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(G_OBJECT(store));
    GtkCellRenderer *cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell, "text", 0, NULL);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &selected);
    return combo;
}

static GtkWidget *
build_int_spin(GParamSpec *spec, GValue *val, GRegex *reg_minmax)
{
    const char *desc = g_param_spec_get_blurb(spec);
    GtkWidget  *spin = gtk_spin_button_new(NULL, 1, 0);
    int min = G_MININT, max = G_MAXINT;
    GMatchInfo *mi;
    if (g_regex_match(reg_minmax, desc ? desc : "", 0, &mi)) {
        while (g_match_info_matches(mi)) {
            gchar *lv = g_match_info_fetch(mi, 1);
            gchar *ls = g_match_info_fetch(mi, 2);
            gchar *rs = g_match_info_fetch(mi, 4);
            gchar *rv = g_match_info_fetch(mi, 5);
            if (lv && strlen(lv) > 0) min = atoi(lv);
            if (ls && strlen(ls) > 0 && strcmp(ls, "<") == 0) min++;
            if (rs && rv) {
                if (rs[0] == '>') { min = atoi(rv); if (strcmp(rs, ">=") != 0) min++; }
                else              { max = atoi(rv); if (strcmp(rs, "<=") != 0) max--; }
            }
            g_match_info_next(mi, NULL);
        }
    }
    g_match_info_free(mi);
    gtk_spin_button_configure(GTK_SPIN_BUTTON(spin),
        GTK_ADJUSTMENT(gtk_adjustment_new(
            val ? (gdouble)g_value_get_int(val) : 0.0, min, max, 1, 1, 0)), 0, 0);
    return spin;
}

static GtkWidget *
build_int_param(GParamSpec *spec, GValue *val, GRegex *reg_combo, GRegex *reg_minmax)
{
    GtkWidget *w = build_int_combo(spec, val, reg_combo);
    return w ? w : build_int_spin(spec, val, reg_minmax);
}

static GtkWidget *
build_double_param(GParamSpec *spec, GValue *val, GRegex *reg_minmax)
{
    const char *desc = g_param_spec_get_blurb(spec);
    const char *name = g_param_spec_get_name(spec);
    GtkWidget  *spin = gtk_spin_button_new(NULL, 1, 1);
    double fmin = -G_MAXDOUBLE, fmax = G_MAXDOUBLE;
    GMatchInfo *mi;
    if (g_regex_match(reg_minmax, desc ? desc : "", 0, &mi)) {
        while (g_match_info_matches(mi)) {
            gchar *lv = g_match_info_fetch(mi, 1);
            gchar *ls = g_match_info_fetch(mi, 2);
            gchar *rs = g_match_info_fetch(mi, 4);
            gchar *rv = g_match_info_fetch(mi, 5);
            if (lv && strlen(lv) > 0) fmin = atof(lv);
            if (ls && strlen(ls) > 0 && strcmp(ls, "<") == 0) fmin += 0.1;
            if (rs && rv) {
                if (rs[0] == '>') { fmin = atof(rv); if (strcmp(rs, ">=") != 0) fmin += 0.1; }
                else              { fmax = atof(rv); if (strcmp(rs, "<=") != 0) fmax -= 0.1; }
            }
            g_match_info_next(mi, NULL);
        }
    } else {
        if (strcmp(name, "opacity") == 0 ||
            (desc && (str_contains_cins((char*)desc, "%") ||
                      str_contains_cins((char*)desc, "percent"))))
            fmin = 0.0, fmax = 100.0;
    }
    g_match_info_free(mi);
    gtk_spin_button_configure(GTK_SPIN_BUTTON(spin),
        GTK_ADJUSTMENT(gtk_adjustment_new(
            val ? g_value_get_double(val) : 0.0, fmin, fmax, 0.1, 1, 0)), 0, 1);
    return spin;
}

static GtkWidget *
make_param_widget(GParamSpec *spec, GValue *val, userdef_settings settings,
                  GRegex *reg_combo, GRegex *reg_minmax)
{
    GType       type = G_PARAM_SPEC_VALUE_TYPE(spec);
    const char *name = g_param_spec_get_name(spec);
    if (strcmp(name, "run-mode") == 0 ||
        g_type_is_a(type, GIMP_TYPE_IMAGE) ||
        g_type_is_a(type, GIMP_TYPE_DRAWABLE) ||
        g_type_is_a(type, GIMP_TYPE_ITEM))
        return NULL;
    if (type == G_TYPE_BOOLEAN) {
        GtkWidget *w = gtk_check_button_new();
        gtk_check_button_set_active(GTK_CHECK_BUTTON(w),
            val ? g_value_get_boolean(val) : FALSE);
        return w;
    }
    if (type == G_TYPE_INT)
        return build_int_param(spec, val, reg_combo, reg_minmax);
    if (type == G_TYPE_UINT) {
        GtkWidget *w = gtk_spin_button_new(NULL, 1, 0);
        gtk_spin_button_configure(GTK_SPIN_BUTTON(w),
            GTK_ADJUSTMENT(gtk_adjustment_new(
                val ? (gdouble)g_value_get_uint(val) : 0.0, 0, 255, 1, 1, 0)), 0, 0);
        return w;
    }
    if (type == G_TYPE_DOUBLE)
        return build_double_param(spec, val, reg_minmax);
    if (type == G_TYPE_STRING) {
        const char *cur = val ? g_value_get_string(val) : "";
        if (strcmp(name, "font") == 0)
            return gtk_font_button_new_with_font(cur ? cur : "Sans 16px");
        if (strstr(settings->procedure, "-save") != NULL &&
            (strcmp(name, "filename") == 0 || strcmp(name, "raw-filename") == 0))
            return NULL;
        GtkWidget *e = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(e), cur ? cur : "");
        return e;
    }
    if (type == GEGL_TYPE_COLOR || g_type_is_a(type, GEGL_TYPE_COLOR)) {
        GeglColor *gc = val ? GEGL_COLOR(g_value_get_object(val)) : NULL;
        GdkRGBA rgba = {0, 0, 0, 1};
        if (gc) gegl_color_get_rgba(gc, &rgba.red, &rgba.green, &rgba.blue, &rgba.alpha);
        return gtk_color_button_new_with_rgba(&rgba);
    }
    return NULL;
}

static void
build_proc_info_labels(GtkWidget *vbox, GimpProcedure *proc, const char *proc_name)
{
    GtkWidget *lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), g_strconcat("<b>", proc_name, "</b>", NULL));
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_box_append(GTK_BOX(vbox), lbl);
    const char *blurb = gimp_procedure_get_blurb(proc);
    if (blurb) {
        GtkWidget *ld = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(ld),
            g_strconcat("<i>", g_markup_escape_text(blurb, -1), "</i>", NULL));
        gtk_label_set_xalign(GTK_LABEL(ld), 0.0f);
        gtk_box_append(GTK_BOX(vbox), ld);
    }
    const char *author = gimp_procedure_get_authors(proc);
    if (author) {
        GtkWidget *la = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(la),
            g_strconcat(_("Author"), ": ", g_markup_escape_text(author, -1), NULL));
        gtk_label_set_xalign(GTK_LABEL(la), 0.0f);
        gtk_box_append(GTK_BOX(vbox), la);
    }
}

static int
build_param_rows(GtkWidget *vbox, userdef_settings settings,
                 GParamSpec **args, gint n_args,
                 GRegex *reg_combo, GRegex *reg_minmax)
{
    gint nparams    = settings->params ? gimp_value_array_length(settings->params) : 0;
    gint loop_count = MIN(n_args, nparams);
    int  show_count = 0;
    g_free(param_widget);
    param_widget = g_new(GtkWidget *, n_args);
    for (int i = 0; i < n_args; i++) param_widget[i] = NULL;
    for (int i = 0; i < loop_count; i++) {
        GValue    *val = gimp_value_array_index(settings->params, i);
        GtkWidget *pw  = make_param_widget(args[i], val, settings, reg_combo, reg_minmax);
        param_widget[i] = pw;
        if (pw) {
            const char *desc = g_param_spec_get_blurb(args[i]);
            GtkWidget *label = gtk_label_new(desc);
            gtk_widget_set_tooltip_text(label, g_param_spec_get_name(args[i]));
            gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
            GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            gtk_box_append(GTK_BOX(row), pw);
            gtk_box_append(GTK_BOX(row), label);
            gtk_box_append(GTK_BOX(vbox), row);
            show_count++;
        }
    }
    return show_count;
}

void
update_procedure_box(userdef_settings settings)
{
    GtkWidget *old = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scroll_procparam));
    if (old) gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam), NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    if (settings->procedure == NULL) {
        GtkWidget *lbl = gtk_label_new(
            _("Can't save because\nno procedure has been selected"));
        gtk_label_set_justify(GTK_LABEL(lbl), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(vbox), lbl);
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent_dialog),
                                          GTK_RESPONSE_ACCEPT, FALSE);
    } else {
        GimpPDB       *pdb  = gimp_get_pdb();
        GimpProcedure *proc = gimp_pdb_lookup_procedure(pdb, settings->procedure);
        if (!proc) {
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam), vbox);
            return;
        }
        gint        n_args;
        GParamSpec **args = gimp_procedure_get_arguments(proc, &n_args);
        build_proc_info_labels(vbox, proc, settings->procedure);
        GRegex *reg_combo  = g_regex_new(
            "([A-Za-z\\d-_]+)\\s\\((\\d+)\\)", 0, 0, NULL);
        GRegex *reg_minmax = g_regex_new(
            "(?:(-?[\\d,\\.]+)\\s([<|>]{1}=?)\\s)?([\\w|-]+)\\s([<|>]{1}=?)\\s(-?[\\d,\\.]+)",
            0, 0, NULL);
        int show_count = build_param_rows(vbox, settings, args, n_args,
                                          reg_combo, reg_minmax);
        if (show_count == 0) {
            GtkWidget *lbl = gtk_label_new(
                _("This procedure takes no editable params"));
            gtk_label_set_justify(GTK_LABEL(lbl), GTK_JUSTIFY_LEFT);
            gtk_box_append(GTK_BOX(vbox), lbl);
        }
        g_regex_unref(reg_combo);
        g_regex_unref(reg_minmax);
        g_free(args);
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent_dialog),
                                          GTK_RESPONSE_ACCEPT, TRUE);
    }
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_procparam), vbox);
}
