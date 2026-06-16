#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "gui-rename.h"
#include "../bimp-manipulations.h"
#include "../bimp-gui.h"
#include "../bimp-utils.h"
#include "../plugin-intl.h"

static void check_entrytext(GtkEditable *, gpointer);
static GtkWidget *entry_pattern, *label_preview;

GtkWidget *
bimp_rename_gui_new(rename_settings settings, GtkWidget *parent)
{
    GtkWidget *gui, *frame_help, *label_help;

    gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    entry_pattern = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_pattern), 50);
    gtk_editable_set_text(GTK_EDITABLE(entry_pattern), settings->pattern);

    frame_help = gtk_frame_new(_("Keywords"));
    label_help = gtk_label_new(g_strconcat(
        RENAME_KEY_ORIG,     " = ", _("Original filename (without extension)"), "\n",
        RENAME_KEY_COUNT,    " = ", _("Incremental number"), "\n",
        RENAME_KEY_DATETIME, " = ", _("Date and time (YYYY-MM-DD_hh-mm)"), NULL));
    gtk_frame_set_child(GTK_FRAME(frame_help), label_help);

    label_preview = gtk_label_new("");

    gtk_box_append(GTK_BOX(gui), entry_pattern);
    gtk_box_append(GTK_BOX(gui), frame_help);
    gtk_box_append(GTK_BOX(gui), label_preview);

    g_signal_connect(G_OBJECT(entry_pattern), "changed",
                     G_CALLBACK(check_entrytext), parent);
    return gui;
}

static void
check_entrytext(GtkEditable *editable, gpointer parent)
{
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry_pattern));

    if (!strstr(text, RENAME_KEY_ORIG) && !strstr(text, RENAME_KEY_COUNT)) {
        gtk_label_set_text(GTK_LABEL(label_preview),
            g_strdup_printf(_("Can't save!\n'%s' or '%s' symbol must be present."),
                            RENAME_KEY_ORIG, RENAME_KEY_COUNT));
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent),
                                          GTK_RESPONSE_ACCEPT, FALSE);
    } else if (
        strstr(text, "\\") || strstr(text, "/")  ||
        strstr(text, "*")  || strstr(text, ":")  ||
        strstr(text, "?")  || strstr(text, "|")  ||
        strstr(text, ">")  || strstr(text, "<"))
    {
        gtk_label_set_text(GTK_LABEL(label_preview),
            _("Can't save!\nPattern contains invalid characters."));
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent),
                                          GTK_RESPONSE_ACCEPT, FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(label_preview), "");
        gtk_dialog_set_response_sensitive(GTK_DIALOG(parent),
                                          GTK_RESPONSE_ACCEPT, TRUE);
    }
}

void
bimp_rename_save(rename_settings orig_settings)
{
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry_pattern));
    if (strlen(text) > 0)
        orig_settings->pattern = g_strdup(text);
}
