#include <gtk/gtk.h>
#include "gui-crop.h"
#include "../bimp-manipulations.h"
#include "../bimp-manipulations-gui.h"
#include "../plugin-intl.h"

static void toggle_group(GtkCheckButton *, gpointer);
static void set_customratio(GtkComboBox *, gpointer);
static char *crop_preset_get_string(crop_preset);

static GtkWidget *hbox_ratio, *grid_manual, *hbox_customratio;
static GtkWidget *radio_stratio, *radio_manual;
static GtkWidget *combo_ratio, *spin_ratio1, *spin_ratio2;
static GtkWidget *spin_width, *spin_height;
static GtkWidget *hbox_startpos, *combo_startpos;

GtkWidget *
bimp_crop_gui_new(crop_settings settings)
{
    GtkWidget *gui;
    GtkWidget *label_manual_width, *label_manual_height;
    GtkWidget *label_manual_ratio, *label_startpos;
    GtkWidget *box_stratio, *box_manual;
    int        active_index, i;

    gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    radio_stratio = gtk_check_button_new_with_label(
        _("Crop to a standard aspect ratio"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_stratio),
                                !settings->manual);

    hbox_ratio  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    combo_ratio = gtk_combo_box_text_new();
    for (i = 0; i < CROP_PRESET_END; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_ratio),
                                       crop_preset_get_string(i));

    hbox_customratio = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    spin_ratio1 = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->custom_ratio1,
                                          0.1, 100.0, 0.1, 1, 0)), 1, 1);
    label_manual_ratio = gtk_label_new(":");
    spin_ratio2 = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->custom_ratio2,
                                          0.1, 100.0, 0.1, 1, 0)), 1, 1);

    gtk_box_append(GTK_BOX(hbox_customratio), spin_ratio1);
    gtk_box_append(GTK_BOX(hbox_customratio), label_manual_ratio);
    gtk_box_append(GTK_BOX(hbox_customratio), spin_ratio2);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_ratio), settings->ratio);

    radio_manual = gtk_check_button_new_with_label(_("Manual crop (pixel values)"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_manual),
                               GTK_CHECK_BUTTON(radio_stratio));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_manual), settings->manual);

    grid_manual = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_manual), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid_manual), 5);
    gtk_widget_set_margin_start(grid_manual, 10);

    label_manual_width = gtk_label_new(g_strconcat(_("Width"), ": ", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_manual_width), 0.0f);
    spin_width = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->new_w, 1, 40960, 1, 1, 0)),
        1, 0);

    label_manual_height = gtk_label_new(g_strconcat(_("Height"), ": ", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_manual_height), 0.0f);
    spin_height = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->new_h, 1, 40960, 1, 1, 0)),
        1, 0);

    gtk_grid_attach(GTK_GRID(grid_manual), label_manual_width,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manual), spin_width,          1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manual), label_manual_height, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manual), spin_height,         1, 1, 1, 1);

    hbox_startpos  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    label_startpos = gtk_label_new(g_strconcat(_("Start from"), ":", NULL));
    combo_startpos = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_startpos),
                                   _("Center"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_startpos),
                                   _("Top-left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_startpos),
                                   _("Top-right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_startpos),
                                   _("Bottom-left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_startpos),
                                   _("Bottom-right"));

    switch (settings->start_pos) {
        case CROP_START_CC: active_index = 0; break;
        case CROP_START_TL: active_index = 1; break;
        case CROP_START_TR: active_index = 2; break;
        case CROP_START_BL: active_index = 3; break;
        case CROP_START_BR: active_index = 4; break;
        default:            active_index = 0; break;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_startpos), active_index);

    box_stratio = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(box_stratio, 10);
    gtk_box_append(GTK_BOX(hbox_ratio), combo_ratio);
    gtk_box_append(GTK_BOX(hbox_ratio), hbox_customratio);
    gtk_box_append(GTK_BOX(box_stratio), hbox_ratio);

    box_manual = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(box_manual, 10);
    gtk_box_append(GTK_BOX(box_manual), grid_manual);

    gtk_box_append(GTK_BOX(gui), radio_stratio);
    gtk_box_append(GTK_BOX(gui), box_stratio);
    gtk_box_append(GTK_BOX(gui), radio_manual);
    gtk_box_append(GTK_BOX(gui), box_manual);

    gtk_box_append(GTK_BOX(hbox_startpos), label_startpos);
    gtk_box_append(GTK_BOX(hbox_startpos), combo_startpos);
    gtk_box_append(GTK_BOX(gui), hbox_startpos);

    toggle_group(NULL, NULL);
    set_customratio(NULL, NULL);
    g_signal_connect(G_OBJECT(radio_stratio), "toggled",
                     G_CALLBACK(toggle_group), NULL);
    g_signal_connect(G_OBJECT(combo_ratio), "changed",
                     G_CALLBACK(set_customratio), NULL);
    return gui;
}

static void
toggle_group(GtkCheckButton *togglebutton, gpointer user_data)
{
    gboolean stratio =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(radio_stratio));
    gtk_widget_set_sensitive(hbox_ratio,   stratio);
    gtk_widget_set_sensitive(grid_manual, !stratio);
}

static void
set_customratio(GtkComboBox *combobox, gpointer user_data)
{
    gtk_widget_set_sensitive(hbox_customratio,
        gtk_combo_box_get_active(GTK_COMBO_BOX(combo_ratio)) ==
            CROP_PRESET_CUSTOM);
}

static char *
crop_preset_get_string(crop_preset cps)
{
    switch (cps) {
        case CROP_PRESET_11:      return g_strconcat(_("One-to-one"),          " (1:1)",   NULL);
        case CROP_PRESET_32:      return g_strconcat(_("Classic 35 mm film"),  " (3:2)",   NULL);
        case CROP_PRESET_43:      return g_strconcat(_("Standard VGA monitor")," (4:3)",   NULL);
        case CROP_PRESET_169:     return g_strconcat(_("Widescreen"),          " (16:9)",  NULL);
        case CROP_PRESET_1610:    return g_strconcat(_("Widescreen extended"), " (16:10)", NULL);
        case CROP_PRESET_EUPORT:  return g_strconcat(_("EU Passport portrait")," (7:9)",   NULL);
        case CROP_PRESET_PHONE:   return g_strconcat(_("Classic smartphone screen")," (2:3)", NULL);
        case CROP_PRESET_TALLPHONE: return g_strconcat(_("Tall smartphone screen")," (40:71)", NULL);
        case CROP_PRESET_TABLET:  return g_strconcat(_("Classic tablet screen")," (3:4)",  NULL);
        case CROP_PRESET_CUSTOM:  return g_strconcat(_("Custom ratio"),        "...",      NULL);
        default:                  return "";
    }
}

void
bimp_crop_save(crop_settings orig_settings)
{
    orig_settings->new_w =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_width));
    orig_settings->new_h =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_height));
    orig_settings->manual =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(radio_manual));
    orig_settings->ratio =
        gtk_combo_box_get_active(GTK_COMBO_BOX(combo_ratio));
    orig_settings->custom_ratio1 =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_ratio1));
    orig_settings->custom_ratio2 =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_ratio2));
    orig_settings->start_pos =
        gtk_combo_box_get_active(GTK_COMBO_BOX(combo_startpos));
}
