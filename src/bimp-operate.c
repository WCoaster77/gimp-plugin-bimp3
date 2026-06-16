/* Functions called when the user clicks on 'APPLY' */

#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimpbase/gimpbase.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <pango/pango.h>
#include "bimp-operate.h"
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp-utils.h"
#include "bimp-serialize.h"
#include "plugin-intl.h"

static gboolean process_image(gpointer);

static gboolean apply_manipulation(manipulation, image_output);
static gboolean apply_resize(resize_settings, image_output);
static gboolean apply_crop(crop_settings, image_output);
static gboolean apply_fliprotate(fliprotate_settings, image_output);
static gboolean apply_color(color_settings, image_output);
static gboolean apply_sharpblur(sharpblur_settings, image_output);
static gboolean apply_watermark(watermark_settings, image_output);
static void     calc_watermark_xy(int, int, int, int, watermark_position, int,
                                  gdouble *, gdouble *);
static gboolean apply_userdef(userdef_settings, image_output);
static gboolean apply_rename(rename_settings, image_output, char *);

static gboolean image_save(format_type, image_output, format_params);
static gboolean image_save_bmp(image_output);
static gboolean image_save_gif(image_output, gboolean);
static gboolean image_save_icon(image_output);
static gboolean image_save_jpeg(image_output, float, float, gboolean, gboolean,
                                gchar *, int, gboolean, int, int);
static gboolean image_save_heif(image_output, int, gboolean);
static gboolean image_save_png(image_output, gboolean, int, gboolean, gboolean,
                               gboolean, gboolean, gboolean, gboolean, gboolean);
static gboolean image_save_tga(image_output, gboolean, int);
static gboolean image_save_tiff(image_output, int);
static gboolean image_save_webp(image_output, int, gboolean, float, float,
                                gboolean, gboolean, gboolean, int, gboolean,
                                gboolean, gboolean, int, int);
static gboolean image_save_avif(image_output, gboolean, int);
static gboolean image_save_exr(image_output);

static int overwrite_result(char *, GtkWidget *);

static char *current_datetime;
static int   processed_count;
static int   success_count;
static int   total_images;

static char *common_folder_path;

static gboolean list_contains_changeformat;
static gboolean list_contains_rename;
static gboolean list_contains_watermark;
static gboolean list_contains_savingplugin;

static gboolean  colorcurve_init;
static int       colorcurve_num_points_v;
static gdouble  *colorcurve_ctr_points_v;
static int       colorcurve_num_points_r;
static gdouble  *colorcurve_ctr_points_r;
static int       colorcurve_num_points_g;
static gdouble  *colorcurve_ctr_points_g;
static int       colorcurve_num_points_b;
static gdouble  *colorcurve_ctr_points_b;
static int       colorcurve_num_points_a;
static gdouble  *colorcurve_ctr_points_a;

/* Helper: build a GeglColor from a GdkRGBA */
static GeglColor *
gegl_color_from_gdkrgba(const GdkRGBA *rgba)
{
    GeglColor *color = gegl_color_new(NULL);
    gegl_color_set_rgba(color, rgba->red, rgba->green, rgba->blue, rgba->alpha);
    return color;
}

/* Helper: merge visible layers and return the result layer */
static GimpLayer *
merge_visible(image_output out)
{
    return gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
}

/* Helper: get the first layer from out->layers */
static GimpLayer *
first_layer(image_output out)
{
    if (!out->layers) return NULL;
    return GIMP_LAYER(out->layers->data);
}

/* -------------------------------------------------------------------------
 * Batch entry point
 * ---------------------------------------------------------------------- */

void
bimp_start_batch(gpointer parent_dialog)
{
    bimp_set_busy(TRUE);

    g_print("\nBIMP - Batch Manipulation Plugin\nStart batch processing...\n");
    processed_count = 0;
    success_count   = 0;
    total_images    = g_slist_length(bimp_input_filenames);
    bimp_progress_bar_set(0.0, "");

    bimp_init_batch();

    current_datetime   = get_datetime();
    common_folder_path = NULL;

    if (bimp_opt_keepfolderhierarchy) {
        int      i, j;
        gboolean need_hierarchy = FALSE;
        char    *path           = NULL;
        char   **common_folder;
        char   **current_folder;
        size_t   common_folder_size, current_folder_size;

        path = comp_get_filefolder(g_slist_nth(bimp_input_filenames, 0)->data);

        common_folder      = get_path_folders(path);
        common_folder_size = 0;
        for (common_folder_size = 0;
             common_folder[common_folder_size] != NULL;
             ++common_folder_size);

        for (i = 1; i < total_images; i++) {
            path = comp_get_filefolder(g_slist_nth(bimp_input_filenames, i)->data);
            current_folder = get_path_folders(path);
            for (current_folder_size = 0;
                 current_folder[current_folder_size] != NULL;
                 ++current_folder_size);

            while (common_folder_size > current_folder_size) {
                need_hierarchy = TRUE;
                g_free(common_folder[common_folder_size - 1]);
                common_folder[common_folder_size - 1] = NULL;
                common_folder_size--;
            }

            for (j = 0; j < (int)common_folder_size; j++) {
                if (strcmp(common_folder[j], current_folder[j]) != 0) {
                    need_hierarchy = TRUE;
                    while (common_folder_size > (size_t)j) {
                        g_free(common_folder[common_folder_size - 1]);
                        common_folder[common_folder_size - 1] = NULL;
                        common_folder_size--;
                    }
                    break;
                }
            }

            g_strfreev(current_folder);
        }

        if (need_hierarchy)
            common_folder_path = g_strjoinv(FILE_SEPARATOR_STR, common_folder);

        g_strfreev(common_folder);
    }

    g_idle_add((GSourceFunc)process_image, parent_dialog);
}

