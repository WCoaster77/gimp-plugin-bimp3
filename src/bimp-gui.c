#include <gtk/gtk.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp-gui-priv.h"
#include "bimp-manipulations-gui.h"
#include "bimp-operate.h"
#include "bimp-serialize.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

GtkWidget    *bimp_window_main    = NULL;
GtkWidget    *panel_sequence      = NULL;
GtkWidget    *panel_options       = NULL;
GtkWidget    *progressbar_visible = NULL;
const gchar  *progressbar_data    = NULL;

/* -------------------------------------------------------------------------
 * Popover helpers — used by sequence.c and panel.c via bimp-gui-priv.h
 * ---------------------------------------------------------------------- */

void
popover_on_closed(GtkWidget *popover, gpointer data)
{
    gtk_widget_unparent(popover);
}

void
popup_and_track(GtkWidget *popover, GtkWidget *parent)
{
    gtk_widget_set_parent(popover, parent);
    g_signal_connect(popover, "closed", G_CALLBACK(popover_on_closed), NULL);
    gtk_popover_popup(GTK_POPOVER(popover));
}

GtkWidget *
popover_button(const char *label, GCallback cb, gpointer cb_data)
{
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
    if (cb) g_signal_connect(btn, "clicked", cb, cb_data);
    return btn;
}

void
close_popover_ancestor(GtkWidget *widget, gpointer data)
{
    GtkWidget *pop = gtk_widget_get_ancestor(widget, GTK_TYPE_POPOVER);
    if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
}

/* -------------------------------------------------------------------------
 * Progress bar (defined early — used by setup_main_content)
 * ---------------------------------------------------------------------- */

static void progressbar_start_hidden(const gchar *m, gboolean c, gpointer u)
{ (void)m; (void)c; (void)u; }
static void progressbar_end_hidden(gpointer u)
{ (void)u; }
static void progressbar_settext_hidden(const gchar *m, gpointer u)
{ (void)m; (void)u; }
static void progressbar_setvalue_hidden(double p, gpointer u)
{ (void)p; (void)u; }

static const gchar *
progressbar_init_hidden(void)
{
    GimpProgressVtable vt = { 0, };
    vt.start     = progressbar_start_hidden;
    vt.end       = progressbar_end_hidden;
    vt.set_text  = progressbar_settext_hidden;
    vt.set_value = progressbar_setvalue_hidden;
    return gimp_progress_install_vtable(&vt, NULL);
}

void
bimp_progress_bar_set(double fraction, char *text)
{
    if (fraction > 1.0) fraction = 1.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar_visible), fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar_visible),
                              text ? text : " ");
}

/* -------------------------------------------------------------------------
 * About dialog (defined early — called from run_response_loop)
 * ---------------------------------------------------------------------- */

static void
open_about(void)
{
    const gchar *authors[] = {
        "Alessandro Francesconi <alessandrofrancesconi@live.it>",
        NULL
    };
    const gchar *license =
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version.\n\n"
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
        "GNU General Public License for more details.";
    gtk_show_about_dialog(
        GTK_WINDOW(bimp_window_main),
        "program-name",  PLUG_IN_FULLNAME,
        "version",       PLUG_IN_VERSION,
        "comments",      _("Applies GIMP manipulations on groups of images"),
        "logo",          pixbuf_new_from_resource(
                             "/gimp/plugin/bimp/icons/bimp-icon.png"),
        "copyright",     PLUG_IN_COPYRIGHT,
        "license",       license,
        "wrap-license",  TRUE,
        "website",       PLUG_IN_WEBSITE,
        "authors",       authors,
        NULL);
}

/* -------------------------------------------------------------------------
 * Main dialog
 * ---------------------------------------------------------------------- */

