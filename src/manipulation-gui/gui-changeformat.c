#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "gui-changeformat.h"
#include "gui-changeformat-priv.h"
#include "../bimp-manipulations.h"
#include "../plugin-intl.h"

GtkWidget *frame_params;
GtkWidget *combo_format;
GtkWidget *scale_quality, *scale_alpha_quality, *scale_smoothing;
GtkWidget *scale_compression;
GtkWidget *check_interlace, *check_baseline, *check_rle;
GtkWidget *check_progressive, *check_entrophy, *check_lossless;
GtkWidget *check_savebgc, *check_savegamma, *check_saveoff;
GtkWidget *check_savephys, *check_savetime, *check_savecomm;
GtkWidget *check_savetrans;
GtkWidget *check_saveexif, *check_savexmp, *check_savecp;
GtkWidget *combo_compression, *combo_subsampling, *combo_dct;
GtkWidget *combo_origin, *combo_preset;
GtkWidget *spin_markers;
GtkTextBuffer *buffer_comment;
GtkTextIter    start_comment, end_comment;

static void update_frame_params(GtkComboBox *, changeformat_settings);

GtkWidget *
bimp_changeformat_gui_new(changeformat_settings settings, GtkWidget *parent)
{
    GtkWidget *gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    combo_format = gtk_combo_box_text_new();
    for (int i = 0; i < FORMAT_END; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_format),
                                       format_type_string[i][1]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_format), settings->format);
    frame_params = gtk_frame_new(_("Format settings"));
    gtk_box_append(GTK_BOX(gui), combo_format);
    gtk_box_append(GTK_BOX(gui), frame_params);
    update_frame_params(GTK_COMBO_BOX(combo_format), settings);
    g_signal_connect(G_OBJECT(combo_format), "changed",
                     G_CALLBACK(update_frame_params), settings);
    return gui;
}

static void
update_frame_params(GtkComboBox *widget, changeformat_settings settings)
{
    format_type fmt  = (format_type)gtk_combo_box_get_active(widget);
    GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(inner,    8);
    gtk_widget_set_margin_bottom(inner, 8);
    gtk_widget_set_margin_start(inner,  8);
    gtk_widget_set_margin_end(inner,    8);

    if      (fmt == FORMAT_GIF)  build_gif_inner (inner, settings);
    else if (fmt == FORMAT_JPEG) build_jpeg_inner(inner, settings);
    else if (fmt == FORMAT_PNG)  build_png_inner (inner, settings);
    else if (fmt == FORMAT_TGA)  build_tga_inner (inner, settings);
    else if (fmt == FORMAT_TIFF) build_tiff_inner(inner, settings);
    else if (fmt == FORMAT_HEIF) build_heif_inner(inner, settings);
    else if (fmt == FORMAT_WEBP) build_webp_inner(inner, settings);
    else if (fmt == FORMAT_AVIF) build_avif_inner(inner, settings);
    else gtk_box_append(GTK_BOX(inner),
                        gtk_label_new(_("This format has no params")));

    gtk_frame_set_child(GTK_FRAME(frame_params), inner);
}

static void
save_jpeg_params(changeformat_settings s)
{
    s->params = (format_params_jpeg)g_malloc(sizeof(struct changeformat_params_jpeg));
    ((format_params_jpeg)s->params)->quality    = gtk_range_get_value(GTK_RANGE(scale_quality));
    ((format_params_jpeg)s->params)->smoothing  = gtk_range_get_value(GTK_RANGE(scale_smoothing));
    ((format_params_jpeg)s->params)->entropy    = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_entrophy));
    ((format_params_jpeg)s->params)->progressive = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_progressive));
    ((format_params_jpeg)s->params)->baseline   = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_baseline));
    gtk_text_buffer_get_start_iter(buffer_comment, &start_comment);
    gtk_text_buffer_get_end_iter(buffer_comment,   &end_comment);
    ((format_params_jpeg)s->params)->comment    = g_strdup(
        gtk_text_buffer_get_text(buffer_comment, &start_comment, &end_comment, TRUE));
    ((format_params_jpeg)s->params)->markers    = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_markers));
    ((format_params_jpeg)s->params)->subsampling = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_subsampling));
    ((format_params_jpeg)s->params)->dct        = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_dct));
}