void
bimp_init_batch(void)
{
    list_contains_changeformat  = bimp_list_contains_manip(MANIP_CHANGEFORMAT);
    list_contains_rename        = bimp_list_contains_manip(MANIP_RENAME);
    list_contains_watermark     = bimp_list_contains_manip(MANIP_WATERMARK);
    list_contains_savingplugin  = bimp_list_contains_savingplugin();
    colorcurve_init             = FALSE;
}

static gboolean
process_image(gpointer parent)
{
    gboolean success = TRUE;

    image_output imageout = (image_output)g_malloc(sizeof(struct imageout_str));
    char *orig_filename   = NULL;
    char *orig_basename   = NULL;
    char *orig_file_ext   = NULL;
    char *output_file_comp = NULL;

    orig_filename = g_slist_nth(bimp_input_filenames, processed_count)->data;
    orig_basename = g_strdup(comp_get_filename(orig_filename));

    orig_file_ext = g_strdup(strrchr(orig_basename, '.'));
    if (orig_file_ext == NULL) {
        if (list_contains_changeformat) {
            orig_file_ext = g_malloc0(sizeof(char));
        } else {
            bimp_show_error_dialog(
                g_strdup_printf(_("Can't save image \"%s\": input file has no extension.\n"
                                  "Add a \"Change format or compression\" step to fix this."),
                                orig_basename),
                bimp_window_main
            );
            success = FALSE;
            goto process_end;
        }
    } else if (g_ascii_strcasecmp(orig_file_ext, ".svg") == 0 &&
               !list_contains_changeformat) {
        bimp_show_error_dialog(
            g_strdup_printf(_("GIMP can't save %s back to SVG.\n"
                              "Add a \"Change format or compression\" step to fix this."),
                            orig_basename),
            bimp_window_main
        );
        success = FALSE;
        goto process_end;
    }

    g_print("\nWorking on file %d of %d (%s)\n",
            processed_count + 1, total_images, orig_filename);
    bimp_progress_bar_set(
        ((double)processed_count) / total_images,
        g_strdup_printf(_("Working on file \"%s\"..."), orig_basename)
    );

    orig_basename[strlen(orig_basename) - strlen(orig_file_ext)] = '\0';

    if (list_contains_rename) {
        g_print("Applying RENAME...\n");
        apply_rename((rename_settings)(bimp_list_get_manip(MANIP_RENAME))->settings,
                     imageout, orig_basename);
    } else {
        imageout->filename = orig_basename;
    }

    if (common_folder_path == NULL) {
        output_file_comp = g_malloc0(sizeof(char));
    } else {
        output_file_comp = g_strndup(
            &orig_filename[strlen(common_folder_path) + 1],
            strlen(orig_filename) - (strlen(common_folder_path) + 1)
                - strlen(orig_basename) - strlen(orig_file_ext)
        );
    }

    if (strlen(output_file_comp) > 0) {
#ifdef _WIN32
        for (int i = 0; i < (int)strlen(output_file_comp); ++i)
            if (output_file_comp[i] == ':')
                output_file_comp[i] = '_';
#endif
        g_mkdir_with_parents(
            g_strconcat(bimp_output_folder, FILE_SEPARATOR_STR,
                        output_file_comp, NULL),
            0777
        );
    }

    format_type   final_format = -1;
    format_params params       = NULL;

    if (list_contains_changeformat) {
        changeformat_settings settings =
            (changeformat_settings)(bimp_list_get_manip(MANIP_CHANGEFORMAT))->settings;
        final_format = settings->format;
        params       = settings->params;

        g_print("Changing FORMAT to %s\n", format_type_string[final_format][0]);
        imageout->filename = g_strconcat(
            imageout->filename, ".", format_type_string[final_format][0], NULL);
        imageout->filepath = g_strconcat(
            bimp_output_folder, FILE_SEPARATOR_STR,
            output_file_comp, imageout->filename, NULL);
    } else if (list_contains_savingplugin) {
        imageout->filename = g_strconcat(imageout->filename, ".dds", NULL);
        imageout->filepath = g_strconcat(
            bimp_output_folder, FILE_SEPARATOR_STR,
            output_file_comp, imageout->filename, NULL);

        GSList *iterator = NULL;
        for (iterator = bimp_selected_manipulations; iterator;
             iterator = iterator->next) {
            manipulation man = (manipulation)(iterator->data);
            if (man->type == MANIP_USERDEF &&
                strstr(((userdef_settings)(man->settings))->procedure, "-save") != NULL) {
                apply_userdef((userdef_settings)(man->settings), imageout);
            }
        }
    } else {
        imageout->filename = g_strconcat(imageout->filename, orig_file_ext, NULL);
        imageout->filepath = g_strconcat(
            bimp_output_folder, FILE_SEPARATOR_STR,
            output_file_comp, imageout->filename, NULL);
        final_format = -1;
    }

    gboolean will_overwrite = FALSE;
    if (bimp_opt_alertoverwrite != BIMP_OVERWRITE_SKIP_ASK) {
        will_overwrite = g_file_test(imageout->filepath, G_FILE_TEST_IS_REGULAR);
        if (will_overwrite) {
            if (bimp_opt_alertoverwrite == BIMP_DONT_OVERWRITE_SKIP_ASK) {
                g_print("Destination file already exists and won't be overwritten\n");
                goto process_end;
            } else {
                int ow_res = overwrite_result(imageout->filepath, parent);
                if (ow_res == 0) {
                    g_print("Destination file already exists; user chose not to overwrite\n");
                    goto process_end;
                }
            }
        }
    }

    bimp_apply_drawable_manipulations(imageout, (gchar *)orig_filename,
                                      (gchar *)orig_basename);

    time_t mod_time = -1;
    if (will_overwrite && bimp_opt_keepdates) {
        mod_time = get_modification_time(imageout->filepath);
        if (mod_time == -1)
            g_print("Error retrieving modification date of file.\n");
    }

    g_print("Saving file %s in %s\n", imageout->filename, imageout->filepath);
    image_save(final_format, imageout, params);

    if (will_overwrite && bimp_opt_keepdates && mod_time > -1) {
        if (set_modification_time(imageout->filepath, mod_time) == -1)
            g_print("Error replacing modification date of file.\n");
    }

    g_object_unref(imageout->image);

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
        int errors_count = processed_count - success_count;
        bimp_progress_bar_set(
            1.0,
            g_strdup_printf(_("End, all files have been processed with %d errors"),
                            errors_count)
        );
        g_print("\nEnd, %d files processed with %d errors.\n",
                processed_count, errors_count);
        bimp_set_busy(FALSE);
        return FALSE;
    }

    return TRUE;
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

    g_slist_foreach(bimp_selected_manipulations,
                    (GFunc)apply_manipulation, imageout);

    if (list_contains_watermark) {
        GSList *watermarks = bimp_list_get_manip_all(MANIP_WATERMARK);
        GSList *iterator   = NULL;
        for (iterator = watermarks; iterator; iterator = iterator->next) {
            g_print("Applying WATERMARK...\n");
            apply_watermark(
                (watermark_settings)(((manipulation)(iterator->data))->settings),
                imageout
            );
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
        apply_resize(
            (resize_settings)(bimp_list_get_manip(MANIP_RESIZE))->settings, out);
    } else if (man->type == MANIP_CROP) {
        g_print("Applying CROP...\n");
        apply_crop(
            (crop_settings)(bimp_list_get_manip(MANIP_CROP))->settings, out);
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
        g_print("Applying %s...\n",
                ((userdef_settings)(man->settings))->procedure);
        success = apply_userdef((userdef_settings)(man->settings), out);
    }

    return success;
}

