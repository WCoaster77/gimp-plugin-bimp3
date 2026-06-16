#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimpwidgets/gimpwidgets.h>
#include "gui-fliprotate.h"
#include "../bimp-manipulations.h"
#include "../bimp-manipulations-gui.h"
#include "../bimp-utils.h"
#include "../plugin-intl.h"

static GtkWidget *button_flipH, *button_flipV, *combo_rotate;

GtkWidget *
bimp_fliprotate_gui_new(fliprotate_settings settings)
{
    GtkWidget *gui, *hbox_flip, *hbox_rotate;
    GtkWidget *label_flip, *label_rotate;
    GtkWidget *icon_h, *label_h, *box_h;
    GtkWidget *icon_v, *label_v, *box_v;
    int        active_index;

    gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    label_flip = gtk_label_new(g_strconcat(_("Flip"), ":", NULL));
    hbox_flip  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    button_flipH = gtk_toggle_button_new();
    box_h   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    icon_h  = image_new_from_resource(
        "/gimp/plugin/bimp/icons/stock-flip-horizontal.png");
    label_h = gtk_label_new(_("Horizontally"));
    gtk_box_append(GTK_BOX(box_h), icon_h);
    gtk_box_append(GTK_BOX(box_h), label_h);
    gtk_button_set_child(GTK_BUTTON(button_flipH), box_h);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_flipH),
                                 settings->flip_h);

    button_flipV = gtk_toggle_button_new();
    box_v   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    icon_v  = image_new_from_resource(
        "/gimp/plugin/bimp/icons/stock-flip-vertical.png");
    label_v = gtk_label_new(_("Vertically"));
    gtk_box_append(GTK_BOX(box_v), icon_v);
    gtk_box_append(GTK_BOX(box_v), label_v);
    gtk_button_set_child(GTK_BUTTON(button_flipV), box_v);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_flipV),
                                 settings->flip_v);

    hbox_rotate  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    label_rotate = gtk_label_new(g_strconcat(_("Rotation"), ":", NULL));

    combo_rotate = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_rotate), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_rotate), "90°");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_rotate), "180°");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_rotate), "270°");

    if (!settings->rotate) {
        active_index = 0;
    } else {
        switch (settings->rotation_type) {
            case GIMP_ROTATE_90:  active_index = 1; break;
            case GIMP_ROTATE_180: active_index = 2; break;
            case GIMP_ROTATE_270: active_index = 3; break;
            default:              active_index = 0; break;
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_rotate), active_index);

    gtk_box_append(GTK_BOX(hbox_flip), button_flipH);
    gtk_box_append(GTK_BOX(hbox_flip), button_flipV);

    gtk_box_append(GTK_BOX(hbox_rotate), label_rotate);
    gtk_box_append(GTK_BOX(hbox_rotate), combo_rotate);

    gtk_box_append(GTK_BOX(gui), label_flip);
    gtk_box_append(GTK_BOX(gui), hbox_flip);
    gtk_box_append(GTK_BOX(gui), hbox_rotate);

    return gui;
}

void
bimp_fliprotate_save(fliprotate_settings orig_settings)
{
    int active_index;

    orig_settings->flip_h =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button_flipH));
    orig_settings->flip_v =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button_flipV));

    active_index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_rotate));
    orig_settings->rotate = (active_index > 0);
    if (active_index == 1) orig_settings->rotation_type = GIMP_ROTATE_90;
    else if (active_index == 2) orig_settings->rotation_type = GIMP_ROTATE_180;
    else if (active_index == 3) orig_settings->rotation_type = GIMP_ROTATE_270;
}
