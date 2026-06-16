# BIMP Rebuild Changelog — GIMP 3.x Port

**Branch:** `feature/gimp3-port`
**Base:** BIMP 2.6 (GIMP 2.x / GTK 2/3)
**Target:** GIMP 3.x / GTK 4 / GEGL

---

## Phase 2 — Code Structure Refactor
**Commit:** `ce9ad90`
**Date:** 2026-06-16

Split all oversized source files to comply with project coding rules
(≤ 300 lines per file, ≤ 50 lines per function, no commented-out code).

### `src/bimp-operate.c` (1,443 lines → 7 files)

| New File | Lines | Contents |
|---|---|---|
| `bimp-operate-priv.h` | 57 | Internal shared-state `extern` declarations and cross-module function declarations |
| `bimp-operate.c` | 165 | Batch driver (`bimp_start_batch`, `bimp_init_batch`), shared state variables, manipulation dispatch |
| `bimp-operate-process.c` | 237 | `bimp_process_image`, output-path helpers (`get_output_ext`, `compute_output_dir`, `overwrite_result`, `do_apply_rename`) |
| `bimp-operate-apply.c` | 260 | `apply_resize`, `apply_crop`, `apply_fliprotate`, `apply_color`, `bimp_apply_color_reset`, `merge_visible`, `first_layer` |
| `bimp-operate-fx.c` | 245 | `apply_sharpblur`, `apply_watermark`, `apply_userdef`, `gegl_color_from_gdkrgba`, `bimp_apply_drawable_manipulations` |
| `bimp-operate-save.c` | 212 | `image_save` dispatcher, `image_save_bmp`, `image_save_gif`, `image_save_ico`, `image_save_jpeg`, `build_save_args` |
| `bimp-operate-formats.c` | 179 | `image_save_png`, `image_save_tga`, `image_save_tiff`, `image_save_heif`, `image_save_webp`, `image_save_avif`, `image_save_exr` |

### `src/bimp-gui.c` (1,264 lines → 6 files)

| New File | Lines | Contents |
|---|---|---|
| `bimp-gui-priv.h` | 65 | Internal widget `extern` declarations and cross-module function declarations |
| `bimp-gui.c` | 247 | Main dialog (`bimp_show_gui`, `create_main_dialog`, `setup_main_content`, `run_response_loop`), progress bar, error dialog, busy state, popover helpers |
| `bimp-gui-filelist.c` | 102 | `init_fileview`, `bimp_refresh_fileview`, `get_treeview_selection`, `remove_input_file`, `remove_all_input_files`, `select_filename` |
| `bimp-gui-chooser.c` | 229 | `open_file_chooser`, `open_folder_chooser`, `add_opened_files`, `open_outputfolder_chooser`, `set_source_output_folder`, recursive folder-add helpers |
| `bimp-gui-sequence.c` | 232 | `bimp_sequence_panel_new`, `bimp_refresh_sequence_panel`, `open_manipulation_popover`, add/edit/remove manipulation, save/load set |
| `bimp-gui-panel.c` | 280 | `bimp_option_panel_new`, `get_outputfolder_name`, `update_selection`, `show_preview`, file-list section builder, user-options section builder |

### `src/manipulation-gui/gui-changeformat.c` (540 lines → 3 files)

| New File | Lines | Contents |
|---|---|---|
| `gui-changeformat-priv.h` | 35 | Widget variable `extern` declarations; per-format builder function declarations |
| `gui-changeformat.c` | 156 | `bimp_changeformat_gui_new`, `update_frame_params` dispatcher, `bimp_changeformat_save` and its per-format save helpers |
| `gui-changeformat-build.c` | 294 | Per-format inner widget builders: `build_gif_inner`, `build_jpeg_inner`, `build_jpeg_advanced_box`, `build_png_inner`, `build_tga_inner`, `build_tiff_inner`, `build_heif_inner`, `build_webp_inner`, `build_avif_inner` |