/* -------------------------------------------------------------------------
 * Individual manipulation implementations
 * ---------------------------------------------------------------------- */

static gboolean
apply_resize(resize_settings settings, image_output out)
{
    gboolean success = FALSE;
    gint     orig_w, orig_h, final_w, final_h, view_w, view_h;
    gdouble  orig_res_x, orig_res_y;

    if (settings->change_res) {
        success = gimp_image_get_resolution(out->image, &orig_res_x, &orig_res_y);

        if (settings->new_res_x != orig_res_x || settings->new_res_y != orig_res_y) {
            success = gimp_image_set_resolution(
                out->image, settings->new_res_x, settings->new_res_y);
        }
    }

    orig_w = gimp_image_get_width(out->image);
    orig_h = gimp_image_get_height(out->image);

    if (settings->resize_mode_width == RESIZE_DISABLE &&
        settings->resize_mode_height == RESIZE_DISABLE) {
        return !settings->change_res || success;
    }

    gdouble newwpct, newwpctmax;
    if (settings->resize_mode_width == RESIZE_PERCENT) {
        newwpct = newwpctmax = settings->new_w_pc / 100.0;
    } else if (settings->resize_mode_width == RESIZE_PIXEL) {
        newwpct = newwpctmax = (double)settings->new_w_px / (double)orig_w;
    } else {
        newwpct = 1;
        newwpctmax = DBL_MAX;
    }

    gdouble newhpct, newhpctmax;
    if (settings->resize_mode_height == RESIZE_PERCENT) {
        newhpct = newhpctmax = settings->new_h_pc / 100.0;
    } else if (settings->resize_mode_height == RESIZE_PIXEL) {
        newhpct = newhpctmax = (double)settings->new_h_px / (double)orig_h;
    } else {
        newhpct = 1;
        newhpctmax = DBL_MAX;
    }

    if (settings->stretch_mode == STRETCH_ASPECT) {
        gdouble newpct = min(newwpctmax, newhpctmax);
        final_w = view_w = (gint)round(orig_w * newpct);
        final_h = view_h = (gint)round(orig_h * newpct);
    } else if (settings->stretch_mode == STRETCH_PADDED) {
        gdouble newpct = min(newwpctmax, newhpctmax);
        final_w = (gint)round(orig_w * newpct);
        final_h = (gint)round(orig_h * newpct);
        view_w  = (gint)round(orig_w * newwpct);
        view_h  = (gint)round(orig_h * newhpct);
    } else {
        final_w = view_w = (gint)round(orig_w * newwpct);
        final_h = view_h = (gint)round(orig_h * newhpct);
    }

    GimpInterpolationType old_interpolation =
        gimp_context_get_interpolation();
    gimp_context_set_interpolation(settings->interpolation);
    success = gimp_image_scale(out->image, final_w, final_h);
    gimp_context_set_interpolation(old_interpolation);

    if (settings->stretch_mode == STRETCH_PADDED) {
        GimpImageBaseType image_type = gimp_image_get_base_type(out->image);
        GimpImageType     layer_type;

        switch (image_type) {
            case GIMP_INDEXED: layer_type = GIMP_INDEXEDA_IMAGE; break;
            case GIMP_GRAY:    layer_type = GIMP_GRAYA_IMAGE;    break;
            default:           layer_type = GIMP_RGBA_IMAGE;     break;
        }
        if (!gimp_drawable_has_alpha(GIMP_DRAWABLE(first_layer(out))))
            layer_type--;

        GimpLayer *padding = gimp_layer_new(
            out->image, "padding_layer",
            view_w, view_h, layer_type,
            (settings->padding_color.alpha) * 100.0,
            GIMP_LAYER_MODE_NORMAL_LEGACY
        );

        gimp_image_insert_layer(out->image, padding, NULL, 0);
        gimp_image_lower_item_to_bottom(out->image, GIMP_ITEM(padding));

        GeglColor *old_bg = gimp_context_get_background();
        GeglColor *new_bg = gegl_color_from_gdkrgba(&settings->padding_color);
        gimp_context_set_background(new_bg);
        gimp_drawable_fill(GIMP_DRAWABLE(padding), GIMP_FILL_BACKGROUND);
        gimp_context_set_background(old_bg);
        g_object_unref(new_bg);
        g_object_unref(old_bg);

        gimp_item_transform_translate(
            GIMP_ITEM(padding),
            -(gdouble)abs(view_w - final_w) / 2.0,
            -(gdouble)abs(view_h - final_h) / 2.0
        );

        success = gimp_image_resize_to_layers(out->image);
    }

    return success;
}

