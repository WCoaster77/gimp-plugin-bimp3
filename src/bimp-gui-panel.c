#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include "bimp-gui-priv.h"
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-operate.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

GtkWidget *button_preview       = NULL;
GtkWidget *button_outfolder     = NULL;
GtkWidget *button_samefolder    = NULL;
GtkWidget *check_keepfolderhierarchy = NULL;
GtkWidget *check_deleteondone   = NULL;
GtkWidget *check_keepdates      = NULL;
char      *selected_source_folder = NULL;
char      *last_input_location    = NULL;

char *
get_outputfolder_name(void)
{
    char *last = g_strrstr(bimp_output_folder, FILE_SEPARATOR_STR);
    char *name = last ? last + 1 : bimp_output_folder;
    if (!name || *name == '\0') name = bimp_output_folder;
    if (strlen(name) > 24) {
        char *t = g_malloc(25);
        memcpy(t, name, 21);
        t[21] = t[22] = t[23] = '.';
        t[24] = '\0';
        return t;
    }
    return name;
}

void
update_selection(char *filename)
{
    g_free(selected_source_folder);
    selected_source_folder = NULL;
    if (filename) {
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
            filename, FILE_PREVIEW_W - 20, FILE_PREVIEW_H - 30, TRUE, NULL);
        if (pb) {
            gtk_button_set_child(GTK_BUTTON(button_preview),
                                 gtk_image_new_from_pixbuf(pb));
            g_object_unref(pb);
        }
        gtk_widget_set_visible(button_preview, TRUE);
        selected_source_folder = g_path_get_dirname(filename);
    } else {
        gtk_button_set_child(GTK_BUTTON(button_preview), NULL);
        gtk_widget_set_visible(button_preview, FALSE);
    }
    gtk_widget_set_sensitive(button_samefolder, selected_source_folder != NULL);
}

static GtkWidget *
build_addfiles_popover(GtkWidget *parent)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *b1 = popover_button(_("Add single images..."),
                                    G_CALLBACK(open_file_chooser), NULL);
    g_signal_connect(b1, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b1);
    GtkWidget *b2 = popover_button(_("Add folders..."),
                                    G_CALLBACK(open_folder_chooser), NULL);
    g_signal_connect(b2, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b2);
    GtkWidget *b3 = popover_button(_("Add all opened images"),
                                    G_CALLBACK(add_opened_files), NULL);
    g_signal_connect(b3, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b3);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static GtkWidget *
build_removefiles_popover(GtkWidget *parent)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *b1 = popover_button(_("Remove selected"),
                                    G_CALLBACK(remove_input_file), NULL);
    g_signal_connect(b1, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b1);
    GtkWidget *b2 = popover_button(_("Remove all"),
                                    G_CALLBACK(remove_all_input_files), NULL);
    g_signal_connect(b2, "clicked", G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b2);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static void
open_addfiles_popover(GtkWidget *widget, gpointer data)
{
    popup_and_track(build_addfiles_popover(widget), widget);
}

static void
open_removefiles_popover(GtkWidget *widget, gpointer data)
{
    popup_and_track(build_removefiles_popover(widget), widget);
}

static void
build_preview_pixbufs(char *src_path,
                      GdkPixbuf **pb_before, GdkPixbuf **pb_after)
{
    GFile     *src_file = g_file_new_for_path(src_path);
    GimpImage *orig_img = gimp_file_load(GIMP_RUN_NONINTERACTIVE, src_file);
    g_object_unref(src_file);
    *pb_before = NULL;
    *pb_after  = NULL;
    if (!orig_img) return;
    GimpLayer *orig_flat =
        gimp_image_merge_visible_layers(orig_img, GIMP_CLIP_TO_IMAGE);
    *pb_before = gimp_drawable_get_thumbnail(GIMP_DRAWABLE(orig_flat),
                     PREVIEW_IMG_W, PREVIEW_IMG_H, GIMP_PIXBUF_KEEP_ALPHA);
    g_object_unref(orig_img);
    image_output out = g_new0(struct imageout_str, 1);
    bimp_init_batch();
    bimp_apply_drawable_manipulations(out, src_path, src_path);
    if (out->image) {
        GimpLayer *final_flat =
            gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
        *pb_after = gimp_drawable_get_thumbnail(GIMP_DRAWABLE(final_flat),
                        PREVIEW_IMG_W, PREVIEW_IMG_H, GIMP_PIXBUF_KEEP_ALPHA);
        g_object_unref(out->image);
    }
    g_free(out);
}

static void
show_preview_dialog(GdkPixbuf *pb_before, GdkPixbuf *pb_after)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        _("Preview"), GTK_WINDOW(bimp_window_main),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("_Close"), GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    GtkWidget *vbox  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *label = gtk_label_new(
        _("This is how the selected image will look after batch processing."));
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    GtkWidget *hbox   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *before = pb_before ? gtk_image_new_from_pixbuf(pb_before)
                                   : gtk_label_new(_("(original)"));
    GtkWidget *arrow  = gtk_image_new_from_icon_name("go-next");
    GtkWidget *after  = pb_after ? gtk_image_new_from_pixbuf(pb_after)
                                  : gtk_label_new(_("(result unavailable)"));
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), before);
    gtk_box_append(GTK_BOX(hbox), arrow);
    gtk_box_append(GTK_BOX(hbox), after);
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), hbox);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);
    gtk_widget_set_visible(dialog, TRUE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_window_destroy(GTK_WINDOW(dialog));
}