### `src/manipulation-gui/gui-userdef.c` (579 lines → 4 files)

| New File | Lines | Contents |
|---|---|---|
| `gui-userdef-priv.h` | 16 | Shared widget/state `extern` declarations; `update_procedure_box` declaration |
| `gui-userdef.c` | 237 | `bimp_userdef_gui_new` and its setup helpers (`setup_procedure_chooser`, `build_chooser_grid`, `restore_from_settings`), procedure list management (`init_procedure_list`, `fill_procedure_list`, `search_procedure`, `select_procedure`), `update_selected_procedure`, `init_temp_settings` |
| `gui-userdef-params.c` | 269 | `update_procedure_box` and all parameter-widget builders: `make_param_widget`, `build_int_combo`, `build_int_spin`, `build_int_param`, `build_double_param`, `build_proc_info_labels`, `build_param_rows` |
| `gui-userdef-save.c` | 76 | `bimp_userdef_save`, `save_param_value` |

### Structural notes
- Shared state variables in split modules are declared non-static and exposed via private headers (`-priv.h`) using `extern`.
- The Makefile wildcard build (`src/*.c src/manipulation-gui/*.c`) picks up all new `.c` files automatically — no Makefile changes required.
- `bimp_apply_color_reset()` replaces the previous file-static `colorcurve_init` flag, now callable across modules.
- Popover helpers (`popup_and_track`, `popover_button`, `close_popover_ancestor`, `popover_on_closed`) live in `bimp-gui.c` and are declared in `bimp-gui-priv.h` for use by `bimp-gui-sequence.c` and `bimp-gui-panel.c`.

---

## Phase 1 — GIMP 3.x / GTK 4 Port
**Commits:** `fb65687` through `f546bd4`
**Date:** 2026-06-16

Complete port of BIMP 2.6 from GIMP 2.x / GTK 2-3 to GIMP 3.x / GTK 4.

### API Migration Summary

| Old (GIMP 2.x) | New (GIMP 3.x) |
|---|---|
| `GimpPlugInInfo` + `MAIN()` + `query()`/`run()` | `GimpPlugIn` GObject subclass with `create_procedure()` / `run()` vfuncs |
| `gint32` image/drawable/layer IDs | `GimpImage*` / `GimpDrawable*` / `GimpLayer*` object pointers |
| `GimpParam` / `GimpParamDef` arrays | `GimpValueArray` / `GParamSpec*` |
| `gimp_run_procedure()` | `gimp_pdb_run_procedure()` |
| `gimp_procedural_db_query()` | `gimp_pdb_query_procedures()` |
| `gimp_image_get_layers()` returning `gint*` | `gimp_image_get_layers()` returning `GList*` |
| `GimpRGB` | `GeglColor*` |
| `GdkColor` | `GdkRGBA` |
| `GTK_STOCK_*` icon constants | Named icon strings (`"list-add"`, etc.) |
| `GtkMenu` + `GtkMenuItem` | `GtkPopover` |
| `gtk_vbox_new()` / `gtk_hbox_new()` | `gtk_box_new(GTK_ORIENTATION_VERTICAL/HORIZONTAL, …)` |
| `gtk_container_add()` | `gtk_box_append()` / widget-specific setters |
| `gtk_widget_show_all()` | `gtk_widget_set_visible()` per widget |
| `GtkRadioButton` groups | `gtk_check_button_set_group()` |
| `gimp_ui_init(name, FALSE)` | `gimp_ui_init(name)` |
| `gimp_image_delete()` | `g_object_unref()` |
| `gimptool-2.0` | `gimptool-3.0` |

### File-by-file changes