static gboolean
apply_crop(crop_settings settings, image_output out)
{
    gboolean success  = TRUE;
    gint     newWidth, newHeight, oldWidth, oldHeight;
    gint     posX = 0, posY = 0;
    gboolean keepX = FALSE, keepY = FALSE;

    oldWidth  = gimp_image_get_width(out->image);
    oldHeight = gimp_image_get_height(out->image);

    if (settings->manual) {
        newWidth  = min(oldWidth,  settings->new_w);
        newHeight = min(oldHeight, settings->new_h);
    } else {
        float ratio1, ratio2;
        if (settings->ratio == CROP_PRESET_CUSTOM) {
            ratio1 = settings->custom_ratio1;
            ratio2 = settings->custom_ratio2;
        } else {
            ratio1 = (float)crop_preset_ratio[settings->ratio][0];
            ratio2 = (float)crop_preset_ratio[settings->ratio][1];
        }

        if (((float)oldWidth / oldHeight) > (ratio1 / ratio2)) {
            newHeight = oldHeight;
            newWidth  = (gint)round((ratio1 * (float)newHeight) / ratio2);
            keepY     = TRUE;
        } else {
            newWidth  = oldWidth;
            newHeight = (gint)round((ratio2 * (float)newWidth) / ratio1);
            keepX     = TRUE;
        }
    }

    switch (settings->start_pos) {
        case CROP_START_TL: posX = 0;                posY = 0;                break;
        case CROP_START_TR: posX = oldWidth - newWidth; posY = 0;             break;
        case CROP_START_BL: posX = 0;                posY = oldHeight - newHeight; break;
        case CROP_START_BR: posX = oldWidth - newWidth; posY = oldHeight - newHeight; break;
        default:
            if (!keepX) posX = (oldWidth  - newWidth)  / 2;
            if (!keepY) posY = (oldHeight - newHeight) / 2;
            break;
    }

    success = gimp_image_crop(out->image, newWidth, newHeight, posX, posY);
    return success;
}

static gboolean
apply_fliprotate(fliprotate_settings settings, image_output out)
{
    gboolean success = TRUE;

    if (settings->flip_h)
        success = gimp_image_flip(out->image, GIMP_ORIENTATION_HORIZONTAL);

    if (settings->flip_v)
        success = gimp_image_flip(out->image, GIMP_ORIENTATION_VERTICAL);

    if (settings->rotate)
        success = gimp_image_rotate(out->image, settings->rotation_type);

    return success;
}

static gboolean
apply_color(color_settings settings, image_output out)
{
    gboolean   success          = TRUE;
    GimpLayer *default_layer    = first_layer(out);
    GimpDrawable *default_drawable = GIMP_DRAWABLE(default_layer);

    if (settings->brightness != 0 || settings->contrast != 0) {
        if (!gimp_drawable_is_rgb(default_drawable))
            gimp_image_convert_rgb(out->image);

        GList *node;
        for (node = out->layers; node; node = node->next) {
            success = gimp_drawable_brightness_contrast(
                GIMP_DRAWABLE(node->data),
                settings->brightness,
                settings->contrast
            );
        }
    }

    if (settings->grayscale && !gimp_drawable_is_gray(default_drawable))
        success = gimp_image_convert_grayscale(out->image);

    if (settings->levels_auto) {
        GList *node;
        for (node = out->layers; node; node = node->next)
            success = gimp_drawable_levels_stretch(GIMP_DRAWABLE(node->data));
    }

    if (settings->curve_file != NULL &&
        !gimp_drawable_is_indexed(default_drawable)) {
        if (!colorcurve_init) {
            success = parse_curve_file(
                settings->curve_file,
                &colorcurve_num_points_v, &colorcurve_ctr_points_v,
                &colorcurve_num_points_r, &colorcurve_ctr_points_r,
                &colorcurve_num_points_g, &colorcurve_ctr_points_g,
                &colorcurve_num_points_b, &colorcurve_ctr_points_b,
                &colorcurve_num_points_a, &colorcurve_ctr_points_a
            );
            colorcurve_init = TRUE;
        } else {
            success = TRUE;
        }

        if (success) {
            GList *node;
            for (node = out->layers; node; node = node->next) {
                GimpDrawable *drw = GIMP_DRAWABLE(node->data);
                if (colorcurve_num_points_v >= 4 && colorcurve_num_points_v <= 34)
                    success = gimp_drawable_curves_spline(drw, GIMP_HISTOGRAM_VALUE,
                                colorcurve_num_points_v, colorcurve_ctr_points_v);
                if (colorcurve_num_points_r >= 4 && colorcurve_num_points_r <= 34)
                    success = gimp_drawable_curves_spline(drw, GIMP_HISTOGRAM_RED,
                                colorcurve_num_points_r, colorcurve_ctr_points_r);
                if (colorcurve_num_points_g >= 4 && colorcurve_num_points_g <= 34)
                    success = gimp_drawable_curves_spline(drw, GIMP_HISTOGRAM_GREEN,
                                colorcurve_num_points_g, colorcurve_ctr_points_g);
                if (colorcurve_num_points_b >= 4 && colorcurve_num_points_b <= 34)
                    success = gimp_drawable_curves_spline(drw, GIMP_HISTOGRAM_BLUE,
                                colorcurve_num_points_b, colorcurve_ctr_points_b);
                if (colorcurve_num_points_a >= 4 && colorcurve_num_points_a <= 34)
                    success = gimp_drawable_curves_spline(drw, GIMP_HISTOGRAM_ALPHA,
                                colorcurve_num_points_a, colorcurve_ctr_points_a);
            }
        }
    }

    return success;
}

