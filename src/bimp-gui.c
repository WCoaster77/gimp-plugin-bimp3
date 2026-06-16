/*
 * Functions used to build and return the main BIMP user interface
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <string.h>
#include <stdlib.h>
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp-manipulations-gui.h"
#include "bimp-operate.h"
#include "bimp-serialize.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

static GtkWidget *sequence_panel_new(void);
static GtkWidget *option_panel_new(void);
static void       init_fileview(void);
static void       add_to_fileview(char *);
static GSList    *get_treeview_selection(void);
static void       open_file_chooser(GtkWidget *, gpointer);
static void       open_folder_chooser(GtkWidget *, gpointer);
static void       add_input_file(char *);
static void       add_input_folder(char *, gpointer);
static void       add_opened_files(GtkWidget *, gpointer);
static void       remove_input_file(GtkWidget *, gpointer);
static void       remove_all_input_files(GtkWidget *, gpointer);
static void       select_filename(GtkTreeSelection *, gpointer);
static void       update_selection(char *);
static void       show_preview(GtkWidget *, gpointer);
static char      *get_outputfolder_name(void);
static void       open_outputfolder_chooser(GtkWidget *, gpointer);
static void       set_source_output_folder(GtkWidget *, gpointer);
static void       add_manipulation_from_id(GtkWidget *, gpointer);
static void       edit_clicked_manipulation(GtkWidget *, gpointer);
static void       remove_clicked_manipulation(GtkWidget *, gpointer);
static void       add_manipulation_button(manipulation);
static GtkWidget *build_add_popover(GtkWidget *);
static GtkWidget *build_edit_popover(GtkWidget *, manipulation);
static GtkWidget *build_addfiles_popover(GtkWidget *);
static GtkWidget *build_removefiles_popover(GtkWidget *);
static void       popover_on_closed(GtkWidget *, gpointer);
static void       open_manipulation_popover(GtkWidget *, gpointer);
static void       open_addfiles_popover(GtkWidget *, gpointer);
static void       open_removefiles_popover(GtkWidget *, gpointer);
static void       save_set(GtkWidget *, gpointer);
static void       load_set(GtkWidget *, gpointer);
static void       open_about(void);
static const gchar *progressbar_init_hidden(void);
static void         progressbar_start_hidden(const gchar *, gboolean, gpointer);
static void         progressbar_end_hidden(gpointer);
static void         progressbar_settext_hidden(const gchar *, gpointer);
static void         progressbar_setvalue_hidden(double, gpointer);

GtkWidget *bimp_window_main;

static GtkWidget *panel_sequence, *panel_options;
static GtkWidget *hbox_sequence;
static GtkWidget *scroll_sequence;
static GtkWidget *check_keepfolderhierarchy, *check_deleteondone, *check_keepdates;
static GtkWidget *treeview_files;
static GtkWidget *button_preview, *button_outfolder, *button_samefolder;
static GtkWidget *progressbar_visible;

static char        *selected_source_folder;
static char        *last_input_location;
static const gchar *progressbar_data;

/* manipulation under pointer when right-click popover was triggered */
static manipulation clicked_man;

enum
{
    LIST_ITEM = 0,
    N_COLUMNS
};

/* -------------------------------------------------------------------------
 * Main dialog
 * ---------------------------------------------------------------------- */

