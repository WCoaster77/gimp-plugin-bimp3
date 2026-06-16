#include <gtk/gtk.h>
#include "gui-changeformat-priv.h"
#include "../plugin-intl.h"

void
build_gif_inner(GtkWidget *inner, changeformat_settings s)
{
    check_interlace = gtk_check_button_new_with_label(_("Interlaced"));
    gboolean v = (s->format == FORMAT_GIF)
        ? ((format_params_gif)s->params)->interlace : FALSE;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace), v);
    gtk_box_append(GTK_BOX(inner), check_interlace);
}

void
build_jpeg_advanced_box(GtkWidget *vbox, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_JPEG);
    format_params_jpeg sj = restore ? (format_params_jpeg)s->params : NULL;

    GtkWidget *hbox_s = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_s  = gtk_label_new(g_strconcat(_("Smoothing"), ":", NULL));
    scale_smoothing   = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
    gtk_widget_set_hexpand(scale_smoothing, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_smoothing), restore ? sj->smoothing : 0.0);
    gtk_box_append(GTK_BOX(hbox_s), lbl_s);
    gtk_box_append(GTK_BOX(hbox_s), scale_smoothing);

    GtkWidget *hbox_c   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    check_entrophy      = gtk_check_button_new_with_label(_("Optimize"));
    check_progressive   = gtk_check_button_new_with_label(_("Progressive"));
    check_baseline      = gtk_check_button_new_with_label(_("Save baseline"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_entrophy),    restore ? sj->entropy     : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_progressive), restore ? sj->progressive : FALSE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_baseline),    restore ? sj->baseline    : FALSE);
    gtk_box_append(GTK_BOX(hbox_c), check_entrophy);
    gtk_box_append(GTK_BOX(hbox_c), check_progressive);
    gtk_box_append(GTK_BOX(hbox_c), check_baseline);

    GtkWidget *hbox_cm = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_cm  = gtk_label_new(g_strconcat(_("Comment"), ":", NULL));
    GtkWidget *txt_cm  = gtk_text_view_new();
    gtk_widget_set_hexpand(txt_cm, TRUE);
    buffer_comment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_cm));
    gtk_text_buffer_set_text(buffer_comment, restore ? sj->comment : "", -1);
    gtk_box_append(GTK_BOX(hbox_cm), lbl_cm);
    gtk_box_append(GTK_BOX(hbox_cm), txt_cm);

    GtkWidget *hbox_m = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_m  = gtk_label_new(g_strconcat(_("Markers rows"), ":", NULL));
    spin_markers = gtk_spin_button_new(NULL, 1, 0);
    gtk_spin_button_configure(GTK_SPIN_BUTTON(spin_markers),
        GTK_ADJUSTMENT(gtk_adjustment_new(restore ? sj->markers : 0, 0, 64, 1, 1, 0)), 0, 0);
    gtk_box_append(GTK_BOX(hbox_m), lbl_m);
    gtk_box_append(GTK_BOX(hbox_m), spin_markers);

    GtkWidget *hbox_ss = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_ss  = gtk_label_new(g_strconcat(_("Subsampling"), ":", NULL));
    combo_subsampling  = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
        g_strconcat("2x2, 1x1, 1x1 (", _("Small size"), ")", NULL));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling), "2x1, 1x1, 1x1 (4:2:2)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling),
        g_strconcat("1x1, 1x1, 1x1 (", _("Quality"), ")", NULL));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_subsampling), "1x2, 1x1, 1x1");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_subsampling), restore ? sj->subsampling : 2);
    gtk_box_append(GTK_BOX(hbox_ss), lbl_ss);
    gtk_box_append(GTK_BOX(hbox_ss), combo_subsampling);

    GtkWidget *hbox_d = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_d  = gtk_label_new(g_strconcat(_("DCT algorithm"), ":", NULL));
    combo_dct         = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Integer"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Fast integer"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_dct), _("Float"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_dct), restore ? sj->dct : 1);
    gtk_box_append(GTK_BOX(hbox_d), lbl_d);
    gtk_box_append(GTK_BOX(hbox_d), combo_dct);

    gtk_box_append(GTK_BOX(vbox), hbox_s);
    gtk_box_append(GTK_BOX(vbox), hbox_c);
    gtk_box_append(GTK_BOX(vbox), hbox_cm);
    gtk_box_append(GTK_BOX(vbox), hbox_m);
    gtk_box_append(GTK_BOX(vbox), hbox_ss);
    gtk_box_append(GTK_BOX(vbox), hbox_d);
}

