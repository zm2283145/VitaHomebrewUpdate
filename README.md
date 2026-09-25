# VitaHomebrewUpdate

> [!WARNING]
> **VitaDB Downloader's `vdb_daemon.suprx` is confirmed incompatible with
> VitaHomebrewUpdate. Enabling both causes SceShell hard locks, LiveArea
> background corruption, and FTP/network instability. Comment out or remove
> `ux0:data/vitadb/vdb_daemon.suprx` from `*main` and reboot before enabling
> VitaHomebrewUpdate. This project does not claim VitaDB compatibility.**

VitaHomebrewUpdate gives configured PS Vita homebrew an official-game-style
LiveArea update workflow. It discovers per-title configuration, checks a
stable release manifest, presents the native update experience, downloads
through BGDL, verifies the package, stages it, and installs it before the
updated app launches.

## Reference project acknowledgment

VitaHomebrewUpdate is an independent behavioral reimplementation inspired by
the reference HomebrewUpdate project, which demonstrated this workflow on PS
Vita. The reference project's source code was not available and was not
copied or used. This implementation was recreated independently from visual
and on-device observation of the reference behavior and from publicly visible
interfaces and artifacts. It was then independently extended with dynamic
per-title configuration, a title-scoped client readiness API, strict package
verification, a persistent notification/install lifecycle, and BGDL cleanup.

This project is not affiliated with or endorsed by the reference project and
does not claim exact internal equivalence. The reference project is credited
with demonstrating that this user experience was possible.

**Supported firmware:** retail 3.65 is the only hardware-proven scope for
v1.0.0. Private SceShell offsets and signatures are firmware-specific; do not
interpret the runtime guards as evidence of compatibility with other firmware.

## Installation

1. Disable VitaDB Downloader's daemon and reboot as required by the warning.
2. Copy `vita-homebrew-update.suprx` to `ur0:tai/`.
3. Add `ur0:tai/vita-homebrew-update.suprx` beneath `*main` in
   `ur0:tai/config.txt`. Keep it after foundational shell/kernel plugins and
   before the next section header.
4. Reboot.

See [docs/INSTALL.md](docs/INSTALL.md) for backup-aware FTP installation,
uninstall, and boot-recovery instructions.

## App configuration

Each participating app must ship:

```text
ux0:app/<TITLE_ID>/sce_sys/homebrew_update.ini
```

The schema is:

```ini
title_id=VITAHBU01
name=Example Homebrew
update_url=https://raw.githubusercontent.com/OWNER/REPOSITORY/main/update/VITAHBU01-ver.xml
```

- `title_id` is required and must be exactly nine uppercase ASCII letters or
  digits. It must match the app directory and installed `param.sfo`.
- `update_url` is required and must be an absolute HTTP or HTTPS URL.
- `name` is optional and defaults to the Title ID.

Use a stable manifest URL that does not change for every app release. A raw
file on the default branch is suitable; that file may point package and
changeinfo URLs at immutable tagged GitHub release assets. The sample is at
[`examples/homebrew_update.ini`](examples/homebrew_update.ini).

## Release manifest and change information

The manifest is Sony-style update XML with one `<package>` element. The
following package attributes are required and validated:

- `version`
- decimal `size`
- 40-character hexadecimal `sha1sum`
- absolute package `url`
- `content_id`

It must also contain `<changeinfo url="..."/>`. The changeinfo endpoint must
return XML containing release text; a failed or empty changeinfo fetch does
not publish an update. The package is accepted only when its exact size and
SHA-1 match and its VPK structure, entry CRCs/paths, Title ID, app version,
and Content ID all validate.

Generate matching package, manifest, changeinfo, and JSON metadata:

```text
python host/prepare_update.py app.vpk \
  --root release-feed \
  --title-id VITAHBU01 \
  --version 01.23 \
  --name "Example Homebrew" \
  --content-id EP9000-VITAHBU01_00-EXAMPLEHOMEBREW1 \
  --changes "Changes in 01.23" \
  --base-url https://github.com/OWNER/REPOSITORY/releases/download/v1.23.0
```

Do not alter the VPK after generating the manifest hashes.

## Client API integration

Link `libHomebrewUpdateClient.a`, include
`homebrew_update_client.h`, initialize SceNet, and identify the calling title:

```c
int status = HomebrewUpdateClientGetStatusForTitle("VITAHBU01");

if (status == HOMEBREW_UPDATE_READY) {
    /* Disable the app's built-in updater for this run. */
} else {
    /* Keep the built-in updater: -1, 0, 1, and unknown failures fall back. */
}
```

Disable the app's built-in updater **only** when status is exactly
`HOMEBREW_UPDATE_READY` (`2`). Every other status must fall back:

| Status | Symbol | Required app behavior |
| ---: | --- | --- |
| `-1` | `HOMEBREW_UPDATE_NOT_DETECTED` | Use the built-in updater |
| `0` | `HOMEBREW_UPDATE_DISABLED` | Use the built-in updater |
| `1` | `HOMEBREW_UPDATE_HOOK_ERROR` | Use the built-in updater |
| `2` | `HOMEBREW_UPDATE_READY` | Defer to VitaHomebrewUpdate |

The title-scoped call arms behavior only for a discovered title. The
compatibility `HomebrewUpdateClientGetStatus()` call reports service health
without identifying a title. The client timeout is 250 ms.

Apps integrating the explicit pending-update handoff may call
`HomebrewUpdateClientLaunchPendingInstaller(title_id)` before their normal
startup path. It returns `1` after handoff, `0` when no matching staged update
exists, and a negative value for invalid state or launch failure.

## Update lifecycle

1. The plugin discovers up to eight configured apps and fetches each manifest.
2. When a newer version exists, LiveArea offers the update and displays the
   fetched change information.
3. The VPK downloads as a native BGDL task with normal progress, pause,
   resume, and cancellation behavior.
4. Completion is not installation: the plugin exports the task payload,
   verifies all required hashes and package identity, stages it, and cleans
   the title's BGDL task tree.
5. Only after successful verification does the notification become
   **Waiting to Install**.
6. Activating that notification **or starting the target app** installs the
   verified staged update before normal launch. After installation, the app
   starts on the newly installed version.

Staged state is title-scoped and persisted across reboot. Invalid or incomplete
packages are deleted instead of promoted.

## PKGj compatibility evidence

PKGj was hardware-tested concurrently with VitaHomebrewUpdate on retail 3.65.
Both operated with distinct BGDL task IDs, and VitaHomebrewUpdate cleaned only
its own title's task. A narrow fallback remains for the firmware path where
BGDL registration does not immediately return a task ID; this should not be
generalized to untested download managers or firmware.

PKGj evidence does **not** apply to VitaDB Downloader. Its
`vdb_daemon.suprx` is confirmed incompatible as described in the warning.

## Build

Requirements:

- VitaSDK with `VITASDK` set
- CMake 3.16 or newer
- a Make or Ninja generator available to CMake
- Python 3.10 or newer for host/release tooling

Configure a clean production build with the validated dialog path explicitly
enabled:

```text
cmake -S . -B build-release -DVHBU_ENABLE_CUSTOM_DIALOG=ON
cmake --build build-release --target vita-homebrew-update.suprx-self HomebrewUpdateClient
```

On the validated Windows toolchain:

```text
C:\vitasdk-tools\cmake-4.4.3-windows-x86_64\bin\cmake.exe -S . -B build-release -G "Unix Makefiles" -DCMAKE_MAKE_PROGRAM=C:\msys64\usr\bin\make.exe -DVHBU_ENABLE_CUSTOM_DIALOG=ON
C:\vitasdk-tools\cmake-4.4.3-windows-x86_64\bin\cmake.exe --build build-release --target vita-homebrew-update.suprx-self HomebrewUpdateClient
```

Outputs:

```text
build-release/vita-homebrew-update.suprx
build-release/libHomebrewUpdateClient.a
```

Create the release bundle from a clean build:

```text
python host/package_release.py --build-dir build-release --output dist/v1.0.0
```

## Provenance and limitations

v1.0.0 is based on restored hardware-proven candidate 124 (SHA-256
`e2e650af272554b0649da209d51e09e7766aef21491f346bfbd3542d3f598202`,
168,053 bytes). Candidate 125 and its failed low-memory experiment are not
published. Production cleanup removes LAUTEST fallback configuration and
research-only targets, so a clean production build is validated by source,
configuration, and behavior rather than being represented as byte-identical.

See [docs/RELEASE_NOTES_v1.0.0.md](docs/RELEASE_NOTES_v1.0.0.md) for the
release-specific validation statement and known limitations, and
[THIRD_PARTY.md](THIRD_PARTY.md) for license attribution.