void
bimp_show_gui(void)
{
    GtkWidget *vbox_main;
    GtkWidget *content;

    gimp_ui_init(PLUG_IN_BINARY);

    bimp_window_main = gimp_dialog_new(
        PLUG_IN_FULLNAME,
        PLUG_IN_BINARY,
        NULL, 0, NULL, NULL,
        _("About"),   GTK_RESPONSE_HELP,
        _("_Close"),  GTK_RESPONSE_CLOSE,
        _("_Apply"),  GTK_RESPONSE_APPLY,
        _("_Stop"),   GTK_RESPONSE_CANCEL,
        NULL
    );

    gimp_window_set_transient(GTK_WINDOW(bimp_window_main));
    gtk_window_set_default_size(GTK_WINDOW(bimp_window_main),
        (int)(PREVIEW_IMG_W * 2.5),
        SEQ_BUTTON_H + PREVIEW_IMG_H + 160);

    vbox_main      = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    panel_sequence = sequence_panel_new();
    panel_options  = option_panel_new();

    progressbar_visible = gtk_progress_bar_new();
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar_visible), " ");
    progressbar_data = progressbar_init_hidden();

    gtk_box_append(GTK_BOX(vbox_main), panel_sequence);
    gtk_box_append(GTK_BOX(vbox_main), panel_options);
    gtk_widget_set_vexpand(panel_options, TRUE);
    gtk_box_append(GTK_BOX(vbox_main), progressbar_visible);

    content = gtk_dialog_get_content_area(GTK_DIALOG(bimp_window_main));
    gtk_box_append(GTK_BOX(content), vbox_main);

    gtk_widget_set_visible(bimp_window_main, TRUE);
    gtk_widget_set_visible(button_preview, FALSE);
    bimp_set_busy(FALSE);

    while (TRUE) {
        gint run = gimp_dialog_run(GIMP_DIALOG(bimp_window_main));
        if (run == GTK_RESPONSE_APPLY) {
            if (g_slist_length(bimp_selected_manipulations) == 0) {
                bimp_show_error_dialog(
                    _("The manipulations set is empty!"), bimp_window_main);
            } else if (g_slist_length(bimp_input_filenames) == 0) {
                bimp_show_error_dialog(
                    _("The file list is empty!"), bimp_window_main);
            } else {
                bimp_opt_alertoverwrite = BIMP_ASK_OVERWRITE;
                bimp_opt_keepfolderhierarchy =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_keepfolderhierarchy));
                bimp_opt_deleteondone =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_deleteondone));
                bimp_opt_keepdates =
                    gtk_check_button_get_active(
                        GTK_CHECK_BUTTON(check_keepdates));
                bimp_start_batch(bimp_window_main);
            }
        } else if (run == GTK_RESPONSE_HELP) {
            open_about();
        } else if (run == GTK_RESPONSE_CANCEL) {
            bimp_set_busy(FALSE);
        } else {
            gimp_progress_uninstall(progressbar_data);
            gtk_window_destroy(GTK_WINDOW(bimp_window_main));
            return;
        }
    }
}

/* -------------------------------------------------------------------------
 * Sequence panel
 * ---------------------------------------------------------------------- */

static GtkWidget *
sequence_panel_new(void)
{
    GtkWidget *panel = gtk_frame_new(_("Manipulation set"));

    scroll_sequence = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_sequence),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    hbox_sequence = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_sequence),
                                   hbox_sequence);
    gtk_frame_set_child(GTK_FRAME(panel), scroll_sequence);

    bimp_refresh_sequence_panel();
    return panel;
}

/* -------------------------------------------------------------------------
 * Options panel
 * ---------------------------------------------------------------------- */

