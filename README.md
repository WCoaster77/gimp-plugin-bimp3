# BIMP — Batch Image Manipulation Plugin for GIMP 3.x

Apply a set of GIMP operations to a batch of images in a single session.
This is a port of [BIMP 2.6](https://github.com/alessandrofrancesconi/gimp-plugin-bimp) by Alessandro Francesconi, updated to run on **GIMP 3.x and GTK 4**.

**Repository:** https://github.com/WCoaster77/gimp-plugin-bimp3
**Base version:** BIMP 2.6 (GIMP 2.x / GTK 2-3)
**Target platform:** GIMP 3.x / GTK 4 / GEGL

> **Status:** Port complete (Phase 1 & 2). Not yet tested end-to-end in a live GIMP 3.x environment — treat as a work in progress.

---

## Supported manipulations

- Resize
- Crop
- Flip / Rotate
- Color correction (curves, levels, brightness, hue, saturation)
- Sharpen / Blur
- Watermark (text or image)
- Change format and compression (BMP, GIF, ICO, JPEG, PNG, TGA, TIFF, HEIF, WebP, AVIF, EXR)
- Rename
- Other GIMP procedure (run any PDB procedure on each image)

---

## Building on Linux

Install GIMP 3.x development libraries:

```bash
# Debian / Ubuntu
sudo apt-get install libgimp-3.0-dev libgegl-dev

# Fedora
sudo dnf install gimp-devel-tools
```

Clone and build:

```bash
git clone https://github.com/WCoaster77/gimp-plugin-bimp3.git
cd gimp-plugin-bimp3
make && make install
```

For a system-wide install (requires root):

```bash
sudo make install-admin
```

The Makefile uses `gimptool-3.0` to resolve include paths and the install location automatically.

---

## Building on Windows

Use [MSYS2](https://www.msys2.org/) with the GIMP 3.x MinGW toolchain. Install the GIMP 3.x dev package for your architecture, then run `make && make install` inside an MSYS2 shell.

---

## Building on macOS

1. Install [MacPorts](https://www.macports.org/install.php)
2. Install prerequisites: `sudo port install coreutils`
3. Add GNU tools to your PATH: `export PATH=/opt/local/libexec/gnubin:$PATH`
4. Install GIMP 3 with MacPorts: `sudo port install gimp3`
5. Follow the Linux build instructions above.

---

## What changed from BIMP 2.6

See [CHANGELOG_Rebuild.md](CHANGELOG_Rebuild.md) for the full migration log. Key changes:

| Area | Change |
|---|---|
| Plugin entry point | `GimpPlugInInfo` + `MAIN()` → `GimpPlugIn` GObject subclass |
| Image/drawable IDs | `gint32` → `GimpImage*` / `GimpDrawable*` / `GimpLayer*` |
| Procedure arguments | `GimpParam[]` → `GimpValueArray` / `GParamSpec*` |
| Procedure calls | `gimp_run_procedure()` → `gimp_pdb_run_procedure()` |
| Color types | `GdkColor` / `GimpRGB` → `GdkRGBA` / `GeglColor*` |
| Widgets | `GtkMenu` → `GtkPopover`; `gtk_vbox/hbox_new` → `gtk_box_new` |
| Build tool | `gimptool-2.0` → `gimptool-3.0` |

---

## Project structure

```
src/
  bimp.c                        Plugin entry point (GimpPlugIn subclass)
  bimp-operate.c/.h             Batch engine — driver and state
  bimp-operate-process.c        Per-image processing loop
  bimp-operate-apply.c          Resize, crop, flip/rotate, color
  bimp-operate-fx.c             Sharpen/blur, watermark, user-defined
  bimp-operate-save.c           Save dispatcher + BMP/GIF/ICO/JPEG
  bimp-operate-formats.c        PNG/TGA/TIFF/HEIF/WebP/AVIF/EXR
  bimp-gui.c/.h                 Main dialog, progress, busy state
  bimp-gui-filelist.c           Input file treeview
  bimp-gui-chooser.c            File/folder chooser dialogs
  bimp-gui-sequence.c           Manipulation sequence panel
  bimp-gui-panel.c              Options panel, preview, output folder
  bimp-manipulations.c/.h       Manipulation data structures
  bimp-manipulations-gui.c/.h   Edit dialogs per manipulation type
  bimp-serialize.c/.h           Save/load .bimp set files
  bimp-utils.c/.h               PDB helpers, string utilities
  manipulation-gui/
    gui-changeformat.c/.h       Format-change GUI coordinator
    gui-changeformat-build.c    Per-format widget builders
    gui-userdef.c/.h            User-defined procedure GUI
    gui-userdef-params.c        Procedure parameter widget builders
    gui-userdef-save.c          Parameter value save logic
    gui-colorcurve.c/.h         Color curve editor
    gui-resize.c/.h             Resize options
    gui-crop.c/.h               Crop options
    gui-fliprotate.c/.h         Flip/rotate options
    gui-color.c/.h              Color correction options
    gui-sharpblur.c/.h          Sharpen/blur options
    gui-watermark.c/.h          Watermark options
    gui-rename.c/.h             Rename pattern options
```

---

## Credits

Original BIMP by Alessandro Francesconi — https://github.com/alessandrofrancesconi/gimp-plugin-bimp

GIMP 3.x port by Frank (WCoaster77) — https://github.com/WCoaster77/gimp-plugin-bimp3

Licensed under the GNU General Public License v2 or later. See [LICENSE](LICENSE).
