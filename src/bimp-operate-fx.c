#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <gegl.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include "bimp-operate-priv.h"
#include "bimp-manipulations.h"
#include "bimp-utils.h"
#include "bimp.h"
#include "plugin-intl.h"

static void calc_watermark_xy(int, int, int, int, watermark_position, int, gdouble *, gdouble *);

static gboolean
apply_sharpen(sharpblur_settings settings, image_output out)
{
    GimpPDB *pdb  = gimp_get_pdb();
    GList   *node;

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
        GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "plug-in-sharpen", args);
        gimp_value_array_unref(result);
        gimp_value_array_unref(args);
    }
    return TRUE;
}

static gboolean
apply_blur(sharpblur_settings settings, image_output out)
{
    GimpPDB *pdb  = gimp_get_pdb();
    GList   *node;
    float    minsize = (float)min(gimp_image_get_width(out->image) / 4,
                                  gimp_image_get_height(out->image) / 4);
    float    radius  = (minsize / 100.0f) * settings->amount;

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
        g_value_init(&v, G_TYPE_DOUBLE); g_value_set_double(&v, radius);
        gimp_value_array_append(args, &v); g_value_unset(&v);
        g_value_init(&v, G_TYPE_DOUBLE); g_value_set_double(&v, radius);
        gimp_value_array_append(args, &v); g_value_unset(&v);
        g_value_init(&v, G_TYPE_INT); g_value_set_int(&v, 0);
        gimp_value_array_append(args, &v); g_value_unset(&v);
        GimpValueArray *result = gimp_pdb_run_procedure_array(pdb, "plug-in-gauss", args);
        gimp_value_array_unref(result);
        gimp_value_array_unref(args);
    }
    return TRUE;
}

gboolean
apply_sharpblur(sharpblur_settings settings, image_output out)
{
    if (settings->amount < 0) return apply_sharpen(settings, out);
    if (settings->amount > 0) return apply_blur(settings, out);
    return TRUE;
}

static gboolean
apply_watermark_text(watermark_settings settings, image_output out,
                     gint imgwidth, gint imgheight)
{
    gdouble      posX, posY;
    gint         wmwidth, wmheight;
    gdouble      font_size = pango_font_description_get_size(settings->font) / PANGO_SCALE;
    const char  *family    = pango_font_description_get_family(settings->font);
    PangoFontMap *font_map = pango_cairo_font_map_get_default();
    PangoContext *context  = pango_font_map_create_context(font_map);
    PangoLayout  *layout   = pango_layout_new(context);
    PangoFontDescription *fd = pango_font_description_copy(settings->font);
    pango_font_description_set_size(fd, (gint)(font_size * PANGO_SCALE));
    pango_layout_set_font_description(layout, fd);
    pango_layout_set_text(layout, settings->text, -1);
    pango_layout_get_pixel_size(layout, &wmwidth, &wmheight);
    pango_font_description_free(fd);
    g_object_unref(layout);
    g_object_unref(context);

    calc_watermark_xy(imgwidth, imgheight, wmwidth, wmheight,
                      settings->position, settings->edge_distance, &posX, &posY);

    GeglColor *old_fg = gimp_context_get_foreground();
    GeglColor *new_fg = gegl_color_from_gdkrgba(&settings->color);
    gimp_context_set_foreground(new_fg);
    GimpLayer *wm_layer = gimp_text_fontname(out->image, NULL, posX, posY,
        settings->text, -1, TRUE, font_size, GIMP_PIXELS, family);
    gimp_context_set_foreground(old_fg);
    g_object_unref(new_fg);
    g_object_unref(old_fg);

    if (wm_layer) gimp_layer_set_opacity(wm_layer, settings->opacity);
    return TRUE;
}