static void
create_main_dialog(void)
{
    gimp_ui_init(PLUG_IN_BINARY);
    bimp_window_main = gimp_dialog_new(
        PLUG_IN_FULLNAME, PLUG_IN_BINARY, NULL, 0, NULL, NULL,
        _("About"),  GTK_RESPONSE_HELP,
        _("_Close"), GTK_RESPONSE_CLOSE,
        _("_Apply"), GTK_RESPONSE_APPLY,
        _("_Stop"),  GTK_RESPONSE_CANCEL,
        NULL);
    gimp_window_set_transient(GTK_WINDOW(bimp_window_main));
    gtk_window_set_default_size(GTK_WINDOW(bimp_window_main),
        (int)(PREVIEW_IMG_W * 2.5),
        SEQ_BUTTON_H + PREVIEW_IMG_H + 160);
}

static void
setup_main_content(void)
{
    GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    panel_sequence = bimp_sequence_panel_new();
    panel_options  = bimp_option_panel_new();
    progressbar_visible = gtk_progress_bar_new();
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar_visible), " ");
    progressbar_data = progressbar_init_hidden();
    gtk_box_append(GTK_BOX(vbox_main), panel_sequence);
    gtk_box_append(GTK_BOX(vbox_main), panel_options);
    gtk_widget_set_vexpand(panel_options, TRUE);
    gtk_box_append(GTK_BOX(vbox_main), progressbar_visible);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(bimp_window_main));
    gtk_box_append(GTK_BOX(content), vbox_main);
    gtk_widget_set_visible(bimp_window_main, TRUE);
    gtk_widget_set_visible(button_preview, FALSE);
    bimp_set_busy(FALSE);
}

static void
run_response_loop(void)
{
    while (TRUE) {
        gint run = gimp_dialog_run(GIMP_DIALOG(bimp_window_main));
        if (run == GTK_RESPONSE_APPLY) {
            if (g_slist_length(bimp_selected_manipulations) == 0) {
                bimp_show_error_dialog(
                    _("The manipulations set is empty!"), bimp_window_main);
            } else if (g_slist_length(bimp_input_filenames) == 0) {
                bimp_show_error_dialog(
                    _("The file list is empty!"), bimp_window_main);
            } else {
                bimp_opt_alertoverwrite = BIMP_ASK_OVERWRITE;
                bimp_opt_keepfolderhierarchy =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_keepfolderhierarchy));
                bimp_opt_deleteondone =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_deleteondone));
                bimp_opt_keepdates =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_keepdates));
                bimp_start_batch(bimp_window_main);
            }
        } else if (run == GTK_RESPONSE_HELP) {
            open_about();
        } else if (run == GTK_RESPONSE_CANCEL) {
            bimp_set_busy(FALSE);
        } else {
            gimp_progress_uninstall(progressbar_data);
            gtk_window_destroy(GTK_WINDOW(bimp_window_main));
            return;
        }
    }
}

void
bimp_show_gui(void)
{
    create_main_dialog();
    setup_main_content();
    run_response_loop();
}

/* -------------------------------------------------------------------------
 * Error dialog
 * ---------------------------------------------------------------------- */

void
bimp_show_error_dialog(char *message, GtkWidget *parent)
{
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_window_destroy(GTK_WINDOW(dialog));
}

/* -------------------------------------------------------------------------
 * Busy state
 * ---------------------------------------------------------------------- */

void
bimp_set_busy(gboolean busy)
{
    GtkWidget *apply_btn, *stop_btn;
    bimp_is_busy = busy;
    gtk_dialog_set_response_sensitive(GTK_DIALOG(bimp_window_main),
                                      GTK_RESPONSE_CLOSE, !busy);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(bimp_window_main),
                                      GTK_RESPONSE_HELP, !busy);
    apply_btn = gtk_dialog_get_widget_for_response(
        GTK_DIALOG(bimp_window_main), GTK_RESPONSE_APPLY);
    if (apply_btn) gtk_widget_set_visible(apply_btn, !busy);
    stop_btn = gtk_dialog_get_widget_for_response(
        GTK_DIALOG(bimp_window_main), GTK_RESPONSE_CANCEL);
    if (stop_btn) gtk_widget_set_visible(stop_btn, busy);
    gtk_widget_set_sensitive(panel_sequence, !busy);
    gtk_widget_set_sensitive(panel_options,  !busy);
}