static gboolean
apply_sharpblur(sharpblur_settings settings, image_output out)
{
    GimpPDB *pdb = gimp_get_pdb();
    GList   *node;

    if (settings->amount < 0) {
        for (node = out->layers; node; node = node->next) {
            GimpValueArray *args = gimp_value_array_new(4);
            GValue v = G_VALUE_INIT;

            g_value_init(&v, GIMP_TYPE_RUN_MODE);
            g_value_set_enum(&v, GIMP_RUN_NONINTERACTIVE);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, GIMP_TYPE_IMAGE);
            g_value_set_object(&v, out->image);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, GIMP_TYPE_DRAWABLE);
            g_value_set_object(&v, node->data);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, G_TYPE_INT);
            g_value_set_int(&v, -(settings->amount));
            gimp_value_array_append(args, &v); g_value_unset(&v);

            GimpValueArray *result =
                gimp_pdb_run_procedure_array(pdb, "plug-in-sharpen", args);
            gimp_value_array_unref(result);
            gimp_value_array_unref(args);
        }
    } else if (settings->amount > 0) {
        float minsize = (float)min(
            gimp_image_get_width(out->image) / 4,
            gimp_image_get_height(out->image) / 4
        );
        float radius = (minsize / 100.0f) * settings->amount;

        for (node = out->layers; node; node = node->next) {
            GimpValueArray *args = gimp_value_array_new(6);
            GValue v = G_VALUE_INIT;

            g_value_init(&v, GIMP_TYPE_RUN_MODE);
            g_value_set_enum(&v, GIMP_RUN_NONINTERACTIVE);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, GIMP_TYPE_IMAGE);
            g_value_set_object(&v, out->image);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, GIMP_TYPE_DRAWABLE);
            g_value_set_object(&v, node->data);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, G_TYPE_DOUBLE);
            g_value_set_double(&v, radius);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, G_TYPE_DOUBLE);
            g_value_set_double(&v, radius);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            g_value_init(&v, G_TYPE_INT);
            g_value_set_int(&v, 0);
            gimp_value_array_append(args, &v); g_value_unset(&v);

            GimpValueArray *result =
                gimp_pdb_run_procedure_array(pdb, "plug-in-gauss", args);
            gimp_value_array_unref(result);
            gimp_value_array_unref(args);
        }
    }

    return TRUE;
}

static gboolean
apply_watermark(watermark_settings settings, image_output out)
{
    gboolean   success   = TRUE;
    GimpLayer *wm_layer  = NULL;
    gdouble    posX, posY;
    gint       wmwidth, wmheight;

    gint imgwidth  = gimp_image_get_width(out->image);
    gint imgheight = gimp_image_get_height(out->image);

    if (settings->mode) {
        if (strlen(settings->text) == 0)
            return TRUE;

        gdouble font_size  = pango_font_description_get_size(settings->font) / PANGO_SCALE;
        const char *family = pango_font_description_get_family(settings->font);

        PangoFontMap    *font_map = pango_cairo_font_map_get_default();
        PangoContext    *context  = pango_font_map_create_context(font_map);
        PangoLayout     *layout   = pango_layout_new(context);
        PangoFontDescription *fd  = pango_font_description_copy(settings->font);
        pango_font_description_set_size(fd, (gint)(font_size * PANGO_SCALE));
        pango_layout_set_font_description(layout, fd);
        pango_layout_set_text(layout, settings->text, -1);
        pango_layout_get_pixel_size(layout, &wmwidth, &wmheight);
        pango_font_description_free(fd);
        g_object_unref(layout);
        g_object_unref(context);

        calc_watermark_xy(imgwidth, imgheight, wmwidth, wmheight,
                          settings->position, settings->edge_distance,
                          &posX, &posY);

        GeglColor *old_fg = gimp_context_get_foreground();
        GeglColor *new_fg = gegl_color_from_gdkrgba(&settings->color);
        gimp_context_set_foreground(new_fg);

        wm_layer = gimp_text_fontname(
            out->image, NULL,
            posX, posY,
            settings->text, -1, TRUE,
            font_size, GIMP_PIXELS,
            family
        );

        gimp_context_set_foreground(old_fg);
        g_object_unref(new_fg);
        g_object_unref(old_fg);

        if (wm_layer)
            gimp_layer_set_opacity(wm_layer, settings->opacity);
    } else {
        if (!g_file_test(settings->image_file, G_FILE_TEST_IS_REGULAR))
            return TRUE;

        GFile *wm_file = g_file_new_for_path(settings->image_file);
        wm_layer = gimp_file_load_layer(
            GIMP_RUN_NONINTERACTIVE, out->image, wm_file);
        g_object_unref(wm_file);

        gimp_image_insert_layer(out->image, wm_layer, NULL, 0);

        wmwidth  = gimp_drawable_get_width(GIMP_DRAWABLE(wm_layer));
        wmheight = gimp_drawable_get_height(GIMP_DRAWABLE(wm_layer));

        if (settings->image_sizemode != WM_IMG_NOSIZE) {
            if (settings->image_sizemode == WM_IMG_SIZEW) {
                float wmwidth_ = (imgwidth * settings->image_size_percent) / 100.0f;
                float diff     = (wmwidth_ / wmwidth) * 100.0f;
                wmheight       = (gint)round((wmheight * diff) / 100.0f);
                wmwidth        = (gint)round(wmwidth_);
            } else if (settings->image_sizemode == WM_IMG_SIZEH) {
                float wmheight_ = (imgheight * settings->image_size_percent) / 100.0f;
                float diff      = (wmheight_ / wmheight) * 100.0f;
                wmwidth         = (gint)round((wmwidth * diff) / 100.0f);
                wmheight        = (gint)round(wmheight_);
            }

            GimpInterpolationType old_interp = gimp_context_get_interpolation();
            gimp_context_set_interpolation(GIMP_INTERPOLATION_CUBIC);
            success = gimp_layer_scale(wm_layer, wmwidth, wmheight, TRUE);
            gimp_context_set_interpolation(old_interp);
        }

        gimp_layer_set_opacity(wm_layer, settings->opacity);

        calc_watermark_xy(imgwidth, imgheight, wmwidth, wmheight,
                          settings->position, settings->edge_distance,
                          &posX, &posY);

        gimp_layer_set_offsets(wm_layer, (gint)posX, (gint)posY);
    }

    g_list_free(out->layers);
    out->layers = gimp_image_get_layers(out->image);

    return success;
}

