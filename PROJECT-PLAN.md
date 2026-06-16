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

Legend: ✅ Done · 🔲 Pending

| # | File(s) | What Changes | Status |
|---|---|---|---|
| 1 | `PROJECT-PLAN.md`, `AGENTS.md` | Docs setup | ✅ 71fa293 |
| 2 | `src/bimp.h` | Remove `GimpParam` reference from `manip_userdef_set` | ✅ fb65687 |
| 3 | `src/bimp-operate.h` | `gint32` → `GimpImage*`/`GList*` in `image_output` struct | ✅ fb65687 |
| 4 | `src/bimp-manipulations.h` | `GdkColor` → `GdkRGBA`, `GimpParam*` → `GimpValueArray*` | ✅ fb65687 |
| 5 | `src/bimp-utils.h` / `bimp-utils.c` | Remove `GimpParamDef`, fix `GdkWindow` reference | ✅ fb65687 |
| 6 | `src/bimp.c` | Full rewrite: GObject plugin class, new query/run pattern | ✅ fc26a14 |
| 7 | `src/bimp-operate.c` | ID→object types, `GimpValueArray`, save procedure names, `GFile*` | ✅ aa8897c |
| 8 | `src/bimp-manipulations.c` | `GdkColor`→`GdkRGBA`, `GimpRGB`→`GeglColor` | ✅ d082217 |
| 9 | `src/bimp-gui.c` | GTK4: stock items, `GtkMenu`→`GtkPopover`, dialog APIs, `gimp_ui_init` | ✅ c53e7b7 |
| 10 | `src/bimp-manipulations-gui.c` | GTK4: stock items, `gtk_vbox_new` removed | ✅ 9ca4660 |
| 11 | `src/manipulation-gui/*.c` (8 files) | GTK4: `GtkRadioButton`→`GtkCheckButton`, color buttons, deprecated widgets; `GimpParamDef`/`GimpParam`→`GParamSpec*`/`GimpValueArray` in gui-userdef | ✅ 9ca4660 |
| 12 | `src/bimp-serialize.c` | `GdkColor`→`GdkRGBA`; `parse_color_compat()` for backward compat; `write/read_userdef` rewritten for `GimpValueArray` | ✅ b69ceba |
| 13 | `Makefile` | `gimptool-2.0` → `gimptool-3.0`; drop `-DGIMP_DISABLE_DEPRECATED` | ✅ b69ceba |

## Phase 2 — Post-Port Refactor (Pending)

Files exceeding the 300-line limit that must be split into sub-modules:

| File | Lines | Split Plan |
|---|---|---|
| `src/bimp-gui.c` | ~680 | Extract file-list panel, manipulation-list panel, preview logic |
| `src/bimp-operate.c` | ~500+ | Extract per-manipulation apply functions into `bimp-operate-*.c` |
| `src/manipulation-gui/gui-changeformat.c` | ~430 | Extract per-format panel builders |
| `src/manipulation-gui/gui-userdef.c` | ~380 | Extract procedure-list and param-widget builders |

Each split must keep all functions ≤ 50 lines, files ≤ 300 lines, and no commented-out code.

## Definition of Done

### Phase 1 (Port) — COMPLETE
- [x] All source files ported to GIMP 3.x libgimp API and GTK4
- [x] All 8 commits on `feature/gimp3-port` branch, one logical unit per commit
- [x] Build system updated to `gimptool-3.0`
- [x] No `GdkColor`, `GimpParam`, `GimpParamDef`, `GtkMenu`, `GTK_STOCK_*`, or `gtk_vbox/hbox_new` remaining

### Phase 2 (Refactor) — PENDING
- [ ] All files ≤ 300 lines
- [ ] All functions ≤ 50 lines
- [ ] No comments except single-line non-obvious WHY notes
- [ ] Plugin loads in GIMP 3.x and appears in File menu
- [ ] Batch operations (resize, crop, flip, color, watermark, format change) run to completion