static GtkWidget *
option_panel_new(void)
{
    GtkWidget *panel, *grid;
    GtkWidget *scroll_input;
    GtkWidget *button_add, *button_remove;
    GtkWidget *vbox_useroptions, *hbox_outfolder;
    GtkWidget *label_chooser;
    GtkWidget *undo_icon;

    panel = gtk_frame_new(_("Input files and options"));
    grid  = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    scroll_input = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_input),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    treeview_files = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview_files), FALSE);
    gtk_tree_selection_set_mode(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_files)),
        GTK_SELECTION_MULTIPLE);

    button_add    = gtk_button_new_with_label(_("Add images"));
    button_remove = gtk_button_new_with_label(_("Remove images"));

    vbox_useroptions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

    hbox_outfolder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    label_chooser  = gtk_label_new(g_strconcat(_("Output folder"), ":", NULL));
    gtk_label_set_xalign(GTK_LABEL(label_chooser), 0.0f);

    bimp_output_folder = get_user_dir();
    button_outfolder   = gtk_button_new_with_label(get_outputfolder_name());
    gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);

    button_samefolder = gtk_button_new();
    undo_icon = gtk_image_new_from_icon_name("edit-undo");
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

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_input),
                                   treeview_files);

    gtk_box_append(GTK_BOX(vbox_useroptions), label_chooser);
    gtk_box_append(GTK_BOX(hbox_outfolder), button_outfolder);
    gtk_box_append(GTK_BOX(hbox_outfolder), button_samefolder);
    gtk_box_append(GTK_BOX(vbox_useroptions), hbox_outfolder);
    gtk_box_append(GTK_BOX(vbox_useroptions), check_keepfolderhierarchy);
    gtk_box_append(GTK_BOX(vbox_useroptions), check_deleteondone);
    gtk_box_append(GTK_BOX(vbox_useroptions), check_keepdates);
    gtk_box_append(GTK_BOX(vbox_useroptions), button_preview);

    gtk_widget_set_hexpand(scroll_input, TRUE);
    gtk_widget_set_vexpand(scroll_input, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scroll_input,     0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), button_add,       0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_remove,    1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), vbox_useroptions, 2, 0, 1, 2);

    gtk_frame_set_child(GTK_FRAME(panel), grid);

    g_signal_connect(button_add,    "clicked",
                     G_CALLBACK(open_addfiles_popover), NULL);
    g_signal_connect(button_remove, "clicked",
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

/* -------------------------------------------------------------------------
 * File input management
 * ---------------------------------------------------------------------- */

static void
add_input_file(char *filename)
{
    if (!g_slist_find_custom(bimp_input_filenames, filename,
                             (GCompareFunc)strcmp)) {
        bimp_input_filenames =
            g_slist_append(bimp_input_filenames, g_strdup(filename));
        bimp_refresh_fileview();
    }
}

static void
add_input_folder_r(char *folder, gboolean with_subdirs)
{
    static const char *img_exts[] = {
        ".bmp", ".jpeg", ".jpg", ".jpe", ".jp2",
        ".gif", ".heif", ".heic", ".png",
        ".tif", ".tiff", ".tga", ".svg", ".webp",
        ".avif", ".xpm", ".exr", ".dds", ".xcf", NULL
    };
    GDir        *dp;
    const gchar *entry;

    dp = g_dir_open(folder, 0, NULL);
    if (!dp) {
        bimp_show_error_dialog(
            g_strdup_printf(_("Couldn't read into \"%s\" directory."), folder),
            bimp_window_main);
        return;
    }

    while ((entry = g_dir_read_name(dp))) {
        char *filepath = g_build_filename(folder, entry, NULL);

        if (g_file_test(filepath, G_FILE_TEST_IS_DIR)) {
            if (with_subdirs &&
                g_strcmp0(entry, ".") != 0 &&
                g_strcmp0(entry, "..") != 0)
            {
                add_input_folder_r(filepath, with_subdirs);
            }
            g_free(filepath);
            continue;
        }

        const char *dot = strrchr(entry, '.');
        if (dot) {
            for (int i = 0; img_exts[i]; i++) {
                if (g_ascii_strcasecmp(dot, img_exts[i]) == 0) {
                    if (!g_slist_find_custom(bimp_input_filenames, filepath,
                                            (GCompareFunc)strcmp))
                    {
                        bimp_input_filenames =
                            g_slist_append(bimp_input_filenames,
                                           g_strdup(filepath));
                    }
                    break;
                }
            }
        }
        g_free(filepath);
    }
    g_dir_close(dp);
}

static void
add_input_folder(char *folder, gpointer with_subdirs)
{
    add_input_folder_r(folder, (gboolean)GPOINTER_TO_INT(with_subdirs));
    bimp_refresh_fileview();
}

static void
add_opened_files(GtkWidget *widget, gpointer data)
{
    GList   *images = gimp_image_list();
    GList   *node;
    gboolean missing = FALSE;

    for (node = images; node; node = node->next) {
        GimpImage *img  = GIMP_IMAGE(node->data);
        GFile     *file = gimp_image_get_file(img);
        if (file) {
            gchar *path = g_file_get_path(file);
            if (path) add_input_file(path);
            g_object_unref(file);
        } else {
            missing = TRUE;
        }
    }
    g_list_free(images);

    if (missing) {
        bimp_show_error_dialog(
            _("Some images were not imported because they have not been saved yet."),
            bimp_window_main);
    }
}

/* -------------------------------------------------------------------------
 * File list view
 * ---------------------------------------------------------------------- */

static void
init_fileview(void)
{
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;
    GtkListStore      *store;

    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes(
        "Files", renderer, "text", LIST_ITEM, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_files), column);

    store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview_files),
                            GTK_TREE_MODEL(store));
    g_object_unref(store);
}

static void
add_to_fileview(char *str)
{
    GtkListStore *store;
    GtkTreeIter   iter;

    store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files)));
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}

void
bimp_refresh_fileview(void)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GSList       *node;

    store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files)));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_files));

    if (gtk_tree_model_get_iter_first(model, &iter))
        gtk_list_store_clear(store);

    for (node = bimp_input_filenames; node; node = node->next)
        add_to_fileview(node->data);
}

