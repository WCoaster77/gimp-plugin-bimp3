# BIMP 3.x — Making It Installable

**Goal:** Take the ported codebase from "code exists" to "a user can download and run it."

**Branch:** `feature/gimp3-port`
**Repo:** https://github.com/WCoaster77/gimp-plugin-bimp3

---

## Current State

| Item | Status |
|---|---|
| Source ported to GIMP 3.x / GTK4 | ✅ Complete |
| All files ≤ 300 lines, functions ≤ 50 lines | ✅ Complete |
| Compiled and confirmed to build | ❌ Not yet |
| Tested in live GIMP 3.x session | ❌ Not yet |
| Binary release available | ❌ Not yet |
| Windows installer | ❌ Not yet |

---

## Phase 3 — Compile and Fix

**Goal:** `make` completes without errors on a machine with GIMP 3.x dev headers.

### Environment setup

| Platform | Required packages |
|---|---|
| Ubuntu 24.04+ / Debian | `libgimp-3.0-dev libgegl-dev libgtk-4-dev gcc make` |
| Fedora 40+ | `gimp-devel-tools gegl-devel gtk4-devel gcc make` |
| Arch Linux | `gimp gegl gtk4 base-devel` |
| Windows (MSYS2) | `mingw-w64-x86_64-gimp mingw-w64-x86_64-gegl mingw-w64-x86_64-gtk4` |

