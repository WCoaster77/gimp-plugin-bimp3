#include <gtk/gtk.h>
#include <glib.h>
#include "bimp-gui-priv.h"
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-manipulations-gui.h"
#include "bimp-serialize.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

GtkWidget    *hbox_sequence   = NULL;
GtkWidget    *scroll_sequence = NULL;
manipulation  clicked_man     = NULL;

void add_manipulation_from_id(GtkWidget *, gpointer);
void edit_clicked_manipulation(GtkWidget *, gpointer);
void remove_clicked_manipulation(GtkWidget *, gpointer);
void save_set(GtkWidget *, gpointer);
void load_set(GtkWidget *, gpointer);

GtkWidget *
bimp_sequence_panel_new(void)
{
    GtkWidget *panel = gtk_frame_new(_("Manipulation set"));
    scroll_sequence  = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_sequence),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    hbox_sequence = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_sequence),
                                   hbox_sequence);
    gtk_frame_set_child(GTK_FRAME(panel), scroll_sequence);
    bimp_refresh_sequence_panel();
    return panel;
}

static void
add_manipulation_button(manipulation man)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *vbox   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *icon   = image_new_from_resource(man->icon);
    GtkWidget *label  = gtk_label_new(bimp_manip_get_string(man->type));
    gtk_box_append(GTK_BOX(vbox), icon);
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_button_set_child(GTK_BUTTON(button), vbox);
    gtk_widget_set_size_request(button, SEQ_BUTTON_W, SEQ_BUTTON_H);
    gtk_box_append(GTK_BOX(hbox_sequence), button);
    g_signal_connect(button, "clicked",
                     G_CALLBACK(open_manipulation_popover), man);
}

void
bimp_refresh_sequence_panel(void)
{
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(hbox_sequence)))
        gtk_widget_unparent(child);
    g_slist_foreach(bimp_selected_manipulations,
                    (GFunc)add_manipulation_button, NULL);
    GtkWidget *add_btn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(add_btn, SEQ_BUTTON_W - 20, SEQ_BUTTON_H);
    gtk_box_append(GTK_BOX(hbox_sequence), add_btn);
    g_signal_connect(add_btn, "clicked",
                     G_CALLBACK(open_manipulation_popover), NULL);
}

static GtkWidget *
build_add_popover(GtkWidget *parent)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    for (int id = 0; id < MANIP_END; id++) {
        GtkWidget *btn = popover_button(bimp_manip_get_string(id),
                                        G_CALLBACK(add_manipulation_from_id),
                                        GINT_TO_POINTER(id));
        g_signal_connect(btn, "clicked",
                         G_CALLBACK(close_popover_ancestor), NULL);
        gtk_box_append(GTK_BOX(box), btn);
    }
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *save_btn = popover_button(_("Save this set..."),
                                         G_CALLBACK(save_set), NULL);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), save_btn);
    GtkWidget *load_btn = popover_button(_("Load set..."),
                                         G_CALLBACK(load_set), NULL);
    g_signal_connect(load_btn, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), load_btn);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static GtkWidget *
build_edit_popover(GtkWidget *parent, manipulation man)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    clicked_man = man;
    GtkWidget *edit_btn = popover_button(_("Edit properties..."),
                                         G_CALLBACK(edit_clicked_manipulation), NULL);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), edit_btn);
    GtkWidget *rem_btn = popover_button(_("Remove this manipulation"),
                                        G_CALLBACK(remove_clicked_manipulation), NULL);
    g_signal_connect(rem_btn, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), rem_btn);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

void
open_manipulation_popover(GtkWidget *widget, gpointer data)
{
    GtkWidget *pop = (data == NULL)
        ? build_add_popover(widget)
        : build_edit_popover(widget, (manipulation)data);
    popup_and_track(pop, widget);
}