static GSList *
get_treeview_selection(void)
{
    GtkTreeModel *model;
    GList        *rows;
    GSList       *out = NULL;

    rows = gtk_tree_selection_get_selected_rows(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_files)), &model);

    for (GList *i = rows; i; i = g_list_next(i)) {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath *)i->data)) {
            char *val;
            gtk_tree_model_get(model, &iter, LIST_ITEM, &val, -1);
            out = g_slist_append(out, val);
        }
    }
    g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(rows);
    return out;
}

static void
remove_input_file(GtkWidget *widget, gpointer data)
{
    GSList *sel = get_treeview_selection();
    for (GSList *i = sel; i; i = g_slist_next(i)) {
        GSList *found = g_slist_find_custom(bimp_input_filenames,
                                            i->data, (GCompareFunc)strcmp);
        if (found)
            bimp_input_filenames = g_slist_delete_link(
                bimp_input_filenames, found);
    }
    g_slist_free(sel);
    bimp_refresh_fileview();
    update_selection(NULL);
}

static void
remove_all_input_files(GtkWidget *widget, gpointer data)
{
    g_slist_free_full(bimp_input_filenames, g_free);
    bimp_input_filenames = NULL;
    bimp_refresh_fileview();
    update_selection(NULL);
}

static void
select_filename(GtkTreeSelection *sel, gpointer data)
{
    GSList *selection = get_treeview_selection();
    if (selection && g_slist_length(selection) == 1)
        update_selection((char *)selection->data);
    else
        update_selection(NULL);
    g_slist_free(selection);
}

static void
update_selection(char *filename)
{
    g_free(selected_source_folder);
    selected_source_folder = NULL;

    if (filename) {
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
            filename,
            FILE_PREVIEW_W - 20, FILE_PREVIEW_H - 30,
            TRUE, NULL);
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

static void
show_preview(GtkWidget *widget, gpointer data)
{
    GSList *sel;
    char   *src_path;
    char   *tmp_path;

    if (g_slist_length(bimp_selected_manipulations) == 0) {
        bimp_show_error_dialog(
            _("Can't show a preview: the manipulations set is empty."),
            bimp_window_main);
        return;
    }

    sel = get_treeview_selection();
    if (!sel || g_slist_length(sel) != 1) {
        g_slist_free(sel);
        return;
    }
    src_path = (char *)sel->data;
    g_slist_free(sel);

    /* Load original for before-thumbnail */
    GFile     *src_file = g_file_new_for_path(src_path);
    GimpImage *orig_img = gimp_file_load(GIMP_RUN_NONINTERACTIVE, src_file);
    g_object_unref(src_file);
    if (!orig_img) return;

    GimpLayer *orig_flat =
        gimp_image_merge_visible_layers(orig_img, GIMP_CLIP_TO_IMAGE);
    GdkPixbuf *pb_before =
        gimp_drawable_get_thumbnail(GIMP_DRAWABLE(orig_flat),
                                    PREVIEW_IMG_W, PREVIEW_IMG_H,
                                    GIMP_PIXBUF_KEEP_ALPHA);
    g_object_unref(orig_img);

    /* Apply manipulations, output to a temp path */
    tmp_path = g_build_filename(g_get_tmp_dir(), "bimp_preview.png", NULL);
    image_output out = g_new0(struct imageout_str, 1);

    bimp_init_batch();
    bimp_apply_drawable_manipulations(out, src_path, tmp_path);
    g_free(tmp_path);

    GdkPixbuf *pb_after = NULL;
    if (out->image) {
        GimpLayer *final_flat =
            gimp_image_merge_visible_layers(out->image, GIMP_CLIP_TO_IMAGE);
        pb_after = gimp_drawable_get_thumbnail(GIMP_DRAWABLE(final_flat),
                                               PREVIEW_IMG_W, PREVIEW_IMG_H,
                                               GIMP_PIXBUF_KEEP_ALPHA);
        g_object_unref(out->image);
    }
    g_free(out);

    /* Preview dialog */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        _("Preview"),
        GTK_WINDOW(bimp_window_main),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("_Close"), GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *vbox  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *label = gtk_label_new(
        _("This is how the selected image will look after batch processing."));
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

    GtkWidget *hbox  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *before = pb_before
        ? gtk_image_new_from_pixbuf(pb_before)
        : gtk_label_new(_("(original)"));
    GtkWidget *arrow = gtk_image_new_from_icon_name("go-next");
    GtkWidget *after = pb_after
        ? gtk_image_new_from_pixbuf(pb_after)
        : gtk_label_new(_("(result unavailable)"));

    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), before);
    gtk_box_append(GTK_BOX(hbox), arrow);
    gtk_box_append(GTK_BOX(hbox), after);

    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), hbox);
    gtk_box_append(
        GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

    gtk_widget_set_visible(dialog, TRUE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_window_destroy(GTK_WINDOW(dialog));

    if (pb_before) g_object_unref(pb_before);
    if (pb_after)  g_object_unref(pb_after);
}

/* -------------------------------------------------------------------------
 * File choosers
 * ---------------------------------------------------------------------- */

static void
open_file_chooser(GtkWidget *widget, gpointer data)
{
    static const char *img_exts[] = {
        "*.bmp", "*.jpeg", "*.jpg", "*.jpe", "*.jp2",
        "*.gif", "*.heif", "*.heic", "*.png",
        "*.tif", "*.tiff", "*.tga", "*.svg", "*.webp",
        "*.avif", "*.xpm", "*.exr", "*.dds", "*.xcf", NULL
    };

    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select images"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Add"),    GTK_RESPONSE_ACCEPT,
        NULL
    );
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

    if (last_input_location) {
        GFile *dir = g_file_new_for_path(last_input_location);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),
                                            dir, NULL);
        g_object_unref(dir);
    }

    GtkFileFilter *all = gtk_file_filter_new();
    gtk_file_filter_set_name(all, _("All supported types"));
    for (int i = 0; img_exts[i]; i++) {
        gtk_file_filter_add_pattern(all, img_exts[i]);
        /* also match uppercase */
        gchar *upper = g_ascii_strup(img_exts[i], -1);
        gtk_file_filter_add_pattern(all, upper);
        g_free(upper);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), all);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GFile *cur = gtk_file_chooser_get_current_folder(
            GTK_FILE_CHOOSER(chooser));
        g_free(last_input_location);
        last_input_location = cur ? g_file_get_path(cur) : NULL;
        if (cur) g_object_unref(cur);

        GListModel *files =
            gtk_file_chooser_get_files(GTK_FILE_CHOOSER(chooser));
        guint n = g_list_model_get_n_items(files);
        for (guint i = 0; i < n; i++) {
            GFile *f = G_FILE(g_list_model_get_item(files, i));
            gchar *p = g_file_get_path(f);
            if (p) add_input_file(p);
            g_object_unref(f);
        }
        g_object_unref(files);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

