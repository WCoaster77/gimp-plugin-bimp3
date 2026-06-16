#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <glib/gstdio.h>
#include "bimp-operate-priv.h"
#include "bimp-operate.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

static gboolean
get_output_ext(char *orig_basename, char **ext_out)
{
    char *ext = g_strdup(strrchr(orig_basename, '.'));
    if (!ext) {
        if (list_contains_changeformat) {
            *ext_out = g_malloc0(sizeof(char));
            return TRUE;
        }
        bimp_show_error_dialog(
            g_strdup_printf(
                _("Can't save image \"%s\": input file has no extension.\n"
                  "Add a \"Change format or compression\" step to fix this."),
                orig_basename),
            bimp_window_main);
        return FALSE;
    }
    if (g_ascii_strcasecmp(ext, ".svg") == 0 && !list_contains_changeformat) {
        bimp_show_error_dialog(
            g_strdup_printf(
                _("GIMP can't save %s back to SVG.\n"
                  "Add a \"Change format or compression\" step to fix this."),
                orig_basename),
            bimp_window_main);
        g_free(ext);
        return FALSE;
    }
    *ext_out = ext;
    return TRUE;
}

static void
compute_output_dir(image_output imageout, char *orig_filename,
                   char *orig_basename, const char *orig_file_ext,
                   char **file_comp_out,
                   format_type *fmt_out, format_params *par_out)
{
    char *output_file_comp;

    if (!common_folder_path) {
        output_file_comp = g_malloc0(sizeof(char));
    } else {
        size_t skip         = strlen(common_folder_path) + 1;
        size_t base_ext_len = strlen(orig_basename) + strlen(orig_file_ext);
        output_file_comp    = g_strndup(&orig_filename[skip],
                                        strlen(orig_filename) - skip - base_ext_len);
    }

    if (strlen(output_file_comp) > 0) {
#ifdef _WIN32
        for (int i = 0; i < (int)strlen(output_file_comp); ++i)
            if (output_file_comp[i] == ':') output_file_comp[i] = '_';
#endif
        g_mkdir_with_parents(
            g_strconcat(bimp_output_folder, FILE_SEPARATOR_STR, output_file_comp, NULL),
            0777);
    }

    if (list_contains_changeformat) {
        changeformat_settings cs =
            (changeformat_settings)(bimp_list_get_manip(MANIP_CHANGEFORMAT))->settings;
        *fmt_out = cs->format;
        *par_out = cs->params;
        g_print("Changing FORMAT to %s\n", format_type_string[cs->format][0]);
        imageout->filename = g_strconcat(imageout->filename, ".",
                                          format_type_string[cs->format][0], NULL);
    } else if (list_contains_savingplugin) {
        imageout->filename = g_strconcat(imageout->filename, ".dds", NULL);
        *fmt_out = -1;
    } else {
        imageout->filename = g_strconcat(imageout->filename, orig_file_ext, NULL);
        *fmt_out = -1;
    }
    imageout->filepath = g_strconcat(bimp_output_folder, FILE_SEPARATOR_STR,
                                      output_file_comp, imageout->filename, NULL);
    *file_comp_out = output_file_comp;
}

static int
overwrite_result(char *path, GtkWidget *parent)
{
    gboolean oldfile_access = g_file_test(path, G_FILE_TEST_IS_REGULAR);

    if (bimp_opt_alertoverwrite == BIMP_ASK_OVERWRITE && oldfile_access) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
            _("File %s already exists, overwrite it?"), comp_get_filename(path));
        gtk_window_set_title(GTK_WINDOW(dialog), _("Overwrite?"));
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *check   = gtk_check_button_new_with_label(_("Always apply this decision"));
        gtk_box_append(GTK_BOX(content), check);
        gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Yes"), GTK_RESPONSE_YES);
        gtk_dialog_add_button(GTK_DIALOG(dialog), _("_No"),  GTK_RESPONSE_NO);
        gint     response = gtk_dialog_run(GTK_DIALOG(dialog));
        gboolean dont_ask = gtk_check_button_get_active(GTK_CHECK_BUTTON(check));
        gtk_window_destroy(GTK_WINDOW(dialog));
        if (response == GTK_RESPONSE_YES) {
            if (dont_ask) bimp_opt_alertoverwrite = BIMP_OVERWRITE_SKIP_ASK;
            return 1;
        } else {
            if (dont_ask) bimp_opt_alertoverwrite = BIMP_DONT_OVERWRITE_SKIP_ASK;
            return 0;
        }
    }
    if (oldfile_access)
        return (bimp_opt_alertoverwrite == BIMP_OVERWRITE_SKIP_ASK) ? 1 : 0;
    return 2;
}

