#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gui-watermark.h"
#include "../bimp-manipulations.h"
#include "../bimp-manipulations-gui.h"
#include "../bimp-utils.h"
#include "../plugin-intl.h"

static void toggle_group(GtkCheckButton *, gpointer);
static void toggle_image_size(GtkCheckButton *, gpointer);
static void position_btn_toggled(GtkToggleButton *, gpointer);
static const char *watermark_pos_get_string(watermark_position);
static const char *watermark_pos_get_abbreviation(watermark_position);
static void file_filters_add_patterns(GtkFileFilter *, GtkFileFilter *, ...);

static GtkWidget *grid_text, *vbox_image, *hbox_image_size;
static GtkWidget *radio_text, *radio_image;
static GtkWidget *entry_text;
static GtkWidget *chooser_font, *chooser_color, *chooser_image;
static GtkWidget *check_image_adaptsize;
static GtkWidget *spin_image_sizepercent, *spin_edge, *combo_image_sizemode;
static GtkWidget *scale_opacity;
static GtkWidget *position_buttons[9];

GtkWidget *
bimp_watermark_gui_new(watermark_settings settings)
{
    GtkWidget *gui, *hbox_mode, *vbox_mode_text, *vbox_mode_image, *vbox_position;
    GtkWidget *hbox_opacity, *hbox_edge;
    GtkWidget *frame_position, *grid_position;
    GtkWidget *box_text_indent, *box_img_indent;
    GtkWidget *sep_v, *sep_h;
    GtkWidget *label_text, *label_font, *label_color;
    GtkWidget *label_opacity, *label_percent, *label_edge, *label_percentof, *label_px;

    gui           = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    hbox_mode     = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    vbox_mode_text  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    sep_v         = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    vbox_mode_image = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    /* --- Text mode --- */
    radio_text = gtk_check_button_new_with_label(_("Text watermark"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_text), settings->mode);

    grid_text = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_text), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid_text), 5);

    label_text = gtk_label_new(g_strconcat(_("Text"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_text), 0.0f);
    entry_text = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_text), 50);
    gtk_editable_set_text(GTK_EDITABLE(entry_text), settings->text);
    gtk_widget_set_hexpand(entry_text, TRUE);

    label_font = gtk_label_new(g_strconcat(_("Font"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_font), 0.0f);
    chooser_font = gtk_font_button_new_with_font(
        pango_font_description_to_string(settings->font));
    gtk_widget_set_hexpand(chooser_font, TRUE);

    label_color = gtk_label_new(g_strconcat(_("Color"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_color), 0.0f);
    chooser_color = gtk_color_button_new_with_rgba(&settings->color);
    gtk_widget_set_hexpand(chooser_color, TRUE);

    gtk_grid_attach(GTK_GRID(grid_text), label_text,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_text), entry_text,   1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_text), label_font,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_text), chooser_font, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_text), label_color,  0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_text), chooser_color,1, 2, 1, 1);

    box_text_indent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(box_text_indent, 20);
    gtk_box_append(GTK_BOX(box_text_indent), grid_text);
    gtk_widget_set_hexpand(grid_text, TRUE);

    gtk_box_append(GTK_BOX(vbox_mode_text), radio_text);
    gtk_box_append(GTK_BOX(vbox_mode_text), box_text_indent);
    gtk_widget_set_hexpand(vbox_mode_text, TRUE);

    /* --- Image mode --- */
    radio_image = gtk_check_button_new_with_label(_("Image watermark"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_image),
                               GTK_CHECK_BUTTON(radio_text));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_image), !settings->mode);

    vbox_image = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    chooser_image = gtk_file_chooser_button_new(
        _("Select image"), GTK_FILE_CHOOSER_ACTION_OPEN);

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, _("All supported types"));

    GtkFileFilter *supported[5];
    supported[0] = gtk_file_filter_new();
    gtk_file_filter_set_name(supported[0], "Bitmap (*.bmp)");
    file_filters_add_patterns(supported[0], filter_all, "*.bmp", NULL);

    supported[1] = gtk_file_filter_new();
    gtk_file_filter_set_name(supported[1], "JPEG (*.jpg, *.jpeg, *.jpe)");
    file_filters_add_patterns(supported[1], filter_all, "*.jpg", "*.jpeg", "*.jpe", NULL);

    supported[2] = gtk_file_filter_new();
    gtk_file_filter_set_name(supported[2], "GIF (*.gif)");
    file_filters_add_patterns(supported[2], filter_all, "*.gif", NULL);

    supported[3] = gtk_file_filter_new();
    gtk_file_filter_set_name(supported[3], "PNG (*.png)");
    file_filters_add_patterns(supported[3], filter_all, "*.png", NULL);

    supported[4] = gtk_file_filter_new();
    gtk_file_filter_set_name(supported[4], "TIFF (*.tif, *.tiff)");
    file_filters_add_patterns(supported[4], filter_all, "*.tiff", "*.tif", NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_image), filter_all);
    for (int i = 0; i < 5; i++)
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_image), supported[i]);

    if (settings->image_file) {
        GFile *f = g_file_new_for_path(settings->image_file);
        gtk_file_chooser_set_file(GTK_FILE_CHOOSER(chooser_image), f, NULL);
        g_object_unref(f);
    }

    check_image_adaptsize = gtk_check_button_new_with_label(
        _("Adaptive size"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_image_adaptsize),
                                settings->image_sizemode != WM_IMG_NOSIZE);

    hbox_image_size    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    spin_image_sizepercent = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->image_size_percent,
                                          0.1, 100.0, 0.1, 1, 0)), 1, 1);
    label_percentof    = gtk_label_new(_("% of"));
    combo_image_sizemode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_image_sizemode),
                                   _("Width"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_image_sizemode),
                                   _("Height"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_image_sizemode),
                             settings->image_sizemode == WM_IMG_SIZEH ? 1 : 0);

    gtk_box_append(GTK_BOX(hbox_image_size), spin_image_sizepercent);
    gtk_box_append(GTK_BOX(hbox_image_size), label_percentof);
    gtk_box_append(GTK_BOX(hbox_image_size), combo_image_sizemode);

    gtk_box_append(GTK_BOX(vbox_image), chooser_image);
    gtk_box_append(GTK_BOX(vbox_image), check_image_adaptsize);
    gtk_box_append(GTK_BOX(vbox_image), hbox_image_size);

    box_img_indent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(box_img_indent, 20);
    gtk_box_append(GTK_BOX(box_img_indent), vbox_image);
    gtk_widget_set_hexpand(vbox_image, TRUE);

    gtk_box_append(GTK_BOX(vbox_mode_image), radio_image);
    gtk_box_append(GTK_BOX(vbox_mode_image), box_img_indent);
    gtk_widget_set_hexpand(vbox_mode_image, TRUE);

    gtk_box_append(GTK_BOX(hbox_mode), vbox_mode_text);
    gtk_box_append(GTK_BOX(hbox_mode), sep_v);
    gtk_box_append(GTK_BOX(hbox_mode), vbox_mode_image);
    gtk_box_append(GTK_BOX(gui), hbox_mode);

    /* --- Opacity --- */
    sep_h          = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    vbox_position  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    hbox_opacity   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    label_opacity  = gtk_label_new(g_strconcat(_("Opacity"), ":", NULL));
    scale_opacity  = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                              1, 100, 1);
    gtk_widget_set_size_request(scale_opacity, 150, 50);
    gtk_widget_set_hexpand(scale_opacity, TRUE);
    gtk_range_set_value(GTK_RANGE(scale_opacity), settings->opacity);
    label_percent  = gtk_label_new("%");

    gtk_box_append(GTK_BOX(hbox_opacity), label_opacity);
    gtk_box_append(GTK_BOX(hbox_opacity), scale_opacity);
    gtk_box_append(GTK_BOX(hbox_opacity), label_percent);
    gtk_box_append(GTK_BOX(vbox_position), hbox_opacity);

    /* --- Position grid --- */
    frame_position = gtk_frame_new(g_strconcat(_("Position on the image"), ":", NULL));
    grid_position  = gtk_grid_new();
    gtk_widget_set_size_request(grid_position, 250, 120);

    for (watermark_position pos = WM_POS_TL; pos < WM_POS_END; pos++) {
        GtkWidget *icon = image_new_from_resource(
            g_strconcat("/gimp/plugin/bimp/icons/pos-",
                        watermark_pos_get_abbreviation(pos),
                        "-icon.png", NULL));
        position_buttons[pos] = gtk_toggle_button_new();
        gtk_button_set_child(GTK_BUTTON(position_buttons[pos]), icon);
        gtk_widget_set_tooltip_text(position_buttons[pos],
                                    watermark_pos_get_string(pos));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(position_buttons[pos]),
                                     settings->position == pos);
        gtk_grid_attach(GTK_GRID(grid_position), position_buttons[pos],
                        pos % 3, pos / 3, 1, 1);
        g_signal_connect(position_buttons[pos], "toggled",
                         G_CALLBACK(position_btn_toggled),
                         GINT_TO_POINTER((int)pos));
    }
    gtk_frame_set_child(GTK_FRAME(frame_position), grid_position);
    gtk_box_append(GTK_BOX(vbox_position), frame_position);

    /* --- Distance to edge --- */
    hbox_edge  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    label_edge = gtk_label_new(g_strconcat(_("Distance to edge"), ":", NULL));
    spin_edge  = gtk_spin_button_new(
        GTK_ADJUSTMENT(gtk_adjustment_new(settings->edge_distance,
                                          0, G_MAXINT, 1, 1, 0)), 1, 0);
    label_px   = gtk_label_new("px");

    gtk_box_append(GTK_BOX(hbox_edge), label_edge);
    gtk_box_append(GTK_BOX(hbox_edge), spin_edge);
    gtk_box_append(GTK_BOX(hbox_edge), label_px);
    gtk_box_append(GTK_BOX(vbox_position), hbox_edge);

    gtk_box_append(GTK_BOX(gui), sep_h);
    gtk_box_append(GTK_BOX(gui), vbox_position);

    toggle_group(NULL, NULL);
    toggle_image_size(NULL, NULL);

    g_signal_connect(G_OBJECT(radio_text), "toggled",
                     G_CALLBACK(toggle_group), NULL);
    g_signal_connect(G_OBJECT(check_image_adaptsize), "toggled",
                     G_CALLBACK(toggle_image_size), NULL);

    return gui;
}

