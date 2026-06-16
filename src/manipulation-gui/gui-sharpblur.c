#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "gui-sharpblur.h"
#include "../bimp-manipulations.h"
#include "../plugin-intl.h"

static GtkWidget *scale_sharpblur;

GtkWidget *
bimp_sharpblur_gui_new(sharpblur_settings settings)
{
    GtkWidget *gui, *hbox_control;
    GtkWidget *label_sharp, *label_blur;

    gui           = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    hbox_control  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    label_sharp   = gtk_label_new(_("More sharpen"));
    scale_sharpblur = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                               -100, 100, 1);
    gtk_range_set_value(GTK_RANGE(scale_sharpblur), settings->amount);
    gtk_widget_set_size_request(scale_sharpblur, SCALE_AMOUNT_W, -1);
    gtk_widget_set_hexpand(scale_sharpblur, TRUE);
    label_blur    = gtk_label_new(_("More blurred"));

    gtk_box_append(GTK_BOX(hbox_control), label_sharp);
    gtk_box_append(GTK_BOX(hbox_control), scale_sharpblur);
    gtk_box_append(GTK_BOX(hbox_control), label_blur);

    gtk_box_append(GTK_BOX(gui), hbox_control);
    return gui;
}

void
bimp_sharpblur_save(sharpblur_settings orig_settings)
{
    orig_settings->amount = (int)gtk_range_get_value(GTK_RANGE(scale_sharpblur));
}
