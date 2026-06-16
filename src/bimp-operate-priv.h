#ifndef BIMP_OPERATE_PRIV_H
#define BIMP_OPERATE_PRIV_H

#include <gdk/gdk.h>
#include <gegl.h>
#include <libgimp/gimp.h>
#include "bimp-operate.h"
#include "bimp-manipulations.h"

/* Shared state defined in bimp-operate.c */
extern char     *current_datetime;
extern int       processed_count;
extern int       success_count;
extern int       total_images;
extern char     *common_folder_path;
extern gboolean  list_contains_changeformat;
extern gboolean  list_contains_rename;
extern gboolean  list_contains_watermark;
extern gboolean  list_contains_savingplugin;

/* Image helpers defined in bimp-operate.c */
GeglColor *gegl_color_from_gdkrgba(const GdkRGBA *rgba);
GimpLayer *merge_visible(image_output out);
GimpLayer *first_layer(image_output out);

/* Apply functions — bimp-operate-apply.c */
gboolean apply_resize(resize_settings, image_output);
gboolean apply_crop(crop_settings, image_output);
gboolean apply_fliprotate(fliprotate_settings, image_output);
gboolean apply_color(color_settings, image_output);
void     bimp_apply_color_reset(void);

/* FX functions — bimp-operate-fx.c */
gboolean apply_sharpblur(sharpblur_settings, image_output);
gboolean apply_watermark(watermark_settings, image_output);
gboolean apply_userdef(userdef_settings, image_output);

/* Process loop — bimp-operate-process.c */
gboolean bimp_process_image(gpointer);

/* Save dispatcher — bimp-operate-save.c */
gboolean image_save(format_type, image_output, format_params);

/* Format-specific save functions — bimp-operate-formats.c */
gboolean image_save_heif(image_output, int, gboolean);
gboolean image_save_png(image_output, gboolean, int, gboolean, gboolean,
                        gboolean, gboolean, gboolean, gboolean, gboolean);
gboolean image_save_tga(image_output, gboolean, int);
gboolean image_save_tiff(image_output, int);
gboolean image_save_webp(image_output, int, gboolean, float, float,
                         gboolean, gboolean, gboolean, int, gboolean,
                         gboolean, gboolean, int, int);
gboolean image_save_avif(image_output, gboolean, int);
gboolean image_save_exr(image_output);

#endif
