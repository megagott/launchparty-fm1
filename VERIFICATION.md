# Verification — 0.3.2-public

## Fresh public build

Built from this staged public source tree with the existing JieLi pi32v2 toolchain and AC79 SDK. The private first twelve scenes were replaced with original procedural loops before building.

- Package: `launchparty-0.3.2-public.fwsc`, 609999 bytes.
- SHA-256: `83b4a2ee4aa539c8946e8b9660a51b0bc6eff5abd1677fab0f3b55712429873e`.
- Product: `FM-1_900`.
- App image: 555596 bytes.
- RAM data plus BSS: 90780 of 98304 bytes.
- Pool: 319160 of 344064 bytes.
- Loader: 6843 bytes.
- Build register-access and layout checks passed.

## Host checks passed

1. Player lifecycle, beat switching, malformed input and backpressure.
2. All 106 Launchpad physical positions and their unique mapping.
3. Palette, brightness, RGB bounds and timer wrap.
4. Layer mixing, priority, transparent black, release fade and independent timing.
5. Auto Party and manual takeover across long simulated sets.
6. All 27 keyboard mappings and neon recolouring.
7. Solo and 32-position perimeter effects.
8. Persistent storage including interrupted writes, invalid data and sample-region protection.
9. All 35 touch effects retain the intended background floor.
10. Forty boot-animation frames remain within display bounds.
11. All 41 compiled factory slots match their source at every simulated millisecond.

Browser checks passed for all 41 public clips, MIDI conversion, truncated/oversized input, CRC, 7-bit packing, held-key lifecycle, colour wrap and transport state.

## Limits

The public 0.3.2 package has not been flashed during this publication task. Previous 0.3.1 device tests do not establish a physical verification of this changed build. Audio listening, performance load, visual appearance and external USB-MIDI host routing still require a physical check. The included boot image is a software render; it must not be presented as a real device photograph.
