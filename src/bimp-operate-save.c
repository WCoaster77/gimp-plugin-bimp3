#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "bimp-operate-priv.h"
#include "bimp-manipulations.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

static GimpValueArray *
build_save_args(GimpImage *image, GimpDrawable *drawable, GFile *file,
                GValue *extras, gint n_extras)
{
    GimpValueArray *args = gimp_value_array_new(4 + n_extras);
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

    GimpValueArray *args   = build_save_args(out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-bmp-save", args);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_gif(image_output out, gboolean interlace)
{
    gimp_image_convert_indexed(out->image, GIMP_CONVERT_DITHER_FS,
        GIMP_CONVERT_PALETTE_GENERATE,
        gimp_drawable_has_alpha(GIMP_DRAWABLE(first_layer(out))) ? 255 : 256,
        TRUE, FALSE, "");

    GFile   *file = g_file_new_for_path(out->filepath);
    GimpPDB *pdb  = gimp_get_pdb();
    GValue extras[4];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], interlace ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], 1);
    g_value_init(&extras[2], G_TYPE_INT); g_value_set_int(&extras[2], 0);
    g_value_init(&extras[3], G_TYPE_INT); g_value_set_int(&extras[3], 0);

    GimpLayer *drawable = first_layer(out);
    GimpValueArray *args   = build_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 4);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-gif-save", args);
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

    GimpValueArray *args   = build_save_args(out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-ico-save", args);
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

    GimpValueArray *args   = build_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 8);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-jpeg-save", args);
    for (gint i = 0; i < 8; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

static gboolean
image_save_raster(image_output out)
{
    GimpLayer *final_drawable = merge_visible(out);

    if (file_has_extension(out->filename, ".gif") &&
        gimp_drawable_is_rgb(GIMP_DRAWABLE(final_drawable))) {
        gimp_image_convert_indexed(out->image, GIMP_CONVERT_DITHER_FS,
            GIMP_CONVERT_PALETTE_GENERATE,
            gimp_drawable_has_alpha(GIMP_DRAWABLE(final_drawable)) ? 255 : 256,
            TRUE, FALSE, "");
    }

    if (file_has_extension(out->filename, ".heif") ||
        file_has_extension(out->filename, ".heic")) {
        return image_save_heif(out, 100, TRUE);
    } else if (file_has_extension(out->filename, ".webp")) {
        return image_save_webp(out, 0, FALSE, 90, 100, FALSE, TRUE, TRUE,
                               50, TRUE, TRUE, TRUE, 200, FALSE);
    }

    GFile *file   = g_file_new_for_path(out->filepath);
    gboolean result = gimp_file_overwrite(GIMP_RUN_NONINTERACTIVE, out->image,
                                          GIMP_DRAWABLE(final_drawable), file);
    g_object_unref(file);
    return result;
}

static gboolean
image_save_formats1(format_type type, image_output out, format_params p)
{
    if (type == FORMAT_BMP)  return image_save_bmp(out);
    if (type == FORMAT_GIF)  return image_save_gif(out, ((format_params_gif)p)->interlace);
    if (type == FORMAT_ICON) return image_save_icon(out);
    if (type == FORMAT_JPEG)
        return image_save_jpeg(out,
            ((format_params_jpeg)p)->quality,   ((format_params_jpeg)p)->smoothing,
            ((format_params_jpeg)p)->entropy,   ((format_params_jpeg)p)->progressive,
            ((format_params_jpeg)p)->comment,   ((format_params_jpeg)p)->subsampling,
            ((format_params_jpeg)p)->baseline,  ((format_params_jpeg)p)->markers,
            ((format_params_jpeg)p)->dct);
    return FALSE;
}

static gboolean
image_save_formats2(format_type type, image_output out, format_params p)
{
    if (type == FORMAT_PNG)
        return image_save_png(out,
            ((format_params_png)p)->interlace,  ((format_params_png)p)->compression,
            ((format_params_png)p)->savebgc,    ((format_params_png)p)->savegamma,
            ((format_params_png)p)->saveoff,    ((format_params_png)p)->savephys,
            ((format_params_png)p)->savetime,   ((format_params_png)p)->savecomm,
            ((format_params_png)p)->savetrans);
    if (type == FORMAT_TGA)
        return image_save_tga(out, ((format_params_tga)p)->rle,
                              ((format_params_tga)p)->origin);
    if (type == FORMAT_TIFF)
        return image_save_tiff(out, ((format_params_tiff)p)->compression);
    if (type == FORMAT_HEIF)
        return image_save_heif(out, ((format_params_heif)p)->quality,
                               ((format_params_heif)p)->lossless);
    if (type == FORMAT_WEBP)
        return image_save_webp(out,
            ((format_params_webp)p)->preset,        ((format_params_webp)p)->lossless,
            ((format_params_webp)p)->quality,       ((format_params_webp)p)->alpha_quality,
            ((format_params_webp)p)->animation,     ((format_params_webp)p)->anim_loop,
            ((format_params_webp)p)->minimize_size, ((format_params_webp)p)->kf_distance,
            ((format_params_webp)p)->exif,          ((format_params_webp)p)->iptc,
            ((format_params_webp)p)->xmp,           ((format_params_webp)p)->delay,
            ((format_params_webp)p)->force_delay);
    if (type == FORMAT_AVIF)
        return image_save_avif(out, ((format_params_avif)p)->lossless,
                               ((format_params_avif)p)->quality);
    if (type == FORMAT_EXR)  return image_save_exr(out);
    return FALSE;
}

gboolean
image_save(format_type type, image_output out, format_params params)
{
    if (type == -1) return image_save_raster(out);
    gboolean result = image_save_formats1(type, out, params);
    if (!result) result = image_save_formats2(type, out, params);
    return result;
}