static void
toggle_group(GtkCheckButton *btn, gpointer data)
{
    gboolean text_mode =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(radio_text));
    gtk_widget_set_sensitive(grid_text,  text_mode);
    gtk_widget_set_sensitive(vbox_image, !text_mode);
}

static void
toggle_image_size(GtkCheckButton *btn, gpointer data)
{
    gtk_widget_set_sensitive(hbox_image_size,
        gtk_check_button_get_active(GTK_CHECK_BUTTON(check_image_adaptsize)));
}

static void
position_btn_toggled(GtkToggleButton *btn, gpointer data)
{
    if (!gtk_toggle_button_get_active(btn)) return;
    int active_pos = GPOINTER_TO_INT(data);
    for (int i = 0; i < WM_POS_END; i++) {
        if (i != active_pos)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(position_buttons[i]),
                                         FALSE);
    }
}

static const char *watermark_pos_strings[9] = {
    "Top-left", "Top-center", "Top-right",
    "Center-left", "Center", "Center-right",
    "Bottom-left", "Bottom-center", "Bottom-right"
};
static const char *
watermark_pos_get_string(watermark_position wp)
{
    return _(watermark_pos_strings[wp]);
}

static const char *watermark_pos_abbreviations[9] = {
    "tl", "tc", "tr",
    "cl", "cc", "cr",
    "bl", "bc", "br"
};
static const char *
watermark_pos_get_abbreviation(watermark_position wp)
{
    return watermark_pos_abbreviations[wp];
}

