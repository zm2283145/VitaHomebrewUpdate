# VitaHomebrewUpdate v1.0.1

> [!WARNING]
> **VitaDB Downloader's `vdb_daemon.suprx` is confirmed incompatible.**
> Enabling it together with VitaHomebrewUpdate causes SceShell hard locks,
> LiveArea background corruption, and FTP/network instability. Comment out or
> remove `ux0:data/vitadb/vdb_daemon.suprx` from `*main` and reboot **before**
> enabling VitaHomebrewUpdate. VitaDB compatibility is not claimed.

This maintenance release fixes the LiveArea download confirmation dialog.
Instead of unrelated LAUTEST development bullets, the dialog now displays the
active title's downloaded `<changeinfo><changes><![CDATA[...]]></changes>`
content beneath the installed version, available version, and package size.

Change information is parsed with explicit input and output bounds. Invalid
UTF-8 and unsupported control bytes are replaced safely for the PAF dialog,
oversized text is truncated at a complete UTF-8 sequence with an ellipsis,
and missing, empty, or malformed XML displays an explicit
`Release notes are unavailable for this update.` fallback.

All v1.0.0 validation scope and limitations remain in effect: retail firmware
3.65 is the only hardware-proven target, configured packages still undergo
strict identity and archive verification, and VitaDB Downloader's daemon
remains incompatible.