static void
calc_watermark_xy(int imgwidth, int imgheight, int wmwidth, int wmheight,
                  watermark_position position, int edge,
                  gdouble *posX, gdouble *posY)
{
    switch (position) {
        case WM_POS_TL: *posX = edge;                          *posY = edge;                          break;
        case WM_POS_TC: *posX = (imgwidth/2) - (wmwidth/2);   *posY = edge;                          break;
        case WM_POS_TR: *posX = imgwidth - wmwidth - edge;     *posY = edge;                          break;
        case WM_POS_BL: *posX = edge;                          *posY = imgheight - wmheight - edge;   break;
        case WM_POS_BC: *posX = (imgwidth/2) - (wmwidth/2);   *posY = imgheight - wmheight - edge;   break;
        case WM_POS_BR: *posX = imgwidth - wmwidth - edge;     *posY = imgheight - wmheight - edge;   break;
        case WM_POS_CL: *posX = edge;                          *posY = (imgheight/2) - (wmheight/2);  break;
        case WM_POS_CR: *posX = imgwidth - wmwidth - edge;     *posY = (imgheight/2) - (wmheight/2);  break;
        default:        *posX = (imgwidth/2) - (wmwidth/2);   *posY = (imgheight/2) - (wmheight/2);  break;
    }
}

static gboolean
apply_userdef(userdef_settings settings, image_output out)
{
    GimpPDB  *pdb            = gimp_get_pdb();
    gboolean  saving_function =
        (strstr(settings->procedure, "-save") != NULL);

    GimpLayer *single_drawable =
        gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
    gimp_selection_none(out->image);

    GimpValueArray *args = settings->params;

    if (args) {
        for (gint i = 0; i < gimp_value_array_length(args); i++) {
            GValue *val  = gimp_value_array_index(args, i);
            GType   type = G_VALUE_TYPE(val);

            if (type == GIMP_TYPE_IMAGE) {
                g_value_set_object(val, out->image);
            } else if (type == GIMP_TYPE_DRAWABLE || type == GIMP_TYPE_ITEM) {
                g_value_set_object(val, single_drawable);
            } else if (type == G_TYPE_STRING && saving_function) {
                GimpProcedure *proc =
                    gimp_pdb_lookup_procedure(pdb, settings->procedure);
                if (proc) {
                    gint        n_args  = 0;
                    GParamSpec **pspecs =
                        gimp_procedure_get_arguments(proc, &n_args);
                    if (i < n_args) {
                        const gchar *param_name = g_param_spec_get_name(pspecs[i]);
                        if (g_strcmp0(param_name, "filename") == 0 ||
                            g_strcmp0(param_name, "uri") == 0) {
                            g_value_set_string(val, out->filepath);
                        }
                    }
                }
            }
        }
    }

    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, settings->procedure, args);
    gimp_value_array_unref(result);

    gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);

    g_list_free(out->layers);
    out->layers = gimp_image_get_layers(out->image);

    return TRUE;
}

static gboolean
apply_rename(rename_settings settings, image_output out, char *orig_basename)
{
    char *orig_name = g_strdup(orig_basename);

    out->filename = g_strdup(settings->pattern);

    if (strstr(out->filename, RENAME_KEY_ORIG) != NULL)
        out->filename = str_replace(out->filename, RENAME_KEY_ORIG, orig_name);

    if (strstr(out->filename, RENAME_KEY_COUNT) != NULL) {
        char strcount[5];
        sprintf(strcount, "%i", processed_count + 1);
        out->filename = str_replace(out->filename, RENAME_KEY_COUNT, strcount);
    }

    if (strstr(out->filename, RENAME_KEY_DATETIME) != NULL)
        out->filename = str_replace(out->filename, RENAME_KEY_DATETIME,
                                    current_datetime);

    g_free(orig_name);
    return TRUE;
}

/* -------------------------------------------------------------------------
 * File save helpers
 * ---------------------------------------------------------------------- */