#### `src/bimp.h` · `src/bimp-operate.h` · `src/bimp-manipulations.h` · `src/bimp-utils.h/.c`
*(commit `fb65687`)*
- Replaced `GimpParam*` with `GimpValueArray*` in `manip_userdef_set`
- `gint32` image/drawable ID fields replaced with `GimpImage*` / `GimpDrawable*` / `GimpLayer*` in `image_output`
- `GdkColor` → `GdkRGBA` in `manip_color_set` and watermark settings
- Removed `GimpParamDef` usage from `bimp-utils`; updated `init_supported_procedures()` to use `gimp_pdb_query_procedures()` and `GParamSpec*`
- Replaced `GdkWindow` reference with `GdkSurface`

#### `src/bimp.c`
*(commit `fc26a14`)*
- Full rewrite as a `GimpPlugIn` GObject subclass (`BimpPlugin`)
- Implements `create_procedure()` and `run()` vfuncs
- Removed legacy `MAIN()` macro, `query()` function, and `GimpPlugInInfo`
- Plugin registered as `GIMP_PDB_PROC_TYPE_PLUGIN` with correct `GParamSpec` argument definitions
- Entry point via `gimp_main(BIMP_TYPE_PLUGIN, argc, argv)`

#### `src/bimp-operate.c`
*(commit `aa8897c`)*
- All `gint32` IDs replaced with `GimpImage*`, `GimpDrawable*`, `GimpLayer*`, `GList*`
- All procedure calls migrated from `gimp_run_procedure()` + `GimpParam[]` to `gimp_pdb_run_procedure()` + `GimpValueArray`
- Save procedures updated to use `GFile*` (via `g_file_new_for_path()`) instead of bare filename strings
- Color handling updated to use `GeglColor*` and `GdkRGBA`
- `gimp_image_get_layers()` return handling updated for `GList*`

#### `src/bimp-manipulations.c`
*(commit `d082217`)*
- `GdkColor` → `GdkRGBA` throughout color manipulation
- `GimpRGB` → `GeglColor*` for color curve and color settings
- Color parsing updated for GIMP 3.x color APIs
- Backward-compatible color import retained for existing `.bimp` set files

#### `src/bimp-gui.c`
*(commit `c53e7b7`)*
- Replaced all `GTK_STOCK_*` with named icon strings
- `GtkMenu` / `GtkMenuItem` replaced with `GtkPopover` and `GtkButton`
- `gtk_vbox_new()` / `gtk_hbox_new()` replaced with `gtk_box_new()`
- `gtk_container_add()` replaced with `gtk_box_append()` and widget-specific child setters
- `gtk_widget_show_all()` replaced with per-widget `gtk_widget_set_visible()`
- Dialog button wiring updated for GTK 4 response API
- `gimp_ui_init()` call updated (removed second `FALSE` argument)

#### `src/manipulation-gui/*.c` (8 files)
*(commit `9ca4660`)*
- `GtkRadioButton` groups replaced with `gtk_check_button_set_group()`
- GTK 4 color button APIs adopted (`GdkRGBA` throughout)
- Deprecated widget constructors updated across all 8 GUI files
- `gui-userdef.c`: `GimpParamDef` / `GimpParam` replaced with `GParamSpec*` / `GimpValueArray`; procedure parameter widget building rewritten for GIMP 3.x PDB introspection
- `gui-colorcurve.c`: curve file loading updated for GIMP 3.x curve API

#### `src/bimp-serialize.c`
*(commit `b69ceba`)*
- `GdkColor` → `GdkRGBA` in color serialization
- `parse_color_compat()` added for backward compatibility when reading `.bimp` files created by BIMP 2.x
- `write_userdef()` / `read_userdef()` rewritten to serialize `GimpValueArray` (replaces old `GimpParam[]` format)

#### `Makefile`
*(commit `b69ceba`)*
- `gimptool-2.0` replaced with `gimptool-3.0`
- Removed `-DGIMP_DISABLE_DEPRECATED` flag (no longer needed)
- Wildcard source pattern (`src/*.c src/manipulation-gui/*.c`) retained, automatically includes all new Phase 2 split files

---

## Baseline
**Commit:** `1dbb78d`

Initial import of BIMP 2.6 source as pre-port baseline (unmodified from upstream).
