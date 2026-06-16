#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include "bimp-gui-priv.h"
#include "bimp.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

static void
add_input_file(char *filename)
{
    if (!g_slist_find_custom(bimp_input_filenames, filename,
                             (GCompareFunc)strcmp)) {
        bimp_input_filenames =
            g_slist_append(bimp_input_filenames, g_strdup(filename));
        bimp_refresh_fileview();
    }
}

static gboolean
has_image_ext(const char *entry)
{
    static const char *exts[] = {
        ".bmp", ".jpeg", ".jpg", ".jpe", ".jp2",
        ".gif", ".heif", ".heic", ".png",
        ".tif", ".tiff", ".tga", ".svg", ".webp",
        ".avif", ".xpm", ".exr", ".dds", ".xcf", NULL
    };
    const char *dot = strrchr(entry, '.');
    if (!dot) return FALSE;
    for (int i = 0; exts[i]; i++)
        if (g_ascii_strcasecmp(dot, exts[i]) == 0) return TRUE;
    return FALSE;
}

static void
add_input_folder_r(char *folder, gboolean with_subdirs)
{
    GDir        *dp    = g_dir_open(folder, 0, NULL);
    const gchar *entry;

    if (!dp) {
        bimp_show_error_dialog(
            g_strdup_printf(_("Couldn't read into \"%s\" directory."), folder),
            bimp_window_main);
        return;
    }
    while ((entry = g_dir_read_name(dp))) {
        char *filepath = g_build_filename(folder, entry, NULL);
        if (g_file_test(filepath, G_FILE_TEST_IS_DIR)) {
            if (with_subdirs &&
                g_strcmp0(entry, ".") != 0 &&
                g_strcmp0(entry, "..") != 0)
                add_input_folder_r(filepath, with_subdirs);
        } else if (has_image_ext(entry) &&
                   !g_slist_find_custom(bimp_input_filenames, filepath,
                                        (GCompareFunc)strcmp)) {
            bimp_input_filenames =
                g_slist_append(bimp_input_filenames, g_strdup(filepath));
        }
        g_free(filepath);
    }
    g_dir_close(dp);
}

static void
add_input_folder(char *folder, gpointer with_subdirs)
{
    add_input_folder_r(folder, (gboolean)GPOINTER_TO_INT(with_subdirs));
    bimp_refresh_fileview();
}

void
add_opened_files(GtkWidget *widget, gpointer data)
{
    GList   *images  = gimp_image_list();
    gboolean missing = FALSE;

    for (GList *node = images; node; node = node->next) {
        GimpImage *img  = GIMP_IMAGE(node->data);
        GFile     *file = gimp_image_get_file(img);
        if (file) {
            gchar *path = g_file_get_path(file);
            if (path) add_input_file(path);
            g_object_unref(file);
        } else {
            missing = TRUE;
        }
    }
    g_list_free(images);
    if (missing)
        bimp_show_error_dialog(
            _("Some images were not imported because they have not been saved yet."),
            bimp_window_main);
}

static GtkFileFilter *
build_image_filter(void)
{
    static const char *pats[] = {
        "*.bmp", "*.jpeg", "*.jpg", "*.jpe", "*.jp2",
        "*.gif", "*.heif", "*.heic", "*.png",
        "*.tif", "*.tiff", "*.tga", "*.svg", "*.webp",
        "*.avif", "*.xpm", "*.exr", "*.dds", "*.xcf", NULL
    };
    GtkFileFilter *f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, _("All supported types"));
    for (int i = 0; pats[i]; i++) {
        gtk_file_filter_add_pattern(f, pats[i]);
        gchar *upper = g_ascii_strup(pats[i], -1);
        gtk_file_filter_add_pattern(f, upper);
        g_free(upper);
    }
    return f;
}

void
open_file_chooser(GtkWidget *widget, gpointer data)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select images"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Add"),    GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);
    if (last_input_location) {
        GFile *dir = g_file_new_for_path(last_input_location);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir, NULL);
        g_object_unref(dir);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), build_image_filter());

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GFile *cur = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
        g_free(last_input_location);
        last_input_location = cur ? g_file_get_path(cur) : NULL;
        if (cur) g_object_unref(cur);
        GListModel *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(chooser));
        guint n = g_list_model_get_n_items(files);
        for (guint i = 0; i < n; i++) {
            GFile *f = G_FILE(g_list_model_get_item(files, i));
            gchar *p = g_file_get_path(f);
            if (p) add_input_file(p);
            g_object_unref(f);
        }
        g_object_unref(files);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

void
open_folder_chooser(GtkWidget *widget, gpointer data)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select folders containing images"), NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Add"),    GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);
    GtkWidget *subdirs = gtk_check_button_new_with_label(
        _("Add files from the whole hierarchy"));
    gtk_widget_set_visible(subdirs, TRUE);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser), subdirs);
    if (last_input_location) {
        GFile *dir = g_file_new_for_path(last_input_location);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir, NULL);
        g_object_unref(dir);
    }
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gboolean recurse =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(subdirs));
        GFile *cur = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
        g_free(last_input_location);
        last_input_location = cur ? g_file_get_path(cur) : NULL;
        if (cur) g_object_unref(cur);
        GListModel *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(chooser));
        guint n = g_list_model_get_n_items(files);
        for (guint i = 0; i < n; i++) {
            GFile *f = G_FILE(g_list_model_get_item(files, i));
            gchar *p = g_file_get_path(f);
            if (p) add_input_folder(p, GINT_TO_POINTER(recurse));
            g_object_unref(f);
        }
        g_object_unref(files);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

void
open_outputfolder_chooser(GtkWidget *widget, gpointer data)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select output folder"), NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_OK"),     GTK_RESPONSE_ACCEPT,
        NULL);
    if (selected_source_folder) {
        GFile *dir = g_file_new_for_path(selected_source_folder);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir, NULL);
        g_object_unref(dir);
    }
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GFile *f    = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
        gchar *path = g_file_get_path(f);
        if (path) {
            g_free(bimp_output_folder);
            bimp_output_folder = path;
            gtk_button_set_label(GTK_BUTTON(button_outfolder),
                                 get_outputfolder_name());
            gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);
        }
        g_object_unref(f);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

void
set_source_output_folder(GtkWidget *widget, gpointer data)
{
    if (selected_source_folder) {
        g_free(bimp_output_folder);
        bimp_output_folder = g_strdup(selected_source_folder);
        gtk_button_set_label(GTK_BUTTON(button_outfolder), get_outputfolder_name());
        gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);
    }
}