void
add_manipulation_from_id(GtkWidget *widget, gpointer id)
{
    manipulation newman =
        bimp_append_manipulation((manipulation_type)GPOINTER_TO_INT(id));
    if (!newman) {
        bimp_show_error_dialog(
            _("Can't add another manipulation of this kind. Only one is permitted!"),
            bimp_window_main);
    } else {
        bimp_refresh_sequence_panel();
        GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(
            GTK_SCROLLED_WINDOW(scroll_sequence));
        gtk_adjustment_set_value(hadj, gtk_adjustment_get_upper(hadj));
        bimp_open_editwindow(newman, TRUE);
    }
}

void
edit_clicked_manipulation(GtkWidget *widget, gpointer data)
{
    if (clicked_man) bimp_open_editwindow(clicked_man, FALSE);
}

void
remove_clicked_manipulation(GtkWidget *widget, gpointer data)
{
    if (clicked_man) {
        bimp_remove_manipulation(clicked_man);
        g_free(clicked_man);
        clicked_man = NULL;
        bimp_refresh_sequence_panel();
    }
}

void
save_set(GtkWidget *widget, gpointer data)
{
    if (g_slist_length(bimp_selected_manipulations) == 0) {
        bimp_show_error_dialog(_("The manipulations set is empty!"), bimp_window_main);
        return;
    }
    GtkWidget *saver = gtk_file_chooser_dialog_new(
        _("Save this set..."), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Save"),   GTK_RESPONSE_ACCEPT,
        NULL);
    GtkFileFilter *f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, "BIMP set (*.bimp)");
    gtk_file_filter_add_pattern(f, "*.bimp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(saver), f);
    if (gtk_dialog_run(GTK_DIALOG(saver)) == GTK_RESPONSE_ACCEPT) {
        GFile *gf   = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(saver));
        gchar *path = g_file_get_path(gf);
        g_object_unref(gf);
        gtk_window_destroy(GTK_WINDOW(saver));
        if (path) {
            gchar *out = g_str_has_suffix(path, ".bimp")
                ? g_strdup(path)
                : g_strconcat(path, ".bimp", NULL);
            g_free(path);
            if (!bimp_serialize_to_file(out))
                bimp_show_error_dialog(
                    _("An error occurred when saving the batch file."),
                    bimp_window_main);
            g_free(out);
        }
        return;
    }
    gtk_window_destroy(GTK_WINDOW(saver));
}

void
load_set(GtkWidget *widget, gpointer data)
{
    if (g_slist_length(bimp_selected_manipulations) > 0) {
        GtkWidget *q = gtk_message_dialog_new(
            GTK_WINDOW(bimp_window_main),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            _("This will overwrite the current manipulations set. Continue?"));
        gtk_window_set_title(GTK_WINDOW(q), _("Continue?"));
        gint r = gtk_dialog_run(GTK_DIALOG(q));
        gtk_window_destroy(GTK_WINDOW(q));
        if (r != GTK_RESPONSE_YES) return;
    }
    GtkWidget *loader = gtk_file_chooser_dialog_new(
        _("Load set..."), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Open"),   GTK_RESPONSE_ACCEPT,
        NULL);
    GtkFileFilter *f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, "BIMP set (*.bimp)");
    gtk_file_filter_add_pattern(f, "*.bimp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(loader), f);
    if (gtk_dialog_run(GTK_DIALOG(loader)) == GTK_RESPONSE_ACCEPT) {
        GFile *gf   = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(loader));
        gchar *path = g_file_get_path(gf);
        g_object_unref(gf);
        gtk_window_destroy(GTK_WINDOW(loader));
        if (path) {
            if (!bimp_deserialize_from_file(path))
                bimp_show_error_dialog(
                    _("An error occurred when loading the batch file."),
                    bimp_window_main);
            else
                bimp_refresh_sequence_panel();
            g_free(path);
        }
        return;
    }
    gtk_window_destroy(GTK_WINDOW(loader));
}