static gboolean
image_save(format_type type, image_output imageout, format_params params)
{
    gboolean result = FALSE;

    if (type == FORMAT_BMP) {
        result = image_save_bmp(imageout);
    } else if (type == FORMAT_GIF) {
        result = image_save_gif(imageout, ((format_params_gif)params)->interlace);
    } else if (type == FORMAT_ICON) {
        result = image_save_icon(imageout);
    } else if (type == FORMAT_JPEG) {
        result = image_save_jpeg(
            imageout,
            ((format_params_jpeg)params)->quality,
            ((format_params_jpeg)params)->smoothing,
            ((format_params_jpeg)params)->entropy,
            ((format_params_jpeg)params)->progressive,
            ((format_params_jpeg)params)->comment,
            ((format_params_jpeg)params)->subsampling,
            ((format_params_jpeg)params)->baseline,
            ((format_params_jpeg)params)->markers,
            ((format_params_jpeg)params)->dct
        );
    } else if (type == FORMAT_PNG) {
        result = image_save_png(
            imageout,
            ((format_params_png)params)->interlace,
            ((format_params_png)params)->compression,
            ((format_params_png)params)->savebgc,
            ((format_params_png)params)->savegamma,
            ((format_params_png)params)->saveoff,
            ((format_params_png)params)->savephys,
            ((format_params_png)params)->savetime,
            ((format_params_png)params)->savecomm,
            ((format_params_png)params)->savetrans
        );
    } else if (type == FORMAT_TGA) {
        result = image_save_tga(
            imageout,
            ((format_params_tga)params)->rle,
            ((format_params_tga)params)->origin
        );
    } else if (type == FORMAT_TIFF) {
        result = image_save_tiff(imageout, ((format_params_tiff)params)->compression);
    } else if (type == FORMAT_HEIF) {
        result = image_save_heif(
            imageout,
            ((format_params_heif)params)->quality,
            ((format_params_heif)params)->lossless
        );
    } else if (type == FORMAT_WEBP) {
        result = image_save_webp(
            imageout,
            ((format_params_webp)params)->preset,
            ((format_params_webp)params)->lossless,
            ((format_params_webp)params)->quality,
            ((format_params_webp)params)->alpha_quality,
            ((format_params_webp)params)->animation,
            ((format_params_webp)params)->anim_loop,
            ((format_params_webp)params)->minimize_size,
            ((format_params_webp)params)->kf_distance,
            ((format_params_webp)params)->exif,
            ((format_params_webp)params)->iptc,
            ((format_params_webp)params)->xmp,
            ((format_params_webp)params)->delay,
            ((format_params_webp)params)->force_delay
        );
    } else if (type == FORMAT_AVIF) {
        result = image_save_avif(
            imageout,
            ((format_params_avif)params)->lossless,
            ((format_params_avif)params)->quality
        );
    } else if (type == FORMAT_EXR) {
        result = image_save_exr(imageout);
    } else {
        GimpLayer *final_drawable = merge_visible(imageout);

        if (file_has_extension(imageout->filename, ".gif") &&
            gimp_drawable_is_rgb(GIMP_DRAWABLE(final_drawable))) {
            gimp_image_convert_indexed(
                imageout->image,
                GIMP_CONVERT_DITHER_FS,
                GIMP_CONVERT_PALETTE_GENERATE,
                gimp_drawable_has_alpha(GIMP_DRAWABLE(final_drawable)) ? 255 : 256,
                TRUE, FALSE, ""
            );
        }

        if (file_has_extension(imageout->filename, ".heif") ||
            file_has_extension(imageout->filename, ".heic")) {
            result = image_save_heif(imageout, 100, TRUE);
        } else if (file_has_extension(imageout->filename, ".webp")) {
            result = image_save_webp(
                imageout, 0, FALSE, 90, 100, FALSE, TRUE, TRUE,
                50, TRUE, TRUE, TRUE, 200, FALSE
            );
        } else {
            GFile *file = g_file_new_for_path(imageout->filepath);
            result = gimp_file_overwrite(
                GIMP_RUN_NONINTERACTIVE, imageout->image,
                GIMP_DRAWABLE(final_drawable), file
            );
            g_object_unref(file);
        }
    }

    return result;
}

/* Helper: build a GimpValueArray for a PDB file-save call.
 * Takes image + drawable + GFile*, plus optional extra GValue entries. */
static GimpValueArray *
build_save_args(GimpImage *image, GimpDrawable *drawable, GFile *file,
                GValue *extras, gint n_extras)
{
    GimpValueArray *args = gimp_value_array_new(3 + n_extras);
    GValue v = G_VALUE_INIT;

    g_value_init(&v, GIMP_TYPE_RUN_MODE);
    g_value_set_enum(&v, GIMP_RUN_NONINTERACTIVE);
    gimp_value_array_append(args, &v); g_value_unset(&v);

    g_value_init(&v, GIMP_TYPE_IMAGE);
    g_value_set_object(&v, image);
    gimp_value_array_append(args, &v); g_value_unset(&v);

    g_value_init(&v, GIMP_TYPE_DRAWABLE);
    g_value_set_object(&v, drawable);
    gimp_value_array_append(args, &v); g_value_unset(&v);

    g_value_init(&v, G_TYPE_FILE);
    g_value_set_object(&v, file);
    gimp_value_array_append(args, &v); g_value_unset(&v);

    for (gint i = 0; i < n_extras; i++)
        gimp_value_array_append(args, &extras[i]);

    return args;
}

