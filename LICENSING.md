# Licensing and attribution

LAUNCHPARTY retains the GPL-3.0-only firmware licence and upstream copyright notices from MYCELIUM, SLOOP and Felucca. The complete text is in `LICENSE`. Original LAUNCHPARTY code and the original procedural factory patterns included in this public edition are distributed under GPL-3.0-only.

## Corresponding source

`firmware/` contains the app, HAL and update loader. `tools/` contains build, package and generation tools. `build/gen/` contains the actual generated font, icon, sample and table headers consumed by this snapshot; these are deliberately retained. `shows/` contains the raw original procedural factory loops and their manifest. Private imported Ableton shows and user gallery submissions are excluded.

## Third-party components

- Felucca / SLOOP / MYCELIUM code: GPL-3.0-only; upstream notices retained in source.
- PHASE oscillator code derives from CrispyZebra by Leo Kuroshita (GPL-3.0).
- Legacy sample data: CC0, with per-sample provenance in `credits/ATTRIBUTION.txt`.
- Original MYCELIUM procedural drums and geometric firmware icons: GPL-3.0-only. See `credits/MYCELIUM-ICONS-LICENSE.txt`.
- Terminus font: SIL Open Font License 1.1, reproduced in `credits/Terminus-LICENSE.txt`.
- JieLi AC79 SDK package components: Apache-2.0, fetched externally by the builder. The repository does not contain the SDK or toolchain. SDK project: <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK>.
- `credits/CREDITS.txt` retains additional upstream acknowledgements and design references.

The source generators retain upstream history and may reference assets not used by this snapshot. Build using `tools/build_launchparty.py` and the checked-in generated assets, as described in `BUILDING.md`.

LAUNCHPARTY visual branding was selected by MandaLars; the procedural firmware logo representation is distributed with the project. Product names and logos do not imply an endorsement by the named hardware manufacturers.

Novation, Launchpad, Focusrite, M-VAVE, FM-1 and Ableton are names or marks of their respective owners. LAUNCHPARTY is an independent project.
