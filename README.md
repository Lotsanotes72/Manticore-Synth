# Manticore

A modular grid synthesizer by **Fellowship of the String**, VST3 + standalone for Windows.

Four lanes of four slots, filled from a pool of 28 modules (wavetable/additive OSC, FM, noise, exciter, resonator, comb,
13-model VCF, shaper, ring mod, EQ), a modulation rack (4 ENV, 4 LFO, 4 MSEG, 4 mappers, 4 mod math, 24-slot matrix,
4 macros), an FX grid (Main + 3 aux buses), a 16-step arpeggiator, an XY vector mixer (4 pads) and 500 factory presets with favourites.

## What's new in 1.1

- **Mutate** (whole patch, or one module from its editor), **Breed** (A x B, or with a random favourite) and **Haunt**
  (slow drift with ghost pointers, Follow and Capture). Offline level matching keeps every result at the original loudness.
- **Padlocks** on the grid layout, each lane, the modulators, the matrix, the effects, the arp and the XY pads.
- **Undo / Redo** now covers 48 steps of everything (snapshot history).
- **XY Vector Mixer** page: 4 pads x 4 targets per axis; pads are automatable and also global mod sources.
- 40 factory presets renamed so their instrument word matches the sound (stars and saved sessions follow automatically).

## Build (GitHub, same as the other Fellowship plug-ins)

1. Upload this folder to a GitHub repository (branch `main`).
2. The workflow in `.github/workflows/build.yml` runs on every push (or from **Actions > Build Manticore (Windows) > Run workflow**).
3. When it finishes, open the run and download **Manticore-Setup** (the installer) or **Manticore-VST3** (the bare plug-in).

JUCE 8.0.10 is downloaded automatically by CMake.

## Folders

| Path | What |
|---|---|
| `Source/` | Plug-in code: `Params.*` (every parameter, once), `Evolve.*` (Mutate / Breed / Haunt rules, padlocks), `dsp/` (engine), `ui/` (interface), `Presets.*` |
| `Resources/` | Fonts (SIL OFL), app icon, `factory_presets.json.gz` (the 500 presets built into the plug-in) |
| `installer/` | Inno Setup script and the desktop / setup icons |
| `docs/` | `Manticore_User_Manual.pdf`, screenshots, preset list |
| `tools/` | Generators for presets (`make_presets.py`, `normalize_presets.py`), icons and the manual |
| `tests/` | Offline test / render / screenshot tool (Linux, not shipped) |

## Regenerating things (optional)

```
python3 tools/make_presets.py      # 500 presets (applies the stored levels in tools/levels.json)
python3 tools/make_icons.py        # icons
python3 tools/make_manual.py       # manual (needs the screenshots in docs/img)
```
Tests: `ManticoreTests unit` (everything) or `ManticoreTests evolve` (Mutate / Breed / Haunt / XY, plus a 60-preset sweep).

To re-level presets after changing sounds: build with `-DMC_BUILD_TESTS=ON`, run
`ManticoreTests render out.json`, then `python3 tools/normalize_presets.py out.json`, rebuild and repeat until it settles.