static gboolean
do_apply_rename(image_output out, char *orig_basename)
{
    rename_settings settings =
        (rename_settings)(bimp_list_get_manip(MANIP_RENAME))->settings;
    char *orig_name = g_strdup(orig_basename);
    out->filename   = g_strdup(settings->pattern);

    if (strstr(out->filename, RENAME_KEY_ORIG) != NULL)
        out->filename = str_replace(out->filename, RENAME_KEY_ORIG, orig_name);
    if (strstr(out->filename, RENAME_KEY_COUNT) != NULL) {
        char strcount[5];
        sprintf(strcount, "%i", processed_count + 1);
        out->filename = str_replace(out->filename, RENAME_KEY_COUNT, strcount);
    }
    if (strstr(out->filename, RENAME_KEY_DATETIME) != NULL)
        out->filename = str_replace(out->filename, RENAME_KEY_DATETIME, current_datetime);
    g_free(orig_name);
    return TRUE;
}

gboolean
bimp_process_image(gpointer parent)
{
    gboolean      success         = TRUE;
    image_output  imageout        = g_malloc0(sizeof(struct imageout_str));
    char         *orig_filename   = g_slist_nth(bimp_input_filenames, processed_count)->data;
    char         *orig_basename   = g_strdup(comp_get_filename(orig_filename));
    char         *orig_file_ext   = NULL;
    char         *output_file_comp = NULL;
    format_type   final_format    = -1;
    format_params params          = NULL;

    if (!get_output_ext(orig_basename, &orig_file_ext)) {
        success = FALSE;
        goto process_end;
    }

    g_print("\nWorking on file %d of %d (%s)\n",
            processed_count + 1, total_images, orig_filename);
    bimp_progress_bar_set(((double)processed_count) / total_images,
        g_strdup_printf(_("Working on file \"%s\"..."), orig_basename));

    orig_basename[strlen(orig_basename) - strlen(orig_file_ext)] = '\0';

    if (list_contains_rename) do_apply_rename(imageout, orig_basename);
    else                       imageout->filename = orig_basename;

    compute_output_dir(imageout, orig_filename, orig_basename, orig_file_ext,
                       &output_file_comp, &final_format, &params);

    if (list_contains_savingplugin) {
        for (GSList *it = bimp_selected_manipulations; it; it = it->next) {
            manipulation man = (manipulation)it->data;
            if (man->type == MANIP_USERDEF &&
                strstr(((userdef_settings)man->settings)->procedure, "-save") != NULL)
                apply_userdef((userdef_settings)man->settings, imageout);
        }
    }

    gboolean will_overwrite = FALSE;
    if (bimp_opt_alertoverwrite != BIMP_OVERWRITE_SKIP_ASK) {
        will_overwrite = g_file_test(imageout->filepath, G_FILE_TEST_IS_REGULAR);
        if (will_overwrite) {
            if (bimp_opt_alertoverwrite == BIMP_DONT_OVERWRITE_SKIP_ASK) {
                g_print("Destination file already exists and won't be overwritten\n");
                goto process_end;
            }
            if (overwrite_result(imageout->filepath, parent) == 0) {
                g_print("Destination file already exists; user chose not to overwrite\n");
                goto process_end;
            }
        }
    }

    bimp_apply_drawable_manipulations(imageout, orig_filename, orig_basename);

    time_t mod_time = -1;
    if (will_overwrite && bimp_opt_keepdates)
        mod_time = get_modification_time(imageout->filepath);

    g_print("Saving file %s in %s\n", imageout->filename, imageout->filepath);
    image_save(final_format, imageout, params);

    if (will_overwrite && bimp_opt_keepdates && mod_time > -1)
        set_modification_time(imageout->filepath, mod_time);

    if (imageout->image) g_object_unref(imageout->image);

process_end:
    g_free(orig_basename);
    g_free(orig_file_ext);
    g_free(output_file_comp);
    g_free(imageout->filename);
    g_free(imageout->filepath);
    g_free(imageout);

    processed_count++;
    if (success) success_count++;

    if (!bimp_is_busy) {
        bimp_progress_bar_set(0.0, _("Operations stopped"));
        g_print("\nStopped, %d files processed.\n", processed_count);
        return FALSE;
    }
    if (processed_count == total_images) {
        int errors = processed_count - success_count;
        bimp_progress_bar_set(1.0,
            g_strdup_printf(_("End, all files have been processed with %d errors"), errors));
        g_print("\nEnd, %d files processed with %d errors.\n", processed_count, errors);
        bimp_set_busy(FALSE);
        return FALSE;
    }
    return TRUE;
}