static gboolean
apply_watermark_image(watermark_settings settings, image_output out,
                      gint imgwidth, gint imgheight)
{
    gdouble    posX, posY;
    gint       wmwidth, wmheight;
    GFile     *wm_file  = g_file_new_for_path(settings->image_file);
    GimpLayer *wm_layer = gimp_file_load_layer(GIMP_RUN_NONINTERACTIVE, out->image, wm_file);
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
        gimp_layer_scale(wm_layer, wmwidth, wmheight, TRUE);
        gimp_context_set_interpolation(old_interp);
    }
    gimp_layer_set_opacity(wm_layer, settings->opacity);
    calc_watermark_xy(imgwidth, imgheight, wmwidth, wmheight,
                      settings->position, settings->edge_distance, &posX, &posY);
    gimp_layer_set_offsets(wm_layer, (gint)posX, (gint)posY);
    return TRUE;
}

gboolean
apply_watermark(watermark_settings settings, image_output out)
{
    gint     imgwidth  = gimp_image_get_width(out->image);
    gint     imgheight = gimp_image_get_height(out->image);
    gboolean success   = TRUE;

    if (settings->mode) {
        if (strlen(settings->text) == 0) return TRUE;
        success = apply_watermark_text(settings, out, imgwidth, imgheight);
    } else {
        if (!g_file_test(settings->image_file, G_FILE_TEST_IS_REGULAR)) return TRUE;
        success = apply_watermark_image(settings, out, imgwidth, imgheight);
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
        case WM_POS_TL: *posX = edge;                       *posY = edge;                       break;
        case WM_POS_TC: *posX = (imgwidth/2)-(wmwidth/2);  *posY = edge;                       break;
        case WM_POS_TR: *posX = imgwidth-wmwidth-edge;      *posY = edge;                       break;
        case WM_POS_BL: *posX = edge;                       *posY = imgheight-wmheight-edge;    break;
        case WM_POS_BC: *posX = (imgwidth/2)-(wmwidth/2);  *posY = imgheight-wmheight-edge;    break;
        case WM_POS_BR: *posX = imgwidth-wmwidth-edge;      *posY = imgheight-wmheight-edge;    break;
        case WM_POS_CL: *posX = edge;                       *posY = (imgheight/2)-(wmheight/2); break;
        case WM_POS_CR: *posX = imgwidth-wmwidth-edge;      *posY = (imgheight/2)-(wmheight/2); break;
        default:        *posX = (imgwidth/2)-(wmwidth/2);  *posY = (imgheight/2)-(wmheight/2); break;
    }
}

static void
fix_userdef_args(GimpValueArray *args, userdef_settings settings,
                 image_output out, GimpLayer *drawable)
{
    GimpPDB  *pdb            = gimp_get_pdb();
    gboolean  saving_function = (strstr(settings->procedure, "-save") != NULL);

    for (gint i = 0; i < gimp_value_array_length(args); i++) {
        GValue *val  = gimp_value_array_index(args, i);
        GType   type = G_VALUE_TYPE(val);
        if (type == GIMP_TYPE_IMAGE) {
            g_value_set_object(val, out->image);
        } else if (type == GIMP_TYPE_DRAWABLE || type == GIMP_TYPE_ITEM) {
            g_value_set_object(val, GIMP_DRAWABLE(drawable));
        } else if (type == G_TYPE_STRING && saving_function) {
            GimpProcedure *proc = gimp_pdb_lookup_procedure(pdb, settings->procedure);
            if (proc) {
                gint n_args = 0;
                GParamSpec **pspecs = gimp_procedure_get_arguments(proc, &n_args);
                if (i < n_args) {
                    const gchar *nm = g_param_spec_get_name(pspecs[i]);
                    if (!g_strcmp0(nm, "filename") || !g_strcmp0(nm, "uri"))
                        g_value_set_string(val, out->filepath);
                }
            }
        }
    }
}

gboolean
apply_userdef(userdef_settings settings, image_output out)
{
    GimpPDB   *pdb            = gimp_get_pdb();
    GimpLayer *single_drawable =
        gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
    gimp_selection_none(out->image);

    GimpValueArray *args = settings->params;
    if (args) fix_userdef_args(args, settings, out, single_drawable);

    GimpValueArray *result =
        gimp_pdb_run_procedure_array(pdb, settings->procedure, args);
    gimp_value_array_unref(result);

    gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
    g_list_free(out->layers);
    out->layers = gimp_image_get_layers(out->image);
    return TRUE;
}