static void
open_folder_chooser(GtkWidget *widget, gpointer data)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select folders containing images"), NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Add"),    GTK_RESPONSE_ACCEPT,
        NULL
    );
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

    GtkWidget *subdirs = gtk_check_button_new_with_label(
        _("Add files from the whole hierarchy"));
    gtk_widget_set_visible(subdirs, TRUE);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser), subdirs);

    if (last_input_location) {
        GFile *dir = g_file_new_for_path(last_input_location);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),
                                            dir, NULL);
        g_object_unref(dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gboolean   recurse =
            gtk_check_button_get_active(GTK_CHECK_BUTTON(subdirs));
        GFile *cur = gtk_file_chooser_get_current_folder(
            GTK_FILE_CHOOSER(chooser));
        g_free(last_input_location);
        last_input_location = cur ? g_file_get_path(cur) : NULL;
        if (cur) g_object_unref(cur);

        GListModel *files =
            gtk_file_chooser_get_files(GTK_FILE_CHOOSER(chooser));
        guint n = g_list_model_get_n_items(files);
        for (guint i = 0; i < n; i++) {
            GFile *f = G_FILE(g_list_model_get_item(files, i));
            gchar *p = g_file_get_path(f);
            if (p) add_input_folder(p, GINT_TO_POINTER(recurse));
            g_object_unref(f);
        }
        g_object_unref(files);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

static char *
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

static void
open_outputfolder_chooser(GtkWidget *widget, gpointer data)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        _("Select output folder"), NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_OK"),     GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (selected_source_folder) {
        GFile *dir = g_file_new_for_path(selected_source_folder);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),
                                            dir, NULL);
        g_object_unref(dir);
    }

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GFile *f   = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
        gchar *path = g_file_get_path(f);
        if (path) {
            g_free(bimp_output_folder);
            bimp_output_folder = path;
            gtk_button_set_label(GTK_BUTTON(button_outfolder),
                                 get_outputfolder_name());
            gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);
        }
        g_object_unref(f);
    }
    gtk_window_destroy(GTK_WINDOW(chooser));
}

