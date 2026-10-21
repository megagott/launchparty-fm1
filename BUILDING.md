# Build LAUNCHPARTY 0.3.2-public

## Requirements

- Linux x86-64, or WSL with a Linux x86-64 environment.
- Python 3 and the JieLi pi32v2 Linux toolchain (`jieli-linux-toolchains-20250324.1` in the development environment).
- The JieLi AC79 SDK, tested with `AC79NN_SDK_V1.2.1_2023-12-13`.
- A C compiler for host tests; Node.js for browser companion tests.

SDK/toolchain downloads are external dependencies; they are not bundled. See `tools/get_toolchain.sh`, the upstream Felucca project and <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK>.

## Build

```sh
python3 tools/build_launchparty.py \
  --toolchain /path/to/jieli/toolchain \
  --sdk /path/to/fw-AC79_AIoT_SDK
```

The wrapper preserves the checked-in generated headers in `build/gen/`; these are part of the corresponding source, including the current fonts, samples and icons. Do not run the generic `build.py` regeneration path for this snapshot: it expects an upstream asset layout and may generate different assets. Original generator sources and asset notices are retained under `tools/` and `credits/`.

The wrapper builds the app and loader, checks memory/layout limits and creates `build/felucca.fwsc`. It does not flash a device. `FM-1_900` is the package identity. The display version comes from the LAUNCHPARTY source. Do not use the generic `--release` switch, which follows upstream's different version-number scheme.

The package contains external Apache-2.0 SDK components. Their expected hashes are checked by `tools/build.py`; any mismatch must be investigated before distribution.

## Host tests

```sh
python3 tools/run_release_tests.py
node web/test-core.mjs
node web/test-keyboard.mjs
node web/test-transport.mjs
```

The tests do not connect to or install hardware. A full physical hardware and audio check remains separate.

## Browser companion

Serve `web/` from localhost for preview and Web MIDI. Copy the release `.fwsc` into `web/` if testing its updater, since firmware binaries are release assets rather than committed source files. The PHP gallery requires a matching Joomla frontend session integration and protected writable `gallery-data/`; uploaded user data is not included here.