static void
file_filters_add_patterns(GtkFileFilter *filter1, GtkFileFilter *filter2, ...)
{
    va_list patterns;
    gchar  *pattern;

    va_start(patterns, filter2);
    while ((pattern = (gchar *)va_arg(patterns, void *)) != NULL) {
        gtk_file_filter_add_pattern(filter1, pattern);
        gtk_file_filter_add_pattern(filter2, pattern);
    }
    va_end(patterns);
}

void
bimp_watermark_save(watermark_settings orig_settings)
{
    GdkRGBA rgba;

    orig_settings->mode =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(radio_text));
    orig_settings->text =
        g_strdup(gtk_editable_get_text(GTK_EDITABLE(entry_text)));
    orig_settings->font = pango_font_description_from_string(
        gtk_font_chooser_get_font(GTK_FONT_CHOOSER(chooser_font)));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(chooser_color), &rgba);
    orig_settings->color = rgba;

    GFile *f = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser_image));
    if (f) {
        gchar *path = g_file_get_path(f);
        if (path) orig_settings->image_file = g_strdup(path);
        g_free(path);
        g_object_unref(f);
    }

    watermark_image_sizemode new_mode = WM_IMG_NOSIZE;
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_image_adaptsize))) {
        new_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_image_sizemode)) == 0
            ? WM_IMG_SIZEW : WM_IMG_SIZEH;
    }
    orig_settings->image_sizemode = new_mode;
    orig_settings->image_size_percent =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_image_sizepercent));
    orig_settings->opacity =
        (float)gtk_range_get_value(GTK_RANGE(scale_opacity));
    orig_settings->edge_distance =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_edge));

    for (watermark_position pos = WM_POS_TL; pos < WM_POS_END; pos++) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(position_buttons[pos]))) {
            orig_settings->position = pos;
            break;
        }
    }
}