static void
set_source_output_folder(GtkWidget *widget, gpointer data)
{
    if (selected_source_folder) {
        g_free(bimp_output_folder);
        bimp_output_folder = g_strdup(selected_source_folder);
        gtk_button_set_label(GTK_BUTTON(button_outfolder),
                             get_outputfolder_name());
        gtk_widget_set_tooltip_text(button_outfolder, bimp_output_folder);
    }
}

/* -------------------------------------------------------------------------
 * Popover factory helpers
 *
 * GTK4 GtkPopover must have a fixed parent widget. We create a fresh
 * popover each click, then auto-destroy it via the "closed" signal.
 * ---------------------------------------------------------------------- */

static void
popover_on_closed(GtkWidget *popover, gpointer data)
{
    gtk_widget_unparent(popover);
}

static void
popup_and_track(GtkWidget *popover, GtkWidget *parent)
{
    gtk_widget_set_parent(popover, parent);
    g_signal_connect(popover, "closed",
                     G_CALLBACK(popover_on_closed), NULL);
    gtk_popover_popup(GTK_POPOVER(popover));
}

/* Create a frameless button with optional click callback */
static GtkWidget *
popover_button(const char *label, GCallback cb, gpointer cb_data)
{
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
    if (cb) g_signal_connect(btn, "clicked", cb, cb_data);
    return btn;
}

/* Dismiss the popover that contains 'widget' */
static void
close_popover_ancestor(GtkWidget *widget, gpointer data)
{
    GtkWidget *pop = gtk_widget_get_ancestor(widget, GTK_TYPE_POPOVER);
    if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
}

