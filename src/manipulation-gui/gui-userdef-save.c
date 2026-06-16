#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <gegl.h>
#include <string.h>
#include "gui-userdef.h"
#include "gui-userdef-priv.h"
#include "../bimp-manipulations.h"

static void
save_param_value(GParamSpec *spec, GValue *val, GtkWidget *widget)
{
    GType       type = G_PARAM_SPEC_VALUE_TYPE(spec);
    const char *name = g_param_spec_get_name(spec);
    if (widget == NULL) {
        if (strcmp(name, "run-mode") == 0 && G_TYPE_IS_ENUM(type))
            g_value_set_enum(val, GIMP_RUN_NONINTERACTIVE);
    } else if (type == G_TYPE_BOOLEAN) {
        g_value_set_boolean(val,
            gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)));
    } else if (type == G_TYPE_INT) {
        if (GTK_IS_COMBO_BOX(widget)) {
            GtkComboBox *combo = GTK_COMBO_BOX(widget);
            GtkTreeIter  iter;
            int selected = 0;
            if (gtk_combo_box_get_active_iter(combo, &iter))
                gtk_tree_model_get(gtk_combo_box_get_model(combo), &iter,
                                   1, &selected, -1);
            g_value_set_int(val, selected);
        } else if (GTK_IS_SPIN_BUTTON(widget)) {
            g_value_set_int(val,
                (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
        }
    } else if (type == G_TYPE_UINT) {
        g_value_set_uint(val,
            (guint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
    } else if (type == G_TYPE_DOUBLE) {
        g_value_set_double(val,
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
    } else if (type == G_TYPE_STRING) {
        if (strcmp(name, "font") == 0)
            g_value_set_string(val,
                gtk_font_chooser_get_font(GTK_FONT_CHOOSER(widget)));
        else
            g_value_set_string(val,
                gtk_editable_get_text(GTK_EDITABLE(widget)));
    } else if (type == GEGL_TYPE_COLOR || g_type_is_a(type, GEGL_TYPE_COLOR)) {
        GdkRGBA rgba;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &rgba);
        GeglColor *gc = gegl_color_new(NULL);
        gegl_color_set_rgba(gc, rgba.red, rgba.green, rgba.blue, rgba.alpha);
        g_value_set_object(val, gc);
        g_object_unref(gc);
    }
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
    for (int i = 0; i < n_args; i++) {
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_PARAM_SPEC_VALUE_TYPE(args[i]));
        save_param_value(args[i], &val, param_widget[i]);
        gimp_value_array_append(orig_settings->params, &val);
        g_value_unset(&val);
    }
    g_free(args);
}
