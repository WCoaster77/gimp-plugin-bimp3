# BIMP GIMP 3.x Port — Project Plan

## What Is Being Built

A port of BIMP (Batch Image Manipulation Plugin) from GIMP 2.x/GTK2-3 to GIMP 3.x/GTK4.
BIMP lets users apply a set of GIMP operations (resize, crop, color correction, watermark, format conversion, etc.) to a batch of image files through a single interactive session.

## Who It Is For

Users running GIMP 3.x who need batch image processing. The plugin must install and run identically to the original — same UI, same operations, same output behaviour.

## Scope

**In scope:**
- Port all existing functionality to GIMP 3.x libgimp API and GTK4
- Preserve all manipulation types: resize, crop, flip/rotate, color, sharpen/blur, watermark, format change, rename, user-defined
- Preserve all output formats: BMP, GIF, ICO, JPEG, PNG, TGA, TIFF, HEIF, WebP, AVIF, EXR
- Update build system (Makefile) to use `gimptool-3.0`

**Out of scope (deferred to refactor phase):**
- Splitting oversized files (bimp-operate.c, bimp-gui.c) — port first, split after
- Adding new features or manipulations
- Windows installer (nsis/) updates
- Locale/translation file updates

## Tech Stack

| Component | Version |
|---|---|
| GIMP | 3.x (master branch) |
| libgimp | 3.x (GObject-based plugin API) |
| GTK | 4.x |
| GEGL | current (replaces GimpRGB with GeglColor) |
| Language | C (C99) |
| Build tool | gcc + gimptool-3.0 |

## Key API Migration Summary

| Old (2.x) | New (3.x) |
|---|---|
| `GimpPlugInInfo` + `MAIN()` + `query()`/`run()` | `GimpPlugIn` GObject subclass |
| `gint32` image/drawable IDs | `GimpImage*` / `GimpDrawable*` / `GimpLayer*` |
| `GimpParam` arrays | `GimpValueArray` |
| `gimp_run_procedure()` | `gimp_pdb_run_procedure()` |
| `gimp_procedural_db_query()` | `gimp_pdb_query_procedures()` |
| `gimp_image_get_layers()` → `gint*` | `gimp_image_get_layers()` → `GList*` |
| `GimpRGB` | `GeglColor*` |
| `GdkColor` | `GdkRGBA` |
| `GTK_STOCK_*` | named icon strings |
| `gimp_ui_init(name, FALSE)` | `gimp_ui_init(name)` |
| `gimp_image_width/height()` | `gimp_image_get_width/height()` |
| `gimp_image_delete()` | `g_object_unref()` |
| `gimptool-2.0` | `gimptool-3.0` |

## Build Order

Each section is committed before moving to the next.

| # | File(s) | What Changes |
|---|---|---|
| 1 | `PROJECT-PLAN.md`, `AGENTS.md` | Docs setup |
| 2 | `src/bimp.h` | Remove `GimpParam` reference from `manip_userdef_set` |
| 3 | `src/bimp-operate.h` | `gint32` → `GimpImage*`/`GList*` in `image_output` struct |
| 4 | `src/bimp-manipulations.h` | `GdkColor` → `GdkRGBA`, `GimpParam*` → `GimpValueArray*` |
| 5 | `src/bimp-utils.h` / `bimp-utils.c` | Remove `GimpParamDef`, fix `GdkWindow` reference |
| 6 | `src/bimp.c` | Full rewrite: GObject plugin class, new query/run pattern |
| 7 | `src/bimp-operate.c` | ID→object types, `GValueArray`, save procedure names |
| 8 | `src/bimp-manipulations.c` | `GdkColor`→`GdkRGBA`, `GimpRGB`→`GeglColor` |
| 9 | `src/bimp-gui.c` | GTK4: stock items, dialog APIs, `gimp_ui_init` |
| 10 | `src/bimp-manipulations-gui.c` | GTK4: stock items, `gtk_vbox_new` removed |
| 11 | `src/manipulation-gui/*.c` | GTK4: color buttons, stock items, deprecated widgets |
| 12 | `src/bimp-serialize.c` | Check for any type-dependent serialization changes |
| 13 | `Makefile` | Switch to `gimptool-3.0` |
| 14 | Post-port: refactor | Split files exceeding 300 lines into modules |

## Definition of Done

- All source files compile without errors against GIMP 3.x headers
- Plugin loads in GIMP 3.x and appears in File menu
- Batch operations (resize, crop, flip, color, watermark, format change) run to completion
- All committed code is on `feature/gimp3-port` branch
- `AGENTS.md` and this plan are up to date