static GtkWidget *
build_add_popover(GtkWidget *parent)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    for (int id = 0; id < MANIP_END; id++) {
        GtkWidget *btn = popover_button(bimp_manip_get_string(id),
                                        G_CALLBACK(add_manipulation_from_id),
                                        GINT_TO_POINTER(id));
        g_signal_connect(btn, "clicked",
                         G_CALLBACK(close_popover_ancestor), NULL);
        gtk_box_append(GTK_BOX(box), btn);
    }

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *save_btn = popover_button(_("Save this set..."),
                                         G_CALLBACK(save_set), NULL);
    g_signal_connect(save_btn, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), save_btn);

    GtkWidget *load_btn = popover_button(_("Load set..."),
                                         G_CALLBACK(load_set), NULL);
    g_signal_connect(load_btn, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), load_btn);

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static GtkWidget *
build_edit_popover(GtkWidget *parent, manipulation man)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    clicked_man = man;

    GtkWidget *edit_btn = popover_button(_("Edit properties..."),
                                         G_CALLBACK(edit_clicked_manipulation),
                                         NULL);
    g_signal_connect(edit_btn, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), edit_btn);

    GtkWidget *rem_btn = popover_button(_("Remove this manipulation"),
                                        G_CALLBACK(remove_clicked_manipulation),
                                        NULL);
    g_signal_connect(rem_btn, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), rem_btn);

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static GtkWidget *
build_addfiles_popover(GtkWidget *parent)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *b1 = popover_button(_("Add single images..."),
                                    G_CALLBACK(open_file_chooser), NULL);
    g_signal_connect(b1, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b1);

    GtkWidget *b2 = popover_button(_("Add folders..."),
                                    G_CALLBACK(open_folder_chooser), NULL);
    g_signal_connect(b2, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b2);

    GtkWidget *b3 = popover_button(_("Add all opened images"),
                                    G_CALLBACK(add_opened_files), NULL);
    g_signal_connect(b3, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
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
    g_signal_connect(b1, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b1);

    GtkWidget *b2 = popover_button(_("Remove all"),
                                    G_CALLBACK(remove_all_input_files), NULL);
    g_signal_connect(b2, "clicked",
                     G_CALLBACK(close_popover_ancestor), NULL);
    gtk_box_append(GTK_BOX(box), b2);

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static void
open_manipulation_popover(GtkWidget *widget, gpointer data)
{
    GtkWidget *pop = (data == NULL)
        ? build_add_popover(widget)
        : build_edit_popover(widget, (manipulation)data);
    popup_and_track(pop, widget);
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

/* -------------------------------------------------------------------------
 * Manipulation callbacks
 * ---------------------------------------------------------------------- */

static void
add_manipulation_from_id(GtkWidget *widget, gpointer id)
{
    manipulation newman =
        bimp_append_manipulation((manipulation_type)GPOINTER_TO_INT(id));

    if (!newman) {
        bimp_show_error_dialog(
            _("Can't add another manipulation of this kind. Only one is permitted!"),
            bimp_window_main);
    } else {
        bimp_refresh_sequence_panel();
        GtkAdjustment *hadj =
            gtk_scrolled_window_get_hadjustment(
                GTK_SCROLLED_WINDOW(scroll_sequence));
        gtk_adjustment_set_value(hadj, gtk_adjustment_get_upper(hadj));
        bimp_open_editwindow(newman, TRUE);
    }
}

static void
edit_clicked_manipulation(GtkWidget *widget, gpointer data)
{
    if (clicked_man) bimp_open_editwindow(clicked_man, FALSE);
}

static void
remove_clicked_manipulation(GtkWidget *widget, gpointer data)
{
    if (clicked_man) {
        bimp_remove_manipulation(clicked_man);
        g_free(clicked_man);
        clicked_man = NULL;
        bimp_refresh_sequence_panel();
    }
}

/* -------------------------------------------------------------------------
 * Sequence panel refresh
 * ---------------------------------------------------------------------- */

void
bimp_refresh_sequence_panel(void)
{
    GtkWidget *child;
    GtkWidget *add_btn;

    while ((child = gtk_widget_get_first_child(hbox_sequence)))
        gtk_widget_unparent(child);

    g_slist_foreach(bimp_selected_manipulations,
                    (GFunc)add_manipulation_button, NULL);

    add_btn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(add_btn, SEQ_BUTTON_W - 20, SEQ_BUTTON_H);
    gtk_box_append(GTK_BOX(hbox_sequence), add_btn);
    g_signal_connect(add_btn, "clicked",
                     G_CALLBACK(open_manipulation_popover), NULL);
}

static void
add_manipulation_button(manipulation man)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *vbox   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *icon   = image_new_from_resource(man->icon);
    GtkWidget *label  = gtk_label_new(bimp_manip_get_string(man->type));

    gtk_box_append(GTK_BOX(vbox), icon);
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_button_set_child(GTK_BUTTON(button), vbox);
    gtk_widget_set_size_request(button, SEQ_BUTTON_W, SEQ_BUTTON_H);
    gtk_box_append(GTK_BOX(hbox_sequence), button);
    g_signal_connect(button, "clicked",
                     G_CALLBACK(open_manipulation_popover), man);
}

/* -------------------------------------------------------------------------
 * Set save / load
 * ---------------------------------------------------------------------- */

static void
save_set(GtkWidget *widget, gpointer data)
{
    if (g_slist_length(bimp_selected_manipulations) == 0) {
        bimp_show_error_dialog(
            _("The manipulations set is empty!"), bimp_window_main);
        return;
    }

    GtkWidget *saver = gtk_file_chooser_dialog_new(
        _("Save this set..."), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Save"),   GTK_RESPONSE_ACCEPT,
        NULL
    );
    GtkFileFilter *f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, "BIMP set (*.bimp)");
    gtk_file_filter_add_pattern(f, "*.bimp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(saver), f);

    if (gtk_dialog_run(GTK_DIALOG(saver)) == GTK_RESPONSE_ACCEPT) {
        GFile *gf   = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(saver));
        gchar *path = g_file_get_path(gf);
        g_object_unref(gf);
        gtk_window_destroy(GTK_WINDOW(saver));

        if (path) {
            gchar *out_path = g_str_has_suffix(path, ".bimp")
                ? g_strdup(path)
                : g_strconcat(path, ".bimp", NULL);
            g_free(path);
            if (!bimp_serialize_to_file(out_path)) {
                bimp_show_error_dialog(
                    _("An error occurred when saving the batch file."),
                    bimp_window_main);
            }
            g_free(out_path);
        }
        return;
    }
    gtk_window_destroy(GTK_WINDOW(saver));
}

