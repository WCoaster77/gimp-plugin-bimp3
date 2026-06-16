#include <string.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "bimp-operate-priv.h"
#include "bimp-manipulations.h"

static GimpValueArray *
local_save_args(GimpImage *image, GimpDrawable *drawable, GFile *file,
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

gboolean
image_save_heif(image_output out, int quality, gboolean lossless)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();
    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], quality);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], lossless ? 1 : 0);
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-heif-save", args);
    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
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
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], interlace   ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], compression);
    g_value_init(&extras[2], G_TYPE_INT); g_value_set_int(&extras[2], savebgc    ? 1 : 0);
    g_value_init(&extras[3], G_TYPE_INT); g_value_set_int(&extras[3], savegamma  ? 1 : 0);
    g_value_init(&extras[4], G_TYPE_INT); g_value_set_int(&extras[4], saveoff    ? 1 : 0);
    g_value_init(&extras[5], G_TYPE_INT); g_value_set_int(&extras[5], savephys   ? 1 : 0);
    g_value_init(&extras[6], G_TYPE_INT); g_value_set_int(&extras[6], savetime   ? 1 : 0);
    g_value_init(&extras[7], G_TYPE_INT); g_value_set_int(&extras[7], savecomm   ? 1 : 0);
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 8);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-png-save", args);
    for (gint i = 0; i < 8; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
image_save_tga(image_output out, gboolean rle, int origin)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();
    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], rle ? 1 : 0);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], origin);
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-tga-save", args);
    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
image_save_tiff(image_output out, int compression)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();
    GValue extras[1];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], compression);
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 1);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-tiff-save", args);
    g_value_unset(&extras[0]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
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
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 13);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-webp-save", args);
    for (gint i = 0; i < 13; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
image_save_avif(image_output out, gboolean lossless, int quality)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();
    GValue extras[2];
    memset(extras, 0, sizeof(extras));
    g_value_init(&extras[0], G_TYPE_INT); g_value_set_int(&extras[0], quality);
    g_value_init(&extras[1], G_TYPE_INT); g_value_set_int(&extras[1], lossless ? 1 : 0);
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, extras, 2);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-heif-av1-save", args);
    for (gint i = 0; i < 2; i++) g_value_unset(&extras[i]);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}

gboolean
image_save_exr(image_output out)
{
    GimpLayer *drawable = merge_visible(out);
    GFile     *file     = g_file_new_for_path(out->filepath);
    GimpPDB   *pdb      = gimp_get_pdb();
    GimpValueArray *args   = local_save_args(out->image, GIMP_DRAWABLE(drawable), file, NULL, 0);
    GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "file-exr-save", args);
    gimp_value_array_unref(result);
    gimp_value_array_unref(args);
    g_object_unref(file);
    return TRUE;
}
