# Installation and recovery

1. Confirm that the device is an M-VAVE FM-1. Read these instructions before updating.
2. Back up your existing work, samples and show banks with the appropriate tools. Do not assume that changing firmware preserves data across all versions.
3. Use the `.fwsc` from the corresponding release and verify its SHA-256 against the release checksum file. The 0.3.2-public package identity is `FM-1_900` and it includes the Felucca update loader.
4. Connect the FM-1 directly to a computer with a reliable USB data cable. Stop applications that may compete for its MIDI connection.
5. Use the [LAUNCHPARTY browser updater](https://mandalars.org/mandalars/launchparty/) in a Web-MIDI-capable desktop browser such as Chrome or Edge. Follow its prompts and select the FM-1/Launchparty device, not the Launchpad. Do not interrupt power during transfer or reboot.
6. Alternatively install the Python MIDI dependencies and use the bundled installer:

   ```sh
   python3 -m pip install mido python-rtmidi
   python3 tools/fm1_install.py --info
   python3 tools/fm1_install.py /path/to/launchparty-0.3.2-public.fwsc
   ```

7. After reboot check the reported version, synth sound, controls and show output. The Launchpad is connected separately through a USB-MIDI host or computer bridge; its firmware is not replaced.

Firmware replacement can fail. If ordinary USB-MIDI updating is no longer available, consult upstream [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) recovery documentation before attempting recovery. Keep a known-good compatible firmware and its recovery instructions available.

No automatic hardware installation is performed by building this repository or running its host tests.