static void
load_set(GtkWidget *widget, gpointer data)
{
    if (g_slist_length(bimp_selected_manipulations) > 0) {
        GtkWidget *q = gtk_message_dialog_new(
            GTK_WINDOW(bimp_window_main),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            _("This will overwrite the current manipulations set. Continue?")
        );
        gtk_window_set_title(GTK_WINDOW(q), _("Continue?"));
        gint r = gtk_dialog_run(GTK_DIALOG(q));
        gtk_window_destroy(GTK_WINDOW(q));
        if (r != GTK_RESPONSE_YES) return;
    }

    GtkWidget *loader = gtk_file_chooser_dialog_new(
        _("Load set..."), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Open"),   GTK_RESPONSE_ACCEPT,
        NULL
    );
    GtkFileFilter *f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, "BIMP set (*.bimp)");
    gtk_file_filter_add_pattern(f, "*.bimp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(loader), f);

    if (gtk_dialog_run(GTK_DIALOG(loader)) == GTK_RESPONSE_ACCEPT) {
        GFile *gf   = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(loader));
        gchar *path = g_file_get_path(gf);
        g_object_unref(gf);
        gtk_window_destroy(GTK_WINDOW(loader));

        if (path) {
            if (!bimp_deserialize_from_file(path)) {
                bimp_show_error_dialog(
                    _("An error occurred when loading the batch file."),
                    bimp_window_main);
            } else {
                bimp_refresh_sequence_panel();
            }
            g_free(path);
        }
        return;
    }
    gtk_window_destroy(GTK_WINDOW(loader));
}

/* -------------------------------------------------------------------------
 * About dialog
 * ---------------------------------------------------------------------- */

static void
open_about(void)
{
    const gchar *authors[] = {
        "Alessandro Francesconi <alessandrofrancesconi@live.it>",
        NULL
    };
    const gchar *license =
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version.\n\n"
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
        "GNU General Public License for more details.";

    gtk_show_about_dialog(
        GTK_WINDOW(bimp_window_main),
        "program-name",  PLUG_IN_FULLNAME,
        "version",       PLUG_IN_VERSION,
        "comments",      _("Applies GIMP manipulations on groups of images"),
        "logo",          pixbuf_new_from_resource(
                             "/gimp/plugin/bimp/icons/bimp-icon.png"),
        "copyright",     PLUG_IN_COPYRIGHT,
        "license",       license,
        "wrap-license",  TRUE,
        "website",       PLUG_IN_WEBSITE,
        "authors",       authors,
        NULL
    );
}

/* -------------------------------------------------------------------------
 * Error dialog
 * ---------------------------------------------------------------------- */

void
bimp_show_error_dialog(char *message, GtkWidget *parent)
{
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_window_destroy(GTK_WINDOW(dialog));
}

/* -------------------------------------------------------------------------
 * Progress bar
 * ---------------------------------------------------------------------- */

static const gchar *
progressbar_init_hidden(void)
{
    GimpProgressVtable vt = { 0, };
    vt.start     = progressbar_start_hidden;
    vt.end       = progressbar_end_hidden;
    vt.set_text  = progressbar_settext_hidden;
    vt.set_value = progressbar_setvalue_hidden;
    return gimp_progress_install_vtable(&vt, NULL);
}

static void progressbar_start_hidden(const gchar *m, gboolean c, gpointer u)
{ (void)m; (void)c; (void)u; }
static void progressbar_end_hidden(gpointer u)
{ (void)u; }
static void progressbar_settext_hidden(const gchar *m, gpointer u)
{ (void)m; (void)u; }
static void progressbar_setvalue_hidden(double p, gpointer u)
{ (void)p; (void)u; }

void
bimp_progress_bar_set(double fraction, char *text)
{
    if (fraction > 1.0) fraction = 1.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar_visible),
                                  fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar_visible),
                              text ? text : " ");
}

/* -------------------------------------------------------------------------
 * Busy state — show Apply or Stop button depending on busy flag
 * ---------------------------------------------------------------------- */

void
bimp_set_busy(gboolean busy)
{
    GtkWidget *apply_btn, *stop_btn;

    bimp_is_busy = busy;

    gtk_dialog_set_response_sensitive(GTK_DIALOG(bimp_window_main),
                                      GTK_RESPONSE_CLOSE, !busy);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(bimp_window_main),
                                      GTK_RESPONSE_HELP, !busy);

    apply_btn = gtk_dialog_get_widget_for_response(
        GTK_DIALOG(bimp_window_main), GTK_RESPONSE_APPLY);
    if (apply_btn) gtk_widget_set_visible(apply_btn, !busy);

    stop_btn = gtk_dialog_get_widget_for_response(
        GTK_DIALOG(bimp_window_main), GTK_RESPONSE_CANCEL);
    if (stop_btn) gtk_widget_set_visible(stop_btn, busy);

    gtk_widget_set_sensitive(panel_sequence, !busy);
    gtk_widget_set_sensitive(panel_options,  !busy);
}
