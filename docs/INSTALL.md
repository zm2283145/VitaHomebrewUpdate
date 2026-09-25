# Installation and recovery

> [!WARNING]
> **VitaDB Downloader's `vdb_daemon.suprx` is confirmed incompatible.**
> Running both plugins causes SceShell hard locks, LiveArea background
> corruption, and FTP/network instability. Before enabling
> VitaHomebrewUpdate, comment out or remove
> `ux0:data/vitadb/vdb_daemon.suprx` from the `*main` section and reboot.

VitaHomebrewUpdate v1.0.1 is hardware-proven only on retail firmware 3.65.
Its SceShell hooks are firmware-signature-gated and it is not a general
firmware compatibility claim.

## Install

1. Ensure VitaDB Downloader's daemon is disabled as described above, then
   reboot before continuing.
2. Copy `vita-homebrew-update.suprx` to `ur0:tai/`.
3. Add the plugin under the single `*main` section in `ur0:tai/config.txt`.
   Keep it after foundational shell/kernel plugins and do not place it under a
   title-specific section:

   ```text
   *main
   ur0:tai/vita-homebrew-update.suprx
   ```

4. Reboot the Vita. A reload without reboot is not sufficient for this shell
   plugin.

`host/install_plugin.py` can perform a backup-aware FTP installation. It
backs up the existing config and plugin, stages uploads, and verifies final
readback:

```text
python host/install_plugin.py build/vita-homebrew-update.suprx --vita VITA_IP
```

This command changes the device; it is not part of a normal source build.

## Uninstall

1. Comment out or remove `ur0:tai/vita-homebrew-update.suprx` from `*main`.
2. Reboot.
3. After the reboot, remove the plugin file if desired.

The plugin stores working data under `ux0:data/VitaHomebrewUpdate/`. Remove
that directory only after uninstalling and rebooting; doing so discards staged
updates and diagnostic logs.

## Recovery

If SceShell is unstable, power off completely, boot while holding **L** to
skip taiHEN plugins, remove/comment the VitaHomebrewUpdate config line, and
reboot normally. Restore the timestamped `config.txt.pre-vhbu-*` backup if the
FTP installer was used. Do not re-enable the plugin until
`vdb_daemon.suprx` is absent from `*main`.
