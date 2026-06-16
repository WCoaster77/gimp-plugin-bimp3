#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "gui-changeformat.h"
#include "../bimp-manipulations.h"
#include "../plugin-intl.h"

static void update_frame_params(GtkComboBox *, changeformat_settings);

static GtkWidget *frame_params;
static GtkWidget *combo_format, *scale_quality, *scale_alpha_quality;
static GtkWidget *scale_smoothing, *check_interlace, *scale_compression;
static GtkWidget *check_baseline, *check_rle, *check_progressive;
static GtkWidget *check_entrophy, *combo_compression, *spin_markers;
static GtkWidget *combo_subsampling, *combo_dct, *combo_origin;
static GtkWidget *check_savebgc, *check_savegamma, *check_saveoff;
static GtkWidget *check_savephys, *check_savetime, *check_savecomm;
static GtkWidget *check_savetrans, *check_lossless;
static GtkWidget *combo_preset, *check_saveexif, *check_savexmp, *check_savecp;

static GtkTextBuffer *buffer_comment;
static GtkTextIter   start_comment, end_comment;

GtkWidget *
bimp_changeformat_gui_new(changeformat_settings settings, GtkWidget *parent)
{
    GtkWidget *gui;

    gui = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

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
    GtkWidget *inner_widget;
    format_type selected_format =
        (format_type)gtk_combo_box_get_active(widget);

    inner_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(inner_widget,    8);
    gtk_widget_set_margin_bottom(inner_widget, 8);
    gtk_widget_set_margin_start(inner_widget,  8);
    gtk_widget_set_margin_end(inner_widget,    8);

    if (selected_format == FORMAT_GIF) {
        check_interlace = gtk_check_button_new_with_label(_("Interlaced"));
        if (selected_format == settings->format) {
            format_params_gif sg = (format_params_gif)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace),
                                        sg->interlace);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace), FALSE);
        }
        gtk_box_append(GTK_BOX(inner_widget), check_interlace);

    } else if (selected_format == FORMAT_JPEG) {
        GtkWidget *hbox_quality, *hbox_smoothing, *hbox_checks;
        GtkWidget *hbox_comment, *hbox_markers, *hbox_subsampling, *hbox_dct;
        GtkWidget *vbox_advanced, *expander_advanced;
        GtkWidget *label_quality, *label_smoothing, *label_markers;
        GtkWidget *label_comment, *label_subsampling, *label_dct, *text_comment;

        hbox_quality   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_quality  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
        scale_quality  = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                   0, 100, 1);
        gtk_widget_set_hexpand(scale_quality, TRUE);

        expander_advanced = gtk_expander_new(_("Advanced params"));
        vbox_advanced  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        hbox_smoothing = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_smoothing = gtk_label_new(g_strconcat(_("Smoothing"), ":", NULL));
        scale_smoothing = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                    0, 1, 0.01);
        gtk_widget_set_hexpand(scale_smoothing, TRUE);

        hbox_checks    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        check_entrophy  = gtk_check_button_new_with_label(_("Optimize"));
        check_progressive = gtk_check_button_new_with_label(_("Progressive"));
        check_baseline = gtk_check_button_new_with_label(_("Save baseline"));

        hbox_comment   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_comment  = gtk_label_new(g_strconcat(_("Comment"), ":", NULL));
        text_comment   = gtk_text_view_new();
        gtk_widget_set_hexpand(text_comment, TRUE);
        buffer_comment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_comment));

        hbox_markers   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_markers  = gtk_label_new(g_strconcat(_("Markers rows"), ":", NULL));
        spin_markers   = gtk_spin_button_new(NULL, 1, 0);

        hbox_subsampling = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_subsampling = gtk_label_new(g_strconcat(_("Subsampling"), ":", NULL));
        combo_subsampling = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
            g_strconcat("2x2, 1x1, 1x1 (", _("Small size"), ")", NULL));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
            "2x1, 1x1, 1x1 (4:2:2)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
            g_strconcat("1x1, 1x1, 1x1 (", _("Quality"), ")", NULL));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
            "1x2, 1x1, 1x1");

        hbox_dct  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_dct = gtk_label_new(g_strconcat(_("DCT algorithm"), ":", NULL));
        combo_dct = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Integer"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Fast integer"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Float"));

        if (selected_format == settings->format) {
            format_params_jpeg sj = (format_params_jpeg)(settings->params);
            gtk_range_set_value(GTK_RANGE(scale_quality),   sj->quality);
            gtk_range_set_value(GTK_RANGE(scale_smoothing), sj->smoothing);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_entrophy),  sj->entropy);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_progressive), sj->progressive);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_baseline),  sj->baseline);
            gtk_spin_button_configure(GTK_SPIN_BUTTON(spin_markers),
                GTK_ADJUSTMENT(gtk_adjustment_new(sj->markers, 0, 64, 1, 1, 0)), 0, 0);
            buffer_comment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_comment));
            gtk_text_buffer_set_text(buffer_comment, sj->comment, -1);
            gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_comment), buffer_comment);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_subsampling), sj->subsampling);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_dct),         sj->dct);
        } else {
            gtk_range_set_value(GTK_RANGE(scale_quality),   85.0);
            gtk_range_set_value(GTK_RANGE(scale_smoothing), 0.0);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_entrophy),    TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_progressive), FALSE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_baseline),    FALSE);
            gtk_spin_button_configure(GTK_SPIN_BUTTON(spin_markers),
                GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 64, 1, 1, 0)), 0, 0);
            gtk_text_buffer_set_text(buffer_comment, "", -1);
            gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_comment), buffer_comment);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_subsampling), 2);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_dct),         1);
        }

        gtk_box_append(GTK_BOX(hbox_quality),   label_quality);
        gtk_box_append(GTK_BOX(hbox_quality),   scale_quality);
        gtk_box_append(GTK_BOX(inner_widget),   hbox_quality);
        gtk_box_append(GTK_BOX(inner_widget),   expander_advanced);

        gtk_box_append(GTK_BOX(hbox_smoothing), label_smoothing);
        gtk_box_append(GTK_BOX(hbox_smoothing), scale_smoothing);
        gtk_box_append(GTK_BOX(vbox_advanced),  hbox_smoothing);

        gtk_box_append(GTK_BOX(hbox_checks), check_entrophy);
        gtk_box_append(GTK_BOX(hbox_checks), check_progressive);
        gtk_box_append(GTK_BOX(hbox_checks), check_baseline);
        gtk_box_append(GTK_BOX(vbox_advanced), hbox_checks);

        gtk_box_append(GTK_BOX(hbox_comment),  label_comment);
        gtk_box_append(GTK_BOX(hbox_comment),  text_comment);
        gtk_box_append(GTK_BOX(vbox_advanced), hbox_comment);

        gtk_box_append(GTK_BOX(hbox_markers),  label_markers);
        gtk_box_append(GTK_BOX(hbox_markers),  spin_markers);
        gtk_box_append(GTK_BOX(vbox_advanced), hbox_markers);

        gtk_box_append(GTK_BOX(hbox_subsampling), label_subsampling);
        gtk_box_append(GTK_BOX(hbox_subsampling), combo_subsampling);
        gtk_box_append(GTK_BOX(vbox_advanced),    hbox_subsampling);

        gtk_box_append(GTK_BOX(hbox_dct),     label_dct);
        gtk_box_append(GTK_BOX(hbox_dct),     combo_dct);
        gtk_box_append(GTK_BOX(vbox_advanced), hbox_dct);

        gtk_expander_set_child(GTK_EXPANDER(expander_advanced), vbox_advanced);

    } else if (selected_format == FORMAT_PNG) {
        GtkWidget *hbox_compression, *label_compression;
        GtkWidget *vbox_advanced, *expander_advanced;

        check_interlace = gtk_check_button_new_with_label(_("Interlace (Adam7)"));
        hbox_compression = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_compression = gtk_label_new(g_strconcat(_("Compression"), ":", NULL));
        scale_compression = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                      0, 9, 1);
        gtk_widget_set_hexpand(scale_compression, TRUE);

        expander_advanced = gtk_expander_new(_("Advanced params"));
        vbox_advanced = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        check_savebgc  = gtk_check_button_new_with_label(_("Save background color"));
        check_savegamma = gtk_check_button_new_with_label(_("Save gamma"));
        check_saveoff  = gtk_check_button_new_with_label(_("Save layer offset"));
        check_savephys = gtk_check_button_new_with_label(_("Save resolution"));
        check_savetime = gtk_check_button_new_with_label(_("Save creation date"));
        check_savecomm = gtk_check_button_new_with_label(_("Save comments"));
        check_savetrans = gtk_check_button_new_with_label(
            _("Save color from transparent pixels"));

        if (selected_format == settings->format) {
            format_params_png sp = (format_params_png)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace),  sp->interlace);
            gtk_range_set_value(GTK_RANGE(scale_compression),               sp->compression);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savebgc),   sp->savebgc);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savegamma), sp->savegamma);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveoff),   sp->saveoff);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savephys),  sp->savephys);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetime),  sp->savetime);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecomm),  sp->savecomm);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetrans), sp->savetrans);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace),  FALSE);
            gtk_range_set_value(GTK_RANGE(scale_compression),               9);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savebgc),   TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savegamma), FALSE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveoff),   FALSE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savephys),  TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetime),  TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecomm),  TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetrans), TRUE);
        }

        gtk_box_append(GTK_BOX(vbox_advanced), check_savebgc);
        gtk_box_append(GTK_BOX(vbox_advanced), check_savegamma);
        gtk_box_append(GTK_BOX(vbox_advanced), check_saveoff);
        gtk_box_append(GTK_BOX(vbox_advanced), check_savephys);
        gtk_box_append(GTK_BOX(vbox_advanced), check_savetime);
        gtk_box_append(GTK_BOX(vbox_advanced), check_savecomm);
        gtk_box_append(GTK_BOX(vbox_advanced), check_savetrans);

        gtk_box_append(GTK_BOX(inner_widget),    check_interlace);
        gtk_box_append(GTK_BOX(hbox_compression), label_compression);
        gtk_box_append(GTK_BOX(hbox_compression), scale_compression);
        gtk_box_append(GTK_BOX(inner_widget),    hbox_compression);
        gtk_box_append(GTK_BOX(inner_widget),    expander_advanced);

        gtk_expander_set_child(GTK_EXPANDER(expander_advanced), vbox_advanced);

    } else if (selected_format == FORMAT_TGA) {
        GtkWidget *hbox_origin, *label_origin;

        check_rle   = gtk_check_button_new_with_label(_("RLE compression"));
        hbox_origin = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_origin = gtk_label_new(g_strconcat(_("Image origin"), ":", NULL));

        combo_origin = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_origin), _("Top-left"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_origin), _("Bottom-left"));

        if (selected_format == settings->format) {
            format_params_tga st = (format_params_tga)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_rle), st->rle);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_origin),    st->origin);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_rle), FALSE);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_origin),    0);
        }

        gtk_box_append(GTK_BOX(hbox_origin),   label_origin);
        gtk_box_append(GTK_BOX(hbox_origin),   combo_origin);
        gtk_box_append(GTK_BOX(inner_widget),  check_rle);
        gtk_box_append(GTK_BOX(inner_widget),  hbox_origin);

    } else if (selected_format == FORMAT_TIFF) {
        GtkWidget *hbox_compression, *label_compression;

        hbox_compression  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_compression = gtk_label_new(g_strconcat(_("Compression"), ":", NULL));
        combo_compression = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("None"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("LZW"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("Pack bits"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("Deflate"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("JPEG"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("CCITT G3 Fax"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("CCITT G4 Fax"));

        if (selected_format == settings->format) {
            format_params_tiff st = (format_params_tiff)(settings->params);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_compression), st->compression);
        } else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_compression), 0);
        }

        gtk_widget_set_hexpand(combo_compression, TRUE);
        gtk_box_append(GTK_BOX(hbox_compression), label_compression);
        gtk_box_append(GTK_BOX(hbox_compression), combo_compression);
        gtk_box_append(GTK_BOX(inner_widget),     hbox_compression);

    } else if (selected_format == FORMAT_HEIF) {
        GtkWidget *hbox_quality, *label_quality;

        check_lossless = gtk_check_button_new_with_label(_("Lossless"));
        hbox_quality   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_quality  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
        scale_quality  = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                   0, 100, 1);
        gtk_widget_set_hexpand(scale_quality, TRUE);

        if (selected_format == settings->format) {
            format_params_heif sh = (format_params_heif)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), sh->lossless);
            gtk_range_set_value(GTK_RANGE(scale_quality), sh->quality);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), FALSE);
            gtk_range_set_value(GTK_RANGE(scale_quality), 85.0);
        }

        gtk_box_append(GTK_BOX(inner_widget), check_lossless);
        gtk_box_append(GTK_BOX(hbox_quality), label_quality);
        gtk_box_append(GTK_BOX(hbox_quality), scale_quality);
        gtk_box_append(GTK_BOX(inner_widget), hbox_quality);

    } else if (selected_format == FORMAT_WEBP) {
        GtkWidget *hbox_quality, *hbox_alpha_quality, *hbox_preset;
        GtkWidget *label_quality, *label_alpha_quality, *label_preset;

        check_lossless = gtk_check_button_new_with_label(_("Lossless"));

        hbox_quality      = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_quality     = gtk_label_new(g_strconcat(_("Image quality"), ":", NULL));
        scale_quality     = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                      0, 100, 1);
        gtk_widget_set_hexpand(scale_quality, TRUE);

        hbox_alpha_quality = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_alpha_quality = gtk_label_new(g_strconcat(_("Alpha quality"), ":", NULL));
        scale_alpha_quality = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                        0, 100, 1);
        gtk_widget_set_hexpand(scale_alpha_quality, TRUE);

        hbox_preset   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_preset  = gtk_label_new(g_strconcat(_("Preset"), ":", NULL));
        combo_preset  = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Default"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Picture"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Photo"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Drawing"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Icon"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Text"));
        gtk_widget_set_hexpand(combo_preset, TRUE);

        check_saveexif = gtk_check_button_new_with_label(_("Save EXIF data"));
        check_savexmp  = gtk_check_button_new_with_label(_("Save XMP data"));
        check_savecp   = gtk_check_button_new_with_label(_("Save color profile"));

        if (selected_format == settings->format) {
            format_params_webp sw = (format_params_webp)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), sw->lossless);
            gtk_range_set_value(GTK_RANGE(scale_quality),       sw->quality);
            gtk_range_set_value(GTK_RANGE(scale_alpha_quality), sw->alpha_quality);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_preset),           sw->preset);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveexif), sw->exif);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savexmp),  sw->xmp);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecp),   sw->iptc);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), FALSE);
            gtk_range_set_value(GTK_RANGE(scale_quality),        90.0);
            gtk_range_set_value(GTK_RANGE(scale_alpha_quality),  100.0);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_preset), 0);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveexif), TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savexmp),  TRUE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecp),   TRUE);
        }

        gtk_box_append(GTK_BOX(inner_widget),       check_lossless);
        gtk_box_append(GTK_BOX(hbox_quality),       label_quality);
        gtk_box_append(GTK_BOX(hbox_quality),       scale_quality);
        gtk_box_append(GTK_BOX(inner_widget),       hbox_quality);
        gtk_box_append(GTK_BOX(hbox_alpha_quality), label_alpha_quality);
        gtk_box_append(GTK_BOX(hbox_alpha_quality), scale_alpha_quality);
        gtk_box_append(GTK_BOX(inner_widget),       hbox_alpha_quality);
        gtk_box_append(GTK_BOX(hbox_preset),        label_preset);
        gtk_box_append(GTK_BOX(hbox_preset),        combo_preset);
        gtk_box_append(GTK_BOX(inner_widget),       hbox_preset);
        gtk_box_append(GTK_BOX(inner_widget),       check_saveexif);
        gtk_box_append(GTK_BOX(inner_widget),       check_savexmp);
        gtk_box_append(GTK_BOX(inner_widget),       check_savecp);

    } else if (selected_format == FORMAT_AVIF) {
        GtkWidget *hbox_quality, *label_quality;

        check_lossless = gtk_check_button_new_with_label(_("Nearly lossless"));
        hbox_quality   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label_quality  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
        scale_quality  = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                                   0, 100, 1);
        gtk_widget_set_hexpand(scale_quality, TRUE);

        if (selected_format == settings->format) {
            format_params_avif sa = (format_params_avif)(settings->params);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), sa->lossless);
            gtk_range_set_value(GTK_RANGE(scale_quality), sa->quality);
        } else {
            gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless), FALSE);
            gtk_range_set_value(GTK_RANGE(scale_quality), 50);
        }

        gtk_box_append(GTK_BOX(inner_widget), check_lossless);
        gtk_box_append(GTK_BOX(hbox_quality), label_quality);
        gtk_box_append(GTK_BOX(hbox_quality), scale_quality);
        gtk_box_append(GTK_BOX(inner_widget), hbox_quality);

    } else {
        GtkWidget *label_no_param = gtk_label_new(_("This format has no params"));
        gtk_box_append(GTK_BOX(inner_widget), label_no_param);
    }

    gtk_frame_set_child(GTK_FRAME(frame_params), inner_widget);
}

