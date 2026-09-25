# VitaHomebrewUpdate v1.0.0

> [!WARNING]
> **VitaDB Downloader's `vdb_daemon.suprx` is confirmed incompatible.**
> Enabling it together with VitaHomebrewUpdate causes SceShell hard locks,
> LiveArea background corruption, and FTP/network instability. Comment out or
> remove `ux0:data/vitadb/vdb_daemon.suprx` from `*main` and reboot **before**
> enabling VitaHomebrewUpdate. VitaDB compatibility is not claimed.

This first production release brings the official-game LiveArea update
workflow to explicitly configured homebrew on hardware-proven retail 3.65.
The plugin discovers each participating title through
`ux0:app/<TITLE_ID>/sce_sys/homebrew_update.ini`, presents the update in
LiveArea, downloads through BGDL, verifies and stages the VPK, and changes the
notification to **Waiting to Install**. Activating that notification or
starting the target app installs the staged update before normal launch; the
app then starts on the installed version.

## Reference project acknowledgment

VitaHomebrewUpdate is an independent behavioral reimplementation inspired by
the reference HomebrewUpdate project. Its source code was not available and
was not copied or used. The behavior was recreated independently from visual
and on-device observation and publicly visible interfaces/artifacts, then
extended with dynamic per-title discovery, the client readiness API, strict
verification and notification lifecycle, and title-scoped cleanup. We
respectfully credit the reference project for demonstrating the workflow.
There is no affiliation or endorsement, and this release does not claim exact
internal equivalence.

## Validation basis

- Restored candidate 124 is the hardware-proven implementation basis:
  `e2e650af272554b0649da209d51e09e7766aef21491f346bfbd3542d3f598202`,
  168,053 bytes.
- Candidate 125 and its failed low-memory experiment are not included.
- The production source removes LAUTEST fallback configuration and test-only
  build targets. Therefore the clean production artifact is expected to have
  a different binary identity; build validation compares behavior,
  configuration, firmware gates, and dependency inputs rather than claiming
  byte identity.
- PKGj was hardware-tested concurrently and worked with distinct BGDL task
  IDs. The plugin retains a narrow fallback for firmware paths that do not
  return a task ID immediately; this is not evidence for arbitrary concurrent
  BGDL clients.

## Scope and limitations

- Proven firmware scope: retail 3.65 only.
- At most eight configured applications are discovered during one scan.
- Update feeds must remain reachable while the plugin checks them.
- Packages are accepted only after size, SHA-1, archive CRC/path, Title ID,
  version, and Content ID validation.
- HTTPS uses the bundled production trust anchors; invalid certificates fail
  closed.
- VitaDB Downloader's daemon is incompatible as described above.
