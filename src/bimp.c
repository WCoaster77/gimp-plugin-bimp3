/*
 * BIMP - Batch Image Manipulation Plugin for GIMP
 *
 * (C) 2018 - Alessandro Francesconi
 * http://www.alessandrofrancesconi.it/projects/bimp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <libgimp/gimp.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "bimp.h"
#include "bimp-manipulations.h"
#include "bimp-gui.h"
#include "bimp-utils.h"
#include "plugin-intl.h"

/* ---------- global state -------------------------------------------------- */

GSList  *bimp_input_filenames      = NULL;
char    *bimp_output_folder        = NULL;

gint     bimp_opt_alertoverwrite   = BIMP_ASK_OVERWRITE;
gboolean bimp_opt_keepfolderhierarchy = FALSE;
gboolean bimp_opt_deleteondone     = FALSE;
gboolean bimp_opt_keepdates        = FALSE;

gboolean bimp_is_busy              = FALSE;

GSList  *bimp_supported_procedures = NULL;

/* ---------- GObject plugin class ------------------------------------------ */

#define BIMP_TYPE_PLUGIN (bimp_plugin_get_type())
G_DECLARE_FINAL_TYPE (BimpPlugin, bimp_plugin, BIMP, PLUGIN, GimpPlugIn)

struct _BimpPlugin
{
    GimpPlugIn parent_instance;
};

static GList         *bimp_query_procedures  (GimpPlugIn     *plug_in);
static GimpProcedure *bimp_create_procedure  (GimpPlugIn     *plug_in,
                                              const gchar    *name);
static GimpValueArray *bimp_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

G_DEFINE_TYPE (BimpPlugin, bimp_plugin, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BIMP_TYPE_PLUGIN)

static void
bimp_plugin_class_init (BimpPluginClass *klass)
{
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);
    plug_in_class->query_procedures = bimp_query_procedures;
    plug_in_class->create_procedure = bimp_create_procedure;
}

static void
bimp_plugin_init (BimpPlugin *plugin)
{
    (void)plugin;
}

static GList *
bimp_query_procedures (GimpPlugIn *plug_in)
{
    (void)plug_in;
    gimp_domain_register (GETTEXT_PACKAGE, get_bimp_localedir());
    return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
bimp_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
    GimpProcedure *procedure = NULL;

    if (g_strcmp0 (name, PLUG_IN_PROC) == 0)
    {
        procedure = gimp_image_procedure_new (
            plug_in, name,
            GIMP_PDB_PROC_TYPE_PLUGIN,
            bimp_run, NULL, NULL
        );

        gimp_procedure_set_image_types (procedure, "*");
        gimp_procedure_set_sensitivity_mask (procedure,
            GIMP_PROCEDURE_SENSITIVE_ALWAYS);

        gimp_procedure_set_menu_label (procedure,
            _("Batch Image Manipulation..."));
        gimp_procedure_add_menu_path (procedure, "<Image>/File/Open");

        gimp_procedure_set_documentation (
            procedure,
            PLUG_IN_FULLNAME,
            PLUG_IN_DESCRIPTION,
            name
        );
        gimp_procedure_set_attribution (
            procedure,
            "Alessandro Francesconi <alessandrofrancesconi@live.it>",
            PLUG_IN_COPYRIGHT,
            "2018"
        );
    }

    return procedure;
}

static GimpValueArray *
bimp_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
    (void)image;
    (void)n_drawables;
    (void)drawables;
    (void)config;
    (void)run_data;

    bindtextdomain (GETTEXT_PACKAGE, get_bimp_localedir());
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    switch (run_mode)
    {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
            bimp_show_gui();
            break;

        case GIMP_RUN_NONINTERACTIVE:
        default:
            g_message ("BIMP cannot run in non-interactive mode.");
            return gimp_procedure_new_return_values (
                procedure, GIMP_PDB_CALLING_ERROR, NULL);
    }

    return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

/* ---------- PDB procedure list helpers ------------------------------------ */

void
init_supported_procedures (void)
{
    if (bimp_supported_procedures != NULL) return;

    GimpPDB  *pdb        = gimp_get_pdb ();
    gchar   **proc_names = NULL;
    gint      proc_count = 0;

    proc_names = gimp_pdb_query_procedures (
        pdb,
        "^(?!.*(?:"
            "plug-in-bimp|"
            "extension-|"
            "-get-|"
            "-is-|"
            "-has-|"
            "-print-|"
            "file-glob|"
            "twain-acquire|"
            "-load|"
            "-save|"
            "-select|"
            "-free|"
            "-help|"
            "-temp|"
            "-undo|"
            "-copy|"
            "-paste|"
            "-cut|"
            "-buffer|"
            "-register|"
            "-metadata|"
            "-layer|"
            "-selection|"
            "-brush|"
            "-guide|"
            "-parasite|"
            "gimp-display|"
            "gimp-fonts|"
            "gimp-gimprc|"
            "gimp-gradient|"
            "gimp-online|"
            "gimp-palette|"
            "gimp-path|"
            "gimp-pattern|"
            "gimp-plugins|"
            "gimp-procedural|"
            "gimp-progress|"
            "gimp-quit|"
            "gimp-vectors|"
            "temp-procedure"
        ")).*",
        ".*", ".*", ".*", ".*", ".*", ".*",
        &proc_count
    );

    for (gint i = 0; i < proc_count; i++)
    {
        if (pdb_proc_has_compatible_params (pdb, proc_names[i]))
        {
            bimp_supported_procedures = g_slist_insert_sorted (
                bimp_supported_procedures,
                g_strdup (proc_names[i]),
                glib_strcmpi
            );
        }
    }

    g_strfreev (proc_names);
}

gboolean
pdb_proc_has_compatible_params (GimpPDB *pdb, const gchar *proc_name)
{
    GimpProcedure *procedure = gimp_pdb_lookup_procedure (pdb, proc_name);
    if (!procedure) return FALSE;

    gint        n_args = 0;
    GParamSpec **args  = gimp_procedure_get_arguments (procedure, &n_args);

    if (n_args == 0) return FALSE;

    for (gint i = 0; i < n_args; i++)
    {
        GType type = G_PARAM_SPEC_VALUE_TYPE (args[i]);

        if (type == G_TYPE_INT      ||
            type == G_TYPE_UINT     ||
            type == G_TYPE_DOUBLE   ||
            type == G_TYPE_FLOAT    ||
            type == G_TYPE_STRING   ||
            type == GIMP_TYPE_IMAGE ||
            type == GIMP_TYPE_DRAWABLE ||
            type == GIMP_TYPE_ITEM  ||
            type == GEGL_TYPE_COLOR)
        {
            continue;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}