static void
save_png_params(changeformat_settings s)
{
    s->params = (format_params_png)g_malloc(sizeof(struct changeformat_params_png));
    ((format_params_png)s->params)->interlace   = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_interlace));
    ((format_params_png)s->params)->compression = gtk_range_get_value(GTK_RANGE(scale_compression));
    ((format_params_png)s->params)->savebgc     = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savebgc));
    ((format_params_png)s->params)->savegamma   = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savegamma));
    ((format_params_png)s->params)->saveoff     = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_saveoff));
    ((format_params_png)s->params)->savephys    = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savephys));
    ((format_params_png)s->params)->savetime    = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savetime));
    ((format_params_png)s->params)->savecomm    = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savecomm));
    ((format_params_png)s->params)->savetrans   = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savetrans));
}

static void
save_webp_params(changeformat_settings s)
{
    s->params = (format_params_webp)g_malloc(sizeof(struct changeformat_params_webp));
    ((format_params_webp)s->params)->lossless       = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
    ((format_params_webp)s->params)->quality        = gtk_range_get_value(GTK_RANGE(scale_quality));
    ((format_params_webp)s->params)->alpha_quality  = gtk_range_get_value(GTK_RANGE(scale_alpha_quality));
    ((format_params_webp)s->params)->preset         = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_preset));
    ((format_params_webp)s->params)->exif           = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_saveexif));
    ((format_params_webp)s->params)->xmp            = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savexmp));
    ((format_params_webp)s->params)->iptc           = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savecp));
    ((format_params_webp)s->params)->animation      = FALSE;
    ((format_params_webp)s->params)->anim_loop      = TRUE;
    ((format_params_webp)s->params)->minimize_size  = TRUE;
    ((format_params_webp)s->params)->kf_distance    = 50;
    ((format_params_webp)s->params)->delay          = 200;
    ((format_params_webp)s->params)->force_delay    = FALSE;
}

void
bimp_changeformat_save(changeformat_settings s)
{
    s->format = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_format));
    g_free(s->params);

    if (s->format == FORMAT_GIF) {
        s->params = (format_params_gif)g_malloc(sizeof(struct changeformat_params_gif));
        ((format_params_gif)s->params)->interlace =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_interlace));
    } else if (s->format == FORMAT_JPEG) {
        save_jpeg_params(s);
    } else if (s->format == FORMAT_PNG) {
        save_png_params(s);
    } else if (s->format == FORMAT_TGA) {
        s->params = (format_params_tga)g_malloc(sizeof(struct changeformat_params_tga));
        ((format_params_tga)s->params)->rle    = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_rle));
        ((format_params_tga)s->params)->origin = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_origin));
    } else if (s->format == FORMAT_TIFF) {
        s->params = (format_params_tiff)g_malloc(sizeof(struct changeformat_params_tiff));
        ((format_params_tiff)s->params)->compression =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_compression));
    } else if (s->format == FORMAT_HEIF) {
        s->params = (format_params_heif)g_malloc(sizeof(struct changeformat_params_heif));
        ((format_params_heif)s->params)->lossless = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
        ((format_params_heif)s->params)->quality  = gtk_range_get_value(GTK_RANGE(scale_quality));
    } else if (s->format == FORMAT_WEBP) {
        save_webp_params(s);
    } else if (s->format == FORMAT_AVIF) {
        s->params = (format_params_avif)g_malloc(sizeof(struct changeformat_params_avif));
        ((format_params_avif)s->params)->lossless = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
        ((format_params_avif)s->params)->quality  = gtk_range_get_value(GTK_RANGE(scale_quality));
    } else {
        s->params = NULL;
    }
}
