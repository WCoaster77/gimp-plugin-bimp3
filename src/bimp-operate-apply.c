#include <float.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <gegl.h>
#include "bimp-operate-priv.h"
#include "bimp-manipulations.h"
#include "bimp-utils.h"
#include "bimp-serialize.h"
#include "plugin-intl.h"

static gboolean colorcurve_init;
static int      colorcurve_num_points_v;
static gdouble *colorcurve_ctr_points_v;
static int      colorcurve_num_points_r;
static gdouble *colorcurve_ctr_points_r;
static int      colorcurve_num_points_g;
static gdouble *colorcurve_ctr_points_g;
static int      colorcurve_num_points_b;
static gdouble *colorcurve_ctr_points_b;
static int      colorcurve_num_points_a;
static gdouble *colorcurve_ctr_points_a;

void
bimp_apply_color_reset(void)
{
    colorcurve_init = FALSE;
}

static void
calc_resize_dims(resize_settings settings, gint orig_w, gint orig_h,
                 gint *final_w, gint *final_h, gint *view_w, gint *view_h)
{
    gdouble newwpct, newwpctmax, newhpct, newhpctmax;

    if (settings->resize_mode_width == RESIZE_PERCENT)
        newwpct = newwpctmax = settings->new_w_pc / 100.0;
    else if (settings->resize_mode_width == RESIZE_PIXEL)
        newwpct = newwpctmax = (double)settings->new_w_px / (double)orig_w;
    else { newwpct = 1; newwpctmax = DBL_MAX; }

    if (settings->resize_mode_height == RESIZE_PERCENT)
        newhpct = newhpctmax = settings->new_h_pc / 100.0;
    else if (settings->resize_mode_height == RESIZE_PIXEL)
        newhpct = newhpctmax = (double)settings->new_h_px / (double)orig_h;
    else { newhpct = 1; newhpctmax = DBL_MAX; }

    if (settings->stretch_mode == STRETCH_ASPECT) {
        gdouble newpct = min(newwpctmax, newhpctmax);
        *final_w = *view_w = (gint)round(orig_w * newpct);
        *final_h = *view_h = (gint)round(orig_h * newpct);
    } else if (settings->stretch_mode == STRETCH_PADDED) {
        gdouble newpct = min(newwpctmax, newhpctmax);
        *final_w = (gint)round(orig_w * newpct);
        *final_h = (gint)round(orig_h * newpct);
        *view_w  = (gint)round(orig_w * newwpct);
        *view_h  = (gint)round(orig_h * newhpct);
    } else {
        *final_w = *view_w = (gint)round(orig_w * newwpct);
        *final_h = *view_h = (gint)round(orig_h * newhpct);
    }
}

static void
apply_resize_padding(resize_settings settings, image_output out,
                     gint final_w, gint final_h, gint view_w, gint view_h)
{
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
        out->image, "padding_layer", view_w, view_h, layer_type,
        settings->padding_color.alpha * 100.0, GIMP_LAYER_MODE_NORMAL_LEGACY);

    gimp_image_insert_layer(out->image, padding, NULL, 0);
    gimp_image_lower_item_to_bottom(out->image, GIMP_ITEM(padding));

    GeglColor *old_bg = gimp_context_get_background();
    GeglColor *new_bg = gegl_color_from_gdkrgba(&settings->padding_color);
    gimp_context_set_background(new_bg);
    gimp_drawable_fill(GIMP_DRAWABLE(padding), GIMP_FILL_BACKGROUND);
    gimp_context_set_background(old_bg);
    g_object_unref(new_bg);
    g_object_unref(old_bg);

    gimp_item_transform_translate(GIMP_ITEM(padding),
        -(gdouble)abs(view_w - final_w) / 2.0,
        -(gdouble)abs(view_h - final_h) / 2.0);

    gimp_image_resize_to_layers(out->image);
}

gboolean
apply_resize(resize_settings settings, image_output out)
{
    gboolean success = FALSE;
    gint     orig_w, orig_h, final_w, final_h, view_w, view_h;
    gdouble  orig_res_x, orig_res_y;

    if (settings->change_res) {
        success = gimp_image_get_resolution(out->image, &orig_res_x, &orig_res_y);
        if (settings->new_res_x != orig_res_x || settings->new_res_y != orig_res_y)
            success = gimp_image_set_resolution(out->image,
                                                settings->new_res_x, settings->new_res_y);
    }

    orig_w = gimp_image_get_width(out->image);
    orig_h = gimp_image_get_height(out->image);

    if (settings->resize_mode_width  == RESIZE_DISABLE &&
        settings->resize_mode_height == RESIZE_DISABLE)
        return !settings->change_res || success;

    calc_resize_dims(settings, orig_w, orig_h, &final_w, &final_h, &view_w, &view_h);

    GimpInterpolationType old_interp = gimp_context_get_interpolation();
    gimp_context_set_interpolation(settings->interpolation);
    success = gimp_image_scale(out->image, final_w, final_h);
    gimp_context_set_interpolation(old_interp);

    if (settings->stretch_mode == STRETCH_PADDED)
        apply_resize_padding(settings, out, final_w, final_h, view_w, view_h);

    return success;
}

gboolean
apply_crop(crop_settings settings, image_output out)
{
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
        case CROP_START_TL: posX = 0;                      posY = 0;                      break;
        case CROP_START_TR: posX = oldWidth - newWidth;    posY = 0;                      break;
        case CROP_START_BL: posX = 0;                      posY = oldHeight - newHeight;  break;
        case CROP_START_BR: posX = oldWidth - newWidth;    posY = oldHeight - newHeight;  break;
        default:
            if (!keepX) posX = (oldWidth  - newWidth)  / 2;
            if (!keepY) posY = (oldHeight - newHeight) / 2;
            break;
    }
    return gimp_image_crop(out->image, newWidth, newHeight, posX, posY);
}

gboolean
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
apply_color_curves(color_settings settings, image_output out)
{
    gboolean success = TRUE;
    GList   *node;

    if (!colorcurve_init) {
        success = parse_curve_file(settings->curve_file,
            &colorcurve_num_points_v, &colorcurve_ctr_points_v,
            &colorcurve_num_points_r, &colorcurve_ctr_points_r,
            &colorcurve_num_points_g, &colorcurve_ctr_points_g,
            &colorcurve_num_points_b, &colorcurve_ctr_points_b,
            &colorcurve_num_points_a, &colorcurve_ctr_points_a);
        colorcurve_init = TRUE;
        if (!success) return FALSE;
    }

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
    return success;
}

gboolean
apply_color(color_settings settings, image_output out)
{
    gboolean      success          = TRUE;
    GimpLayer    *default_layer    = first_layer(out);
    GimpDrawable *default_drawable = GIMP_DRAWABLE(default_layer);
    GList        *node;

    if (settings->brightness != 0 || settings->contrast != 0) {
        if (!gimp_drawable_is_rgb(default_drawable))
            gimp_image_convert_rgb(out->image);
        for (node = out->layers; node; node = node->next)
            success = gimp_drawable_brightness_contrast(
                GIMP_DRAWABLE(node->data), settings->brightness, settings->contrast);
    }

    if (settings->grayscale && !gimp_drawable_is_gray(default_drawable))
        success = gimp_image_convert_grayscale(out->image);

    if (settings->levels_auto) {
        for (node = out->layers; node; node = node->next)
            success = gimp_drawable_levels_stretch(GIMP_DRAWABLE(node->data));
    }

    if (settings->curve_file != NULL && !gimp_drawable_is_indexed(default_drawable))
        success = apply_color_curves(settings, out);

    return success;
}
