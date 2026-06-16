#include <gtk/gtk.h>
#include "gui-color.h"
#include "../bimp-manipulations.h"
#include "../bimp-manipulations-gui.h"
#include "../plugin-intl.h"

static void toggle_curve(GtkToggleButton *, gpointer);

static GtkWidget *scale_bright, *scale_contrast;
static GtkWidget *check_autolevels, *check_grayscale, *check_curve;
static GtkWidget *chooser_curve;

GtkWidget *
bimp_color_gui_new(color_settings settings)
{
    GtkWidget *gui;
    GtkWidget *label_bright, *label_contrast;

    gui = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(gui), 5);
    gtk_grid_set_column_spacing(GTK_GRID(gui), 5);

    label_bright = gtk_label_new(g_strconcat(_("Brightness"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_bright), 0.0f);
    scale_bright = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                            -0.5, +0.5, 0.01);
    gtk_widget_set_hexpand(scale_bright, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_bright), settings->brightness);

    label_contrast = gtk_label_new(g_strconcat(_("Contrast"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_contrast), 0.0f);
    scale_contrast = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                              -0.5, +0.5, 0.01);
    gtk_widget_set_hexpand(scale_contrast, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_contrast), settings->contrast);

    check_grayscale = gtk_check_button_new_with_label(
        _("Convert to grayscale"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_grayscale),
                                settings->grayscale);
    check_autolevels = gtk_check_button_new_with_label(
        _("Automatic color levels correction"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_autolevels),
                                settings->levels_auto);

    check_curve = gtk_check_button_new_with_label(
        _("Change color curve from settings file:"));

    chooser_curve = gtk_file_chooser_button_new(
        _("Select GIMP Curve file"), GTK_FILE_CHOOSER_ACTION_OPEN);

    if (settings->curve_file) {
        GFile *f = g_file_new_for_path(settings->curve_file);
        gtk_file_chooser_set_file(GTK_FILE_CHOOSER(chooser_curve), f, NULL);
        g_object_unref(f);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(check_curve), TRUE);
    } else {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(check_curve), FALSE);
    }

    gtk_grid_attach(GTK_GRID(gui), label_bright,    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gui), scale_bright,    1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gui), label_contrast,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(gui), scale_contrast,  1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(gui), check_grayscale, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(gui), check_autolevels,0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(gui), check_curve,     0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(gui), chooser_curve,   0, 5, 2, 1);

    toggle_curve(NULL, NULL);
    g_signal_connect(G_OBJECT(check_curve), "toggled",
                     G_CALLBACK(toggle_curve), NULL);
    return gui;
}

static void
toggle_curve(GtkToggleButton *togglebutton, gpointer user_data)
{
    gtk_widget_set_sensitive(chooser_curve,
        gtk_check_button_get_active(GTK_CHECK_BUTTON(check_curve)));
}

void
bimp_color_save(color_settings orig_settings)
{
    orig_settings->brightness =
        gtk_range_get_value(GTK_RANGE(scale_bright));
    orig_settings->contrast =
        gtk_range_get_value(GTK_RANGE(scale_contrast));
    orig_settings->grayscale =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(check_grayscale));
    orig_settings->levels_auto =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(check_autolevels));

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_curve))) {
        GFile *f = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser_curve));
        if (f) {
            orig_settings->curve_file = g_file_get_path(f);
            g_object_unref(f);
        } else {
            orig_settings->curve_file = NULL;
        }
    } else {
        orig_settings->curve_file = NULL;
    }
}
