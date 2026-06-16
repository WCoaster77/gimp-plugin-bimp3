#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "bimp-operate-priv.h"
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

/* Shared state — extern-visible via bimp-operate-priv.h */
char     *current_datetime;
int       processed_count;
int       success_count;
int       total_images;
char     *common_folder_path;
gboolean  list_contains_changeformat;
gboolean  list_contains_rename;
gboolean  list_contains_watermark;
gboolean  list_contains_savingplugin;

static gboolean apply_manipulation(manipulation, image_output);

GeglColor *
gegl_color_from_gdkrgba(const GdkRGBA *rgba)
{
    GeglColor *color = gegl_color_new(NULL);
    gegl_color_set_rgba(color, rgba->red, rgba->green, rgba->blue, rgba->alpha);
    return color;
}

GimpLayer *
merge_visible(image_output out)
{
    return gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
}

GimpLayer *
first_layer(image_output out)
{
    if (!out->layers) return NULL;
    return GIMP_LAYER(out->layers->data);
}

static void
find_common_folder(void)
{
    gboolean  need_hierarchy = FALSE;
    char    **common_folder;
    size_t    common_folder_size;

    char *path     = comp_get_filefolder(g_slist_nth(bimp_input_filenames, 0)->data);
    common_folder  = get_path_folders(path);
    for (common_folder_size = 0; common_folder[common_folder_size]; ++common_folder_size);

    for (int i = 1; i < total_images; i++) {
        path = comp_get_filefolder(g_slist_nth(bimp_input_filenames, i)->data);
        char **cur = get_path_folders(path);
        size_t cur_size = 0;
        for (cur_size = 0; cur[cur_size]; ++cur_size);

        while (common_folder_size > cur_size) {
            need_hierarchy = TRUE;
            g_free(common_folder[common_folder_size - 1]);
            common_folder[--common_folder_size] = NULL;
        }
        for (int j = 0; j < (int)common_folder_size; j++) {
            if (strcmp(common_folder[j], cur[j]) != 0) {
                need_hierarchy = TRUE;
                while (common_folder_size > (size_t)j) {
                    g_free(common_folder[common_folder_size - 1]);
                    common_folder[--common_folder_size] = NULL;
                }
                break;
            }
        }
        g_strfreev(cur);
    }

    if (need_hierarchy)
        common_folder_path = g_strjoinv(FILE_SEPARATOR_STR, common_folder);
    g_strfreev(common_folder);
}

void
bimp_start_batch(gpointer parent_dialog)
{
    bimp_set_busy(TRUE);
    g_print("\nBIMP - Batch Manipulation Plugin\nStart batch processing...\n");
    processed_count    = 0;
    success_count      = 0;
    total_images       = g_slist_length(bimp_input_filenames);
    bimp_progress_bar_set(0.0, "");
    bimp_init_batch();
    current_datetime   = get_datetime();
    common_folder_path = NULL;
    if (bimp_opt_keepfolderhierarchy) find_common_folder();
    g_idle_add((GSourceFunc)bimp_process_image, parent_dialog);
}

void
bimp_init_batch(void)
{
    list_contains_changeformat = bimp_list_contains_manip(MANIP_CHANGEFORMAT);
    list_contains_rename       = bimp_list_contains_manip(MANIP_RENAME);
    list_contains_watermark    = bimp_list_contains_manip(MANIP_WATERMARK);
    list_contains_savingplugin = bimp_list_contains_savingplugin();
    bimp_apply_color_reset();
}

void
bimp_apply_drawable_manipulations(image_output imageout,
                                   gchar       *orig_filename,
                                   gchar       *orig_basename)
{
    GFile *file = g_file_new_for_path(orig_filename);
    imageout->image = gimp_file_load(GIMP_RUN_NONINTERACTIVE, file);
    g_object_unref(file);
    g_print("Image loaded: %s\n", orig_basename);
    gimp_image_undo_freeze(imageout->image);
    g_list_free(imageout->layers);
    imageout->layers = gimp_image_get_layers(imageout->image);
    g_print("Total layers: %d\n", g_list_length(imageout->layers));
    g_slist_foreach(bimp_selected_manipulations, (GFunc)apply_manipulation, imageout);
    if (list_contains_watermark) {
        GSList *watermarks = bimp_list_get_manip_all(MANIP_WATERMARK);
        for (GSList *it = watermarks; it; it = it->next) {
            g_print("Applying WATERMARK...\n");
            apply_watermark(
                (watermark_settings)(((manipulation)(it->data))->settings),
                imageout);
        }
    }
    gimp_image_undo_thaw(imageout->image);
}

static gboolean
apply_manipulation(manipulation man, image_output out)
{
    gboolean success = TRUE;
    if (man->type == MANIP_RESIZE) {
        g_print("Applying RESIZE...\n");
        apply_resize((resize_settings)(bimp_list_get_manip(MANIP_RESIZE))->settings, out);
    } else if (man->type == MANIP_CROP) {
        g_print("Applying CROP...\n");
        apply_crop((crop_settings)(bimp_list_get_manip(MANIP_CROP))->settings, out);
    } else if (man->type == MANIP_FLIPROTATE) {
        g_print("Applying FLIP OR ROTATE...\n");
        success = apply_fliprotate((fliprotate_settings)(man->settings), out);
    } else if (man->type == MANIP_COLOR) {
        g_print("Applying COLOR CORRECTION...\n");
        success = apply_color((color_settings)(man->settings), out);
    } else if (man->type == MANIP_SHARPBLUR) {
        g_print("Applying SHARPBLUR...\n");
        success = apply_sharpblur((sharpblur_settings)(man->settings), out);
    } else if (man->type == MANIP_USERDEF &&
               strstr(((userdef_settings)(man->settings))->procedure, "-save") == NULL) {
        g_print("Applying %s...\n", ((userdef_settings)(man->settings))->procedure);
        success = apply_userdef((userdef_settings)(man->settings), out);
    }
    return success;
}