static gboolean
image_save_bmp(image_output out)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-bmp-save", args);

    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_gif(image_output out, gboolean interlace)
{
    gimp_image_convert_indexed(
        out->image,
        GIMP_CONVERT_DITHER_FS,
        GIMP_CONVERT_PALETTE_GENERATE,
        gimp_drawable_has_alpha(GIMP_DRAWABLE(first_layer(out))) ? 255 : 256,
        TRUE, FALSE, ""
    );

    GFile   *file = g_file_new_for_path(out->filepath);
    GimpPDB *pdb  = gimp_get_pdb();

    GValue extras[4];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], interlace ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], 1);
    g_value_init(&extras[2], G_TYPE_INT); g_value_set_int(&extras[2], 0);
    g_value_init(&extras[3], G_TYPE_INT); g_value_set_int(&extras[3], 0);

    GimpLayer *drawable = first_layer(out);
    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 4);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-gif-save", args);

    for (gint i = 0; i < 4; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_icon(image_output out)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-ico-save", args);

    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_jpeg(image_output out, float quality, float smoothing,
                gboolean entropy, gboolean progressive, gchar *comment,
                int subsampling, gboolean baseline, int markers, int dct)
{
    GimpLayer *drawable = merge_visible(out);
    if (gimp_drawable_is_indexed(GIMP_DRAWABLE(drawable)))
        gimp_image_convert_rgb(out->image);

    GFile   *file = g_file_new_for_path(out->filepath);
    GimpPDB *pdb  = gimp_get_pdb();

    GValue extras[8];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_DOUBLE);
    g_value_set_double(&extras[0], quality >= 3 ? quality / 100.0 : 0.03);
    g_value_init(&extras[1], G_TYPE_DOUBLE); g_value_set_double(&extras[1], smoothing);
    g_value_init(&extras[2], G_TYPE_INT);    g_value_set_int(&extras[2], entropy ? 1 : 0);
    g_value_init(&extras[3], G_TYPE_INT);    g_value_set_int(&extras[3], progressive ? 1 : 0);
    g_value_init(&extras[4], G_TYPE_STRING); g_value_set_string(&extras[4], comment);
    g_value_init(&extras[5], G_TYPE_INT);    g_value_set_int(&extras[5], subsampling);
    g_value_init(&extras[6], G_TYPE_INT);    g_value_set_int(&extras[6], baseline ? 1 : 0);
    g_value_init(&extras[7], G_TYPE_INT);    g_value_set_int(&extras[7], markers);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 8);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-jpeg-save", args);

    for (gint i = 0; i < 8; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_heif(image_output out, int quality, gboolean lossless)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], quality);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], lossless ? 1 : 0);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-heif-save", args);

    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_png(image_output out, gboolean interlace, int compression,
               gboolean savebgc, gboolean savegamma, gboolean saveoff,
               gboolean savephys, gboolean savetime, gboolean savecomm,
               gboolean savetrans)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[8];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], interlace  ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], compression);
    g_value_init(&extras[2], G_TYPE_INT); g_value_set_int(&extras[2], savebgc    ? 1 : 0);
    g_value_init(&extras[3], G_TYPE_INT); g_value_set_int(&extras[3], savegamma  ? 1 : 0);
    g_value_init(&extras[4], G_TYPE_INT); g_value_set_int(&extras[4], saveoff    ? 1 : 0);
    g_value_init(&extras[5], G_TYPE_INT); g_value_set_int(&extras[5], savephys   ? 1 : 0);
    g_value_init(&extras[6], G_TYPE_INT); g_value_set_int(&extras[6], savetime   ? 1 : 0);
    g_value_init(&extras[7], G_TYPE_INT); g_value_set_int(&extras[7], savecomm   ? 1 : 0);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 8);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-png-save", args);

    for (gint i = 0; i < 8; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_tga(image_output out, gboolean rle, int origin)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], rle ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], origin);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-tga-save", args);

    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_tiff(image_output out, int compression)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[1];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], compression);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 1);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-tiff-save", args);

    g_value_unset(&extras[0]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_webp(image_output out, int preset, gboolean lossless,
                float quality, float alpha_quality, gboolean animation,
                gboolean anim_loop, gboolean minimize_size, int kf_distance,
                gboolean exif, gboolean iptc, gboolean xmp,
                int delay, int force_delay)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[13];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0],  G_TYPE_INT);    g_value_set_int   (&extras[0],  preset);
    g_value_init(&extras[1],  G_TYPE_INT);    g_value_set_int   (&extras[1],  lossless);
    g_value_init(&extras[2],  G_TYPE_DOUBLE); g_value_set_double(&extras[2],  quality);
    g_value_init(&extras[3],  G_TYPE_DOUBLE); g_value_set_double(&extras[3],  alpha_quality);
    g_value_init(&extras[4],  G_TYPE_INT);    g_value_set_int   (&extras[4],  animation);
    g_value_init(&extras[5],  G_TYPE_INT);    g_value_set_int   (&extras[5],  anim_loop);
    g_value_init(&extras[6],  G_TYPE_INT);    g_value_set_int   (&extras[6],  minimize_size);
    g_value_init(&extras[7],  G_TYPE_INT);    g_value_set_int   (&extras[7],  kf_distance);
    g_value_init(&extras[8],  G_TYPE_INT);    g_value_set_int   (&extras[8],  exif);
    g_value_init(&extras[9],  G_TYPE_INT);    g_value_set_int   (&extras[9],  iptc);
    g_value_init(&extras[10], G_TYPE_INT);    g_value_set_int   (&extras[10], xmp);
    g_value_init(&extras[11], G_TYPE_INT);    g_value_set_int   (&extras[11], delay);
    g_value_init(&extras[12], G_TYPE_INT);    g_value_set_int   (&extras[12], force_delay);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 13);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-webp-save", args);

    for (gint i = 0; i < 13; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_avif(image_output out, gboolean lossless, int quality)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], quality);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], lossless ? 1 : 0);

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-heif-av1-save", args);

    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_exr(image_output out)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();

    GimpValueArray *args = build_save_args(
        out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, "file-exr-save", args);

    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

/* -------------------------------------------------------------------------
 * Overwrite confirmation dialog
 * ---------------------------------------------------------------------- */

static int
overwrite_result(char *path, GtkWidget *parent)
{
    gboolean oldfile_access = g_file_test(path, G_FILE_TEST_IS_REGULAR);

    if ((bimp_opt_alertoverwrite == BIMP_ASK_OVERWRITE) && oldfile_access) {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(parent),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File %s already exists, overwrite it?"),
            comp_get_filename(path)
        );
        gtk_window_set_title(GTK_WINDOW(dialog), _("Overwrite?"));

        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *check   = gtk_check_button_new_with_label(
            _("Always apply this decision"));
        gtk_box_append(GTK_BOX(content), check);

        gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Yes"), GTK_RESPONSE_YES);
        gtk_dialog_add_button(GTK_DIALOG(dialog), _("_No"),  GTK_RESPONSE_NO);

        gint     response     = gtk_dialog_run(GTK_DIALOG(dialog));
        gboolean dont_ask    =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check));
        gtk_window_destroy(GTK_WINDOW(dialog));

        if (response == GTK_RESPONSE_YES) {
            if (dont_ask) bimp_opt_alertoverwrite = BIMP_OVERWRITE_SKIP_ASK;
            return 1;
        } else {
            if (dont_ask) bimp_opt_alertoverwrite = BIMP_DONT_OVERWRITE_SKIP_ASK;
            return 0;
        }
    } else {
        if (oldfile_access)
            return (bimp_opt_alertoverwrite == BIMP_OVERWRITE_SKIP_ASK) ? 1 : 0;
        return 2;
    }
}