void
build_jpeg_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_JPEG);
    format_params_jpeg sj = restore ? (format_params_jpeg)s->params : NULL;

    GtkWidget *hbox_q = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_q  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
    scale_quality     = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(scale_quality, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_quality), restore ? sj->quality : 85.0);
    gtk_box_append(GTK_BOX(hbox_q), lbl_q);
    gtk_box_append(GTK_BOX(hbox_q), scale_quality);

    GtkWidget *expander = gtk_expander_new(_("Advanced params"));
    GtkWidget *vbox_adv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    build_jpeg_advanced_box(vbox_adv, s);
    gtk_expander_set_child(GTK_EXPANDER(expander), vbox_adv);

    gtk_box_append(GTK_BOX(inner), hbox_q);
    gtk_box_append(GTK_BOX(inner), expander);
}

void
build_png_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_PNG);
    format_params_png sp = restore ? (format_params_png)s->params : NULL;

    check_interlace = gtk_check_button_new_with_label(_("Interlace (Adam7)"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_interlace),
                                restore ? sp->interlace : FALSE);

    GtkWidget *hbox_cmp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_cmp  = gtk_label_new(g_strconcat(_("Compression"), ":", NULL));
    scale_compression   = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 9, 1);
    gtk_widget_set_hexpand(scale_compression, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_compression), restore ? sp->compression : 9);
    gtk_box_append(GTK_BOX(hbox_cmp), lbl_cmp);
    gtk_box_append(GTK_BOX(hbox_cmp), scale_compression);

    GtkWidget *vbox_adv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    check_savebgc  = gtk_check_button_new_with_label(_("Save background color"));
    check_savegamma = gtk_check_button_new_with_label(_("Save gamma"));
    check_saveoff  = gtk_check_button_new_with_label(_("Save layer offset"));
    check_savephys = gtk_check_button_new_with_label(_("Save resolution"));
    check_savetime = gtk_check_button_new_with_label(_("Save creation date"));
    check_savecomm = gtk_check_button_new_with_label(_("Save comments"));
    check_savetrans = gtk_check_button_new_with_label(_("Save color from transparent pixels"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savebgc),   restore ? sp->savebgc   : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savegamma), restore ? sp->savegamma : FALSE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveoff),   restore ? sp->saveoff   : FALSE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savephys),  restore ? sp->savephys  : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetime),  restore ? sp->savetime  : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecomm),  restore ? sp->savecomm  : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savetrans), restore ? sp->savetrans : TRUE);
    gtk_box_append(GTK_BOX(vbox_adv), check_savebgc);
    gtk_box_append(GTK_BOX(vbox_adv), check_savegamma);
    gtk_box_append(GTK_BOX(vbox_adv), check_saveoff);
    gtk_box_append(GTK_BOX(vbox_adv), check_savephys);
    gtk_box_append(GTK_BOX(vbox_adv), check_savetime);
    gtk_box_append(GTK_BOX(vbox_adv), check_savecomm);
    gtk_box_append(GTK_BOX(vbox_adv), check_savetrans);

    GtkWidget *expander = gtk_expander_new(_("Advanced params"));
    gtk_expander_set_child(GTK_EXPANDER(expander), vbox_adv);
    gtk_box_append(GTK_BOX(inner), check_interlace);
    gtk_box_append(GTK_BOX(inner), hbox_cmp);
    gtk_box_append(GTK_BOX(inner), expander);
}

void
build_tga_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_TGA);
    format_params_tga st = restore ? (format_params_tga)s->params : NULL;

    check_rle = gtk_check_button_new_with_label(_("RLE compression"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_rle), restore ? st->rle : FALSE);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl  = gtk_label_new(g_strconcat(_("Image origin"), ":", NULL));
    combo_origin    = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_origin), _("Top-left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_origin), _("Bottom-left"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_origin), restore ? st->origin : 0);
    gtk_box_append(GTK_BOX(hbox), lbl);
    gtk_box_append(GTK_BOX(hbox), combo_origin);
    gtk_box_append(GTK_BOX(inner), check_rle);
    gtk_box_append(GTK_BOX(inner), hbox);
}

