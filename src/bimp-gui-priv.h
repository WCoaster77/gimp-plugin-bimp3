#ifndef BIMP_GUI_PRIV_H
#define BIMP_GUI_PRIV_H

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "bimp-manipulations.h"
#include "bimp-gui.h"

/* bimp-gui.c: main window and panels */
extern GtkWidget    *panel_sequence;
extern GtkWidget    *panel_options;
extern GtkWidget    *progressbar_visible;
extern const gchar  *progressbar_data;

/* bimp-gui-filelist.c: file list treeview */
extern GtkWidget *treeview_files;

/* bimp-gui-panel.c: option panel widgets */
extern GtkWidget *button_preview;
extern GtkWidget *button_outfolder;
extern GtkWidget *button_samefolder;
extern GtkWidget *check_keepfolderhierarchy;
extern GtkWidget *check_deleteondone;
extern GtkWidget *check_keepdates;
extern char      *selected_source_folder;
extern char      *last_input_location;

/* bimp-gui-sequence.c: sequence panel widgets */
extern GtkWidget    *hbox_sequence;
extern GtkWidget    *scroll_sequence;
extern manipulation  clicked_man;

/* bimp-gui.c — popover helpers used across split modules */
void       popover_on_closed(GtkWidget *, gpointer);
void       popup_and_track(GtkWidget *, GtkWidget *);
GtkWidget *popover_button(const char *, GCallback, gpointer);
void       close_popover_ancestor(GtkWidget *, gpointer);

/* bimp-gui-panel.c */
GtkWidget *bimp_option_panel_new(void);
char      *get_outputfolder_name(void);
void       update_selection(char *);
void       show_preview(GtkWidget *, gpointer);

/* bimp-gui-sequence.c */
GtkWidget *bimp_sequence_panel_new(void);
void       open_manipulation_popover(GtkWidget *, gpointer);

/* bimp-gui-filelist.c */
void       init_fileview(void);
GSList    *get_treeview_selection(void);
void       remove_input_file(GtkWidget *, gpointer);
void       remove_all_input_files(GtkWidget *, gpointer);
void       select_filename(GtkTreeSelection *, gpointer);

/* bimp-gui-chooser.c */
void       open_file_chooser(GtkWidget *, gpointer);
void       open_folder_chooser(GtkWidget *, gpointer);
void       add_opened_files(GtkWidget *, gpointer);
void       open_outputfolder_chooser(GtkWidget *, gpointer);
void       set_source_output_folder(GtkWidget *, gpointer);

#endif
