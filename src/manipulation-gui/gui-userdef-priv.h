#ifndef GUI_USERDEF_PRIV_H
#define GUI_USERDEF_PRIV_H

#include <gtk/gtk.h>
#include "../bimp-manipulations.h"

extern GtkWidget        *treeview_procedures;
extern GtkWidget        *scroll_procparam;
extern GtkWidget        *parent_dialog;
extern userdef_settings  temp_settings;
extern GtkWidget       **param_widget;
extern GtkTreeSelection *treesel_proc;

void update_procedure_box(userdef_settings);

#endif