void
show_preview(GtkWidget *widget, gpointer data)
{
    if (g_slist_length(bimp_selected_manipulations) == 0) {
        bimp_show_error_dialog(
            _("Can't show a preview: the manipulations set is empty."),
            bimp_window_main);
        return;
    }
    GSList *sel = get_treeview_selection();
    if (!sel || g_slist_length(sel) != 1) { g_slist_free(sel); return; }
    char *src_path = (char *)sel->data;
    g_slist_free(sel);
    GdkPixbuf *pb_before, *pb_after;
    build_preview_pixbufs(src_path, &pb_before, &pb_after);
    show_preview_dialog(pb_before, pb_after);
    if (pb_before) g_object_unref(pb_before);
    if (pb_after)  g_object_unref(pb_after);
}

static void
build_filelist_section(GtkGrid *grid, GtkWidget **btn_add, GtkWidget **btn_rem)
{
    GtkWidget *scroll_input = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_input),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    treeview_files = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview_files), FALSE);
    gtk_tree_selection_set_mode(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_files)),
        GTK_SELECTION_MULTIPLE);
    *btn_add = gtk_button_new_with_label(_("Add images"));
    *btn_rem = gtk_button_new_with_label(_("Remove images"));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_input), treeview_files);
    gtk_widget_set_hexpand(scroll_input, TRUE);
    gtk_widget_set_vexpand(scroll_input, TRUE);
    gtk_grid_attach(grid, scroll_input, 0, 0, 2, 1);
    gtk_grid_attach(grid, *btn_add,     0, 1, 1, 1);
    gtk_grid_attach(grid, *btn_rem,     1, 1, 1, 1);
}

static GtkWidget *
build_useroptions_box(void)
{
    GtkWidget *vbox           = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *hbox_outfolder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    GtkWidget *label_chooser  = gtk_label_new(g_strconcat(_("Output folder"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_chooser), 0.0f);
    bimp_output_folder = get_user_dir();
    button_outfolder   = gtk_button_new_with_label(get_outputfolder_name());
    gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);
    button_samefolder  = gtk_button_new();
    GtkWidget *undo_icon = gtk_image_new_from_icon_name("edit-undo");
    gtk_button_set_child(GTK_BUTTON(button_samefolder), undo_icon);
    gtk_widget_set_tooltip_text(button_samefolder,
        _("Use the selected file's location as the output"));
    bimp_opt_alertoverwrite      = BIMP_ASK_OVERWRITE;
    bimp_opt_keepfolderhierarchy = FALSE;
    check_keepfolderhierarchy = gtk_check_button_new_with_label(
        _("Keep folder hierarchy"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_keepfolderhierarchy),
                                bimp_opt_keepfolderhierarchy);
    bimp_opt_deleteondone = FALSE;
    check_deleteondone = gtk_check_button_new_with_label(
        _("Delete original file when done"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_deleteondone),
                                bimp_opt_deleteondone);
    bimp_opt_keepdates = FALSE;
    check_keepdates = gtk_check_button_new_with_label(
        _("Keep the modification dates"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_keepdates),
                                bimp_opt_keepdates);
    button_preview = gtk_button_new_with_label(_("Click for preview"));
    gtk_box_append(GTK_BOX(vbox), label_chooser);
    gtk_box_append(GTK_BOX(hbox_outfolder), button_outfolder);
    gtk_box_append(GTK_BOX(hbox_outfolder), button_samefolder);
    gtk_box_append(GTK_BOX(vbox), hbox_outfolder);
    gtk_box_append(GTK_BOX(vbox), check_keepfolderhierarchy);
    gtk_box_append(GTK_BOX(vbox), check_deleteondone);
    gtk_box_append(GTK_BOX(vbox), check_keepdates);
    gtk_box_append(GTK_BOX(vbox), button_preview);
    return vbox;
}

GtkWidget *
bimp_option_panel_new(void)
{
    GtkWidget *panel     = gtk_frame_new(_("Input files and options"));
    GtkWidget *grid      = gtk_grid_new();
    GtkWidget *btn_add, *btn_rem;
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    build_filelist_section(GTK_GRID(grid), &btn_add, &btn_rem);
    GtkWidget *vopts = build_useroptions_box();
    gtk_grid_attach(GTK_GRID(grid), vopts, 2, 0, 1, 2);
    gtk_frame_set_child(GTK_FRAME(panel), grid);
    g_signal_connect(btn_add, "clicked",
                     G_CALLBACK(open_addfiles_popover), NULL);
    g_signal_connect(btn_rem, "clicked",
                     G_CALLBACK(open_removefiles_popover), NULL);
    g_signal_connect(button_outfolder, "clicked",
                     G_CALLBACK(open_outputfolder_chooser), NULL);
    g_signal_connect(button_samefolder, "clicked",
                     G_CALLBACK(set_source_output_folder), NULL);
    g_signal_connect(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_files)),
        "changed", G_CALLBACK(select_filename), NULL);
    g_signal_connect(button_preview, "clicked",
                     G_CALLBACK(show_preview), NULL);
    init_fileview();
    bimp_refresh_fileview();
    return panel;
}