void
build_tiff_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_TIFF);
    format_params_tiff st = restore ? (format_params_tiff)s->params : NULL;

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl  = gtk_label_new(g_strconcat(_("Compression"), ":", NULL));
    combo_compression = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("LZW"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("Pack bits"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("Deflate"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("JPEG"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("CCITT G3 Fax"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_compression), _("CCITT G4 Fax"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_compression), restore ? st->compression : 0);
    gtk_widget_set_hexpand(combo_compression, TRUE);
    gtk_box_append(GTK_BOX(hbox), lbl);
    gtk_box_append(GTK_BOX(hbox), combo_compression);
    gtk_box_append(GTK_BOX(inner), hbox);
}

void
build_heif_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_HEIF);
    format_params_heif sh = restore ? (format_params_heif)s->params : NULL;

    check_lossless = gtk_check_button_new_with_label(_("Lossless"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless),
                                restore ? sh->lossless : FALSE);
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
    scale_quality   = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(scale_quality, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_quality), restore ? sh->quality : 85.0);
    gtk_box_append(GTK_BOX(hbox), lbl);
    gtk_box_append(GTK_BOX(hbox), scale_quality);
    gtk_box_append(GTK_BOX(inner), check_lossless);
    gtk_box_append(GTK_BOX(inner), hbox);
}

void
build_webp_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_WEBP);
    format_params_webp sw = restore ? (format_params_webp)s->params : NULL;

    check_lossless = gtk_check_button_new_with_label(_("Lossless"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless),
                                restore ? sw->lossless : FALSE);

    GtkWidget *hbox_q = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    scale_quality     = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(scale_quality, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_quality), restore ? sw->quality : 90.0);
    gtk_box_append(GTK_BOX(hbox_q), gtk_label_new(g_strconcat(_("Image quality"), ":", NULL)));
    gtk_box_append(GTK_BOX(hbox_q), scale_quality);

    GtkWidget *hbox_aq = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    scale_alpha_quality = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(scale_alpha_quality, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_alpha_quality), restore ? sw->alpha_quality : 100.0);
    gtk_box_append(GTK_BOX(hbox_aq), gtk_label_new(g_strconcat(_("Alpha quality"), ":", NULL)));
    gtk_box_append(GTK_BOX(hbox_aq), scale_alpha_quality);

    GtkWidget *hbox_p = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    combo_preset = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Default"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Picture"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Photo"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Drawing"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Icon"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_preset), _("Text"));
    gtk_widget_set_hexpand(combo_preset, TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_preset), restore ? sw->preset : 0);
    gtk_box_append(GTK_BOX(hbox_p), gtk_label_new(g_strconcat(_("Preset"), ":", NULL)));
    gtk_box_append(GTK_BOX(hbox_p), combo_preset);

    check_saveexif = gtk_check_button_new_with_label(_("Save EXIF data"));
    check_savexmp  = gtk_check_button_new_with_label(_("Save XMP data"));
    check_savecp   = gtk_check_button_new_with_label(_("Save color profile"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_saveexif), restore ? sw->exif : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savexmp),  restore ? sw->xmp  : TRUE);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_savecp),   restore ? sw->iptc : TRUE);

    gtk_box_append(GTK_BOX(inner), check_lossless);
    gtk_box_append(GTK_BOX(inner), hbox_q);
    gtk_box_append(GTK_BOX(inner), hbox_aq);
    gtk_box_append(GTK_BOX(inner), hbox_p);
    gtk_box_append(GTK_BOX(inner), check_saveexif);
    gtk_box_append(GTK_BOX(inner), check_savexmp);
    gtk_box_append(GTK_BOX(inner), check_savecp);
}

void
build_avif_inner(GtkWidget *inner, changeformat_settings s)
{
    gboolean restore = (s->format == FORMAT_AVIF);
    format_params_avif sa = restore ? (format_params_avif)s->params : NULL;

    check_lossless = gtk_check_button_new_with_label(_("Nearly lossless"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_lossless),
                                restore ? sa->lossless : FALSE);
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl  = gtk_label_new(g_strconcat(_("Quality"), ":", NULL));
    scale_quality   = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(scale_quality, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_quality), restore ? sa->quality : 50);
    gtk_box_append(GTK_BOX(hbox), lbl);
    gtk_box_append(GTK_BOX(hbox), scale_quality);
    gtk_box_append(GTK_BOX(inner), check_lossless);
    gtk_box_append(GTK_BOX(inner), hbox);
}
