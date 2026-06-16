#ifndef GUI_CHANGEFORMAT_PRIV_H
#define GUI_CHANGEFORMAT_PRIV_H

#include <gtk/gtk.h>
#include "../bimp-manipulations.h"

/* Widget state — defined in gui-changeformat.c, used by gui-changeformat-build.c */
extern GtkWidget *frame_params;
extern GtkWidget *combo_format;
extern GtkWidget *scale_quality, *scale_alpha_quality, *scale_smoothing;
extern GtkWidget *scale_compression;
extern GtkWidget *check_interlace, *check_baseline, *check_rle;
extern GtkWidget *check_progressive, *check_entrophy, *check_lossless;
extern GtkWidget *check_savebgc, *check_savegamma, *check_saveoff;
extern GtkWidget *check_savephys, *check_savetime, *check_savecomm;
extern GtkWidget *check_savetrans;
extern GtkWidget *check_saveexif, *check_savexmp, *check_savecp;
extern GtkWidget *combo_compression, *combo_subsampling, *combo_dct;
extern GtkWidget *combo_origin, *combo_preset;
extern GtkWidget *spin_markers;
extern GtkTextBuffer *buffer_comment;
extern GtkTextIter    start_comment, end_comment;

/* Per-format inner-widget builders — defined in gui-changeformat-build.c */
void build_gif_inner   (GtkWidget *inner, changeformat_settings s);
void build_jpeg_advanced_box(GtkWidget *vbox, changeformat_settings s);
void build_jpeg_inner  (GtkWidget *inner, changeformat_settings s);
void build_png_inner   (GtkWidget *inner, changeformat_settings s);
void build_tga_inner   (GtkWidget *inner, changeformat_settings s);
void build_tiff_inner  (GtkWidget *inner, changeformat_settings s);
void build_heif_inner  (GtkWidget *inner, changeformat_settings s);
void build_webp_inner  (GtkWidget *inner, changeformat_settings s);
void build_avif_inner  (GtkWidget *inner, changeformat_settings s);

#endif
