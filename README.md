# LAUNCHPARTY for the M-VAVE FM-1

![LAUNCHPARTY](launchparty-logo-v1.png)

**Play the lightshow.** LAUNCHPARTY is an experimental alternative FM-1 firmware combining playable synthesizer sounds with interactive Novation Launchpad Pro MK3 lightshows. It is the performance-and-visuals sibling of [MYCELIUM](https://github.com/megagott/mycelium-fm1), rather than a groovebox sequencer.

## What it does

- Main lightshow clips plus keyboard-triggered overlays with independent visual layers.
- 27 keyboard overlay assignments, selectable neon colours and a multicolour option.
- Pad effects including Solo, rings, halo, explosion, vortex, sparks and perimeter animation.
- Auto Party and live controls for brightness, speed and trails.
- Playable synth sounds alongside the visuals.
- Browser companion for previews, computer-keyboard performance, show import/export, device communication and firmware updates.
- Optional Joomla-backed show gallery; local playback does not need the gallery backend.

[Open the companion](https://mandalars.org/mandalars/launchparty/) Â· [Visual guide](https://mandalars.org/mandalars/launchparty/guide.html) Â· [Both FM-1 firmware projects](https://mandalars.org/mandalars/fm1/)

The browser companion is available on the project website. Its live factory content may differ from this public source edition.

## Hardware and connection

Target: **M-VAVE FM-1 and Novation Launchpad Pro MK3**. Other Launchpad models are not claimed compatible. The Launchpad keeps its standard firmware and uses Programmer Mode.

Both devices are USB peripherals. A computer-free setup therefore requires a suitable **external USB-MIDI host with routing and power**; a passive USB hub alone is insufficient. A computer MIDI bridge can be used while testing. Route the FM-1's Launchparty MIDI output to the appropriate Launchpad MIDI port and route Launchpad input back to the FM-1 as required. Host-specific routing needs to be configured and checked.

## Current release status

The prepared test package is **0.3.2-public**, product identity **FM-1_900**.

SHA-256: `83b4a2ee4aa539c8946e8b9660a51b0bc6eff5abd1677fab0f3b55712429873e`

The preceding 0.3.1 development version has documented transfer/reboot, version reporting, persistent show upload/readback and show-bank preservation. This public 0.3.2 build replaces private imported shows with original procedural loops and has not been installed on hardware. Host-side regression tests cover playback, mixing, effects and storage. These checks do not replace a full physical audio, visual and USB-host compatibility test. Treat the release as experimental, not a production-certified instrument.

Download and extract [the complete corresponding source](launchparty-0.3.2-public-source.zip) to build or modify it. See [installation and recovery](INSTALL.md), [build instructions](BUILDING.md), [release notes](RELEASE.md) and [test report](VERIFICATION.md).

## Firmware Hub

Sven's [FM-1 Firmware Hub](https://fm1.designburgapps.com/) is a community resource. Hub inclusion is not claimed. A real FM-1 display photograph is still needed for a complete submission; `docs/boot-render.png` is a software render, not such a photograph.

Suggested card description:

> LAUNCHPARTY is an experimental, open-source FM-1 firmware for interactive Launchpad Pro MK3 lightshows and playable synth sounds. It combines layered clips, keyboard-triggered overlays, neon pad effects and a browser companion.

## Source and acknowledgements

Derived from MYCELIUM, SLOOP and Felucca, with upstream work by Leo Kuroshita / HÃ¼gelton Instruments and the SLOOP contributors. See [credits](CREDITS.txt), [LICENSING.md](LICENSING.md) and the full [GPL-3.0 licence](LICENSE).

This is independent software, not affiliated with or endorsed by Novation, Focusrite, M-VAVE or Ableton. Third-party names are used to identify hardware and compatibility.