If GIMP 3.x dev packages are not in the distro's stable repos yet, build GIMP from source
using the [GIMP build instructions](https://developer.gimp.org/core/setup/build/linux/).

### Build steps

```bash
git clone https://github.com/WCoaster77/gimp-plugin-bimp3.git
cd gimp-plugin-bimp3
make 2>&1 | tee build.log
```

### Expected failure categories

Work through errors in this order:

1. **Missing includes** — headers moved or renamed between GIMP 3.x minor versions.
   Check `gimptool-3.0 --cflags` for the correct include path.

2. **Deprecated API calls** — GIMP 3.x API is still evolving; some calls ported from
   2.x may have been renamed again before final GIMP 3.0 release. Cross-reference the
   [GIMP 3.x API docs](https://developer.gimp.org/api/3.0/).

3. **GTK4 deprecations** — GTK4 itself deprecates APIs across minor releases.
   Run with `GTK_DEBUG=deprecation` to surface runtime warnings.

4. **Type mismatches** — `GimpValueArray` argument counts or `GParamSpec` type comparisons
   that weren't exercised by the port review.

5. **Linker errors** — missing `-l` flags. Check `gimptool-3.0 --libs` output.

### Definition of done

- [ ] `make` exits 0 with no errors
- [ ] `make install` places the binary in GIMP's plugin directory
- [ ] Commit fix(es) per logical error group: `fix: <description>`

---

## Phase 4 — Test in GIMP 3.x

**Goal:** Every manipulation type runs to completion on a batch of test images.

### Test environment

- GIMP 3.x (same version dev headers were built against)
- A folder of mixed test images: JPEG, PNG, TIFF, WebP, at least one with transparency
- An output folder separate from input

### Test checklist

#### Plugin loads
- [ ] BIMP appears in the GIMP **Filters** menu after install
- [ ] Clicking the menu item opens the BIMP main window
- [ ] File list populates when images are added

#### Manipulations — basic smoke test

Run a 3-image batch through each manipulation type individually:

- [ ] **Resize** — fixed pixel width, preserve aspect ratio
- [ ] **Crop** — centre crop to fixed dimensions
- [ ] **Flip / Rotate** — horizontal flip; 90° rotation
- [ ] **Color correction** — brightness +10, no curves file
- [ ] **Sharpen / Blur** — sharpen amount 25
- [ ] **Watermark (text)** — "TEST" bottom-right, 50% opacity
- [ ] **Watermark (image)** — overlay a PNG with transparency
- [ ] **Change format** — JPEG → PNG; PNG → WebP
- [ ] **Rename** — append `_batch` suffix
- [ ] **Other GIMP procedure** — `plug-in-unsharp-mask` or similar

#### Format output — save round-trip

Verify each output format actually writes a readable file:

- [ ] BMP
- [ ] GIF
- [ ] ICO
- [ ] JPEG (with quality param)
- [ ] PNG (with compression param)
- [ ] TGA
- [ ] TIFF
- [ ] HEIF
- [ ] WebP (lossy and lossless)
- [ ] AVIF
- [ ] EXR

#### Edge cases

- [ ] Batch of 1 image
- [ ] Batch of 50+ images
- [ ] Input and output folder are the same (overwrite prompt)
- [ ] Output folder does not exist yet (should be created or warned)
- [ ] Keep folder hierarchy option with nested input subfolders
- [ ] Save a `.bimp` set file and reload it — settings preserved
- [ ] Delete source files on completion option

#### GUI

- [ ] Preview window shows correct result before applying
- [ ] Progress bar advances during batch
- [ ] Stop button halts the batch mid-run
- [ ] Error dialog appears (not a crash) when a bad procedure is selected in User-Defined

### Regression handling

For each failure found:
1. Identify the source file and function
2. Fix, keeping the function ≤ 50 lines and file ≤ 300 lines
3. Re-run the failing test case
4. Commit: `fix: <what broke and why>`

### Definition of done

- [ ] All checklist items above pass
- [ ] No GIMP crashes or unhandled signals during any test
- [ ] All fixes committed

---

## Phase 5 — Package and Release

**Goal:** A user with no development tools can install BIMP 3.x.

### Linux binary release

1. Build on the oldest supported LTS Ubuntu (currently 24.04) for maximum compatibility.
2. Strip the binary: `strip bin/bimp`
3. Create a GitHub Release tagged `v3.0.0-beta.1` with:
   - `bimp-linux-x86_64` binary
   - Install instructions in the release notes (copy binary to `~/.config/GIMP/3.0/plug-ins/bimp/bimp`)
4. Test the binary on a clean machine without dev headers installed.

### Windows installer

1. Cross-compile using MSYS2 MinGW toolchain (see Phase 3 environment table).
2. Update `nsis/bimp.nsi`:
   - Change install path from `GIMP\2.x\` to `GIMP\3.0\`
   - Update `gimptool` version references
   - Update any GTK runtime version references
3. Build the installer: `makensis nsis/bimp.nsi`
4. Add `bimp-win64-setup.exe` to the GitHub Release.
5. Test on a clean Windows machine with GIMP 3.x installed (no dev tools).

### macOS

1. Document the Homebrew / MacPorts build path once GIMP 3.x is available there.
2. Consider a `.dmg` with the plug-in binary for a future release — defer until
   GIMP 3.x reaches a stable macOS distribution.

### GitHub Release checklist

- [ ] Tag created: `v3.0.0-beta.1`
- [ ] Linux x86_64 binary attached
- [ ] Windows installer attached
- [ ] Release notes list: supported GIMP version, what's new vs BIMP 2.6, known issues
- [ ] README updated with download link to latest release

### Definition of done

- [ ] A user with GIMP 3.x installed can add BIMP with zero compilation steps
- [ ] The release is tagged and published on GitHub

---

## Phase 6 — Continuous Integration

**Goal:** Pull requests automatically build and test to prevent regressions.

### GitHub Actions workflow

Create `.github/workflows/build.yml`:

```yaml
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: sudo apt-get install -y libgimp-3.0-dev libgegl-dev libgtk-4-dev
      - name: Build
        run: make
      - name: Check binary
        run: file bin/bimp | grep -q ELF
```

Extend with a test-image batch run once Phase 4 is complete and a headless test
harness exists (e.g., `xvfb-run` for GTK display).

### Definition of done

- [ ] CI badge on README shows passing
- [ ] Every push to `feature/gimp3-port` triggers a build

---

## Commit convention

Follow the convention in `AGENTS.md`:

| Prefix | When to use |
|---|---|
| `fix:` | Compile error or runtime bug |
| `test:` | Add or update test images / test scripts |
| `chore:` | CI config, release scripts, Makefile tweaks |
| `docs:` | README, release notes, this plan |

One logical fix per commit. Do not batch unrelated fixes.

---

## Tracking progress

Update the **Current State** table at the top of this file as each phase completes.