void
bimp_changeformat_save(changeformat_settings orig_settings)
{
    orig_settings->format = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_format));
    g_free(orig_settings->params);

    if (orig_settings->format == FORMAT_GIF) {
        orig_settings->params = (format_params_gif)
            g_malloc(sizeof(struct changeformat_params_gif));
        ((format_params_gif)orig_settings->params)->interlace =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_interlace));

    } else if (orig_settings->format == FORMAT_JPEG) {
        orig_settings->params = (format_params_jpeg)
            g_malloc(sizeof(struct changeformat_params_jpeg));
        ((format_params_jpeg)orig_settings->params)->quality =
            gtk_range_get_value(GTK_RANGE(scale_quality));
        ((format_params_jpeg)orig_settings->params)->smoothing =
            gtk_range_get_value(GTK_RANGE(scale_smoothing));
        ((format_params_jpeg)orig_settings->params)->entropy =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_entrophy));
        ((format_params_jpeg)orig_settings->params)->progressive =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_progressive));
        ((format_params_jpeg)orig_settings->params)->baseline =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_baseline));
        gtk_text_buffer_get_start_iter(buffer_comment, &start_comment);
        gtk_text_buffer_get_end_iter(buffer_comment,   &end_comment);
        ((format_params_jpeg)orig_settings->params)->comment = g_strdup(
            gtk_text_buffer_get_text(buffer_comment,
                                     &start_comment, &end_comment, TRUE));
        ((format_params_jpeg)orig_settings->params)->markers =
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_markers));
        ((format_params_jpeg)orig_settings->params)->subsampling =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_subsampling));
        ((format_params_jpeg)orig_settings->params)->dct =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_dct));

    } else if (orig_settings->format == FORMAT_PNG) {
        orig_settings->params = (format_params_png)
            g_malloc(sizeof(struct changeformat_params_png));
        ((format_params_png)orig_settings->params)->interlace =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_interlace));
        ((format_params_png)orig_settings->params)->compression =
            gtk_range_get_value(GTK_RANGE(scale_compression));
        ((format_params_png)orig_settings->params)->savebgc =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savebgc));
        ((format_params_png)orig_settings->params)->savegamma =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savegamma));
        ((format_params_png)orig_settings->params)->saveoff =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_saveoff));
        ((format_params_png)orig_settings->params)->savephys =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savephys));
        ((format_params_png)orig_settings->params)->savetime =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savetime));
        ((format_params_png)orig_settings->params)->savecomm =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savecomm));
        ((format_params_png)orig_settings->params)->savetrans =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savetrans));

    } else if (orig_settings->format == FORMAT_TGA) {
        orig_settings->params = (format_params_tga)
            g_malloc(sizeof(struct changeformat_params_tga));
        ((format_params_tga)orig_settings->params)->rle =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_rle));
        ((format_params_tga)orig_settings->params)->origin =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_origin));

    } else if (orig_settings->format == FORMAT_TIFF) {
        orig_settings->params = (format_params_tiff)
            g_malloc(sizeof(struct changeformat_params_tiff));
        ((format_params_tiff)orig_settings->params)->compression =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_compression));

    } else if (orig_settings->format == FORMAT_HEIF) {
        orig_settings->params = (format_params_heif)
            g_malloc(sizeof(struct changeformat_params_heif));
        ((format_params_heif)orig_settings->params)->lossless =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
        ((format_params_heif)orig_settings->params)->quality =
            gtk_range_get_value(GTK_RANGE(scale_quality));

    } else if (orig_settings->format == FORMAT_WEBP) {
        orig_settings->params = (format_params_webp)
            g_malloc(sizeof(struct changeformat_params_webp));
        ((format_params_webp)orig_settings->params)->lossless =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
        ((format_params_webp)orig_settings->params)->quality =
            gtk_range_get_value(GTK_RANGE(scale_quality));
        ((format_params_webp)orig_settings->params)->alpha_quality =
            gtk_range_get_value(GTK_RANGE(scale_alpha_quality));
        ((format_params_webp)orig_settings->params)->preset =
            gtk_combo_box_get_active(GTK_COMBO_BOX(combo_preset));
        ((format_params_webp)orig_settings->params)->exif =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_saveexif));
        ((format_params_webp)orig_settings->params)->xmp =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savexmp));
        ((format_params_webp)orig_settings->params)->iptc =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_savecp));
        ((format_params_webp)orig_settings->params)->animation    = FALSE;
        ((format_params_webp)orig_settings->params)->anim_loop    = TRUE;
        ((format_params_webp)orig_settings->params)->minimize_size = TRUE;
        ((format_params_webp)orig_settings->params)->kf_distance  = 50;
        ((format_params_webp)orig_settings->params)->delay         = 200;
        ((format_params_webp)orig_settings->params)->force_delay   = FALSE;

    } else if (orig_settings->format == FORMAT_AVIF) {
        orig_settings->params = (format_params_avif)
            g_malloc(sizeof(struct changeformat_params_avif));
        ((format_params_avif)orig_settings->params)->lossless =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(check_lossless));
        ((format_params_avif)orig_settings->params)->quality =
            gtk_range_get_value(GTK_RANGE(scale_quality));

    } else {
        orig_settings->params = NULL;
    }
}
