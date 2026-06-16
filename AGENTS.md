# AGENTS.md — BIMP GIMP 3.x Port

Extends vibe-code-rules.md. Read both before writing any code.

## Project Context

Porting the BIMP batch image manipulation plugin from GIMP 2.x (GTK2/3) to GIMP 3.x (GTK4).
Source base: https://github.com/alessandrofrancesconi/gimp-plugin-bimp
Target API: GIMP 3.x libgimp + GTK4 + GEGL

See PROJECT-PLAN.md for the full migration map and build order.

## Directory Structure

```
gimp-plugin-bimp3/
  src/
    bimp.c / bimp.h              # Plugin entry point (GimpPlugIn subclass)
    bimp-operate.c / .h          # Batch execution engine (image load/process/save)
    bimp-manipulations.c / .h    # Manipulation data structures and constructors
    bimp-manipulations-gui.c / .h # Manipulation edit dialog dispatcher
    bimp-gui.c / .h              # Main plugin window
    bimp-utils.c / .h            # Shared utilities
    bimp-serialize.c / .h        # Settings save/load
    manipulation-gui/            # Per-manipulation edit UI panels
      gui-resize.c/h
      gui-crop.c/h
      gui-fliprotate.c/h
      gui-color.c/h
      gui-sharpblur.c/h
      gui-watermark.c/h
      gui-changeformat.c/h
      gui-rename.c/h
      gui-userdef.c/h
    images/                      # Embedded icon resources
  bimp-locale/                   # Gettext translation files
  Makefile
  PROJECT-PLAN.md
```

## Language and Compiler

- C99, compiled with gcc via `gimptool-3.0 --cflags / --libs`
- All headers: `#include <libgimp/gimp.h>`, `#include <gtk/gtk.h>`
- Do NOT include deprecated headers: `<libgimpbase/gimpbase.h>` is fine; `<gdk-pixbuf/gdk-pixdata.h>` may be removed in GTK4 — check before use

## Critical Type Changes

```c
/* OLD (2.x)                      NEW (3.x) */
gint32 image_id                   GimpImage *image
gint32 drawable_id                GimpDrawable *drawable
gint32 layer_id                   GimpLayer *layer
gint *drawable_ids + gint count   GList *layers  (GimpLayer* elements)
GimpRGB                           GeglColor *
GdkColor                          GdkRGBA
GimpParam                         GimpValueArray (via GValue)
GimpParamDef                      GParamSpec (or introspect via GimpPDB)
```

## API Naming Changes

```c
gimp_image_width(id)              gimp_image_get_width(image)
gimp_image_height(id)             gimp_image_get_height(image)
gimp_drawable_width(id)           gimp_drawable_get_width(drawable)
gimp_drawable_height(id)          gimp_drawable_get_height(drawable)
gimp_image_base_type(id)          gimp_image_get_base_type(image)
gimp_image_delete(id)             g_object_unref(image)
gimp_run_procedure(...)           gimp_pdb_run_procedure(pdb, name, args)
gimp_run_procedure2(...)          gimp_pdb_run_procedure_argv(pdb, name, args)
gimp_procedural_db_query(...)     gimp_pdb_query_procedures(pdb, ...)
gimp_procedural_db_proc_info(...) gimp_pdb_lookup_procedure(pdb, name)
gimp_ui_init(name, FALSE)         gimp_ui_init(name)
```

## GTK4 Rules

- `GTK_STOCK_*` constants are removed. Use named icon strings: `"gtk-ok"` → `"_OK"` button label, or icon name like `"document-save"`.
- `gtk_vbox_new()` / `gtk_hbox_new()` removed. Use `gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)`.
- `gtk_container_add()` / `gtk_container_set_border_width()` removed. Use `gtk_box_append()` and CSS/margins.
- `gtk_dialog_get_action_area()` removed. Use `gtk_dialog_get_content_area()` with manually added button boxes, or `gtk_dialog_new_with_buttons()`.
- `GdkWindow` → `GdkSurface`
- `GdkColor` completely removed. Use `GdkRGBA` and `gdk_rgba_parse()`.
- `gtk_widget_destroy()` → `gtk_window_destroy()` for top-level windows; `g_object_unref()` for non-top-level.

## Plugin Registration Pattern (GIMP 3.x)

```c
/* In bimp.c — skeleton only, full implementation in source */
#define BIMP_TYPE_PLUGIN (bimp_plugin_get_type())
G_DECLARE_FINAL_TYPE (BimpPlugin, bimp_plugin, BIMP, PLUGIN, GimpPlugIn)

struct _BimpPlugin { GimpPlugIn parent_instance; };

static GList * bimp_query_procedures (GimpPlugIn *plug_in);
static GimpProcedure * bimp_create_procedure (GimpPlugIn *plug_in, const gchar *name);

G_DEFINE_TYPE (BimpPlugin, bimp_plugin, GIMP_TYPE_PLUG_IN)
GIMP_MAIN (BIMP_TYPE_PLUGIN)

static void bimp_plugin_class_init (BimpPluginClass *klass) {
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(klass);
    plug_in_class->query_procedures = bimp_query_procedures;
    plug_in_class->create_procedure = bimp_create_procedure;
}
```

## Coding Rules (project-specific)

- Never use `gint32` for image, drawable, or layer identifiers — always the typed pointer.
- When iterating layers from `gimp_image_get_layers()`, cast list data: `GIMP_LAYER(node->data)`.
- File paths for save/load use `GFile *` in GIMP 3.x — convert `char*` paths via `g_file_new_for_path()`.
- Always `g_list_free()` (not `g_free()`) after consuming a `GList*` from GIMP APIs.
- Keep the 300-line / 50-line-per-function rules in mind; flag violations for the post-port refactor.
- No TypeScript `any` rule translates to: no void* casts where a typed pointer can be used.

## Commit Convention

```
feat: <description>      # new port section complete
fix: <description>       # compile error or runtime fix
refactor: <description>  # post-port file splits
chore: <description>     # build system, docs
```

## Branch

All work on `feature/gimp3-port`. Never commit to `main`.
