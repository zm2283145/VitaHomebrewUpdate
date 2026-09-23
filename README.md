# VitaHomebrewUpdate

An open PS Vita homebrew update system aimed at making homebrew updates feel
like official LiveArea updates. Development proceeds in small, hardware-tested
gates so an unsupported shell hook cannot create a boot loop.

## Current milestone

The repository currently provides:

- a source-compatible `HomebrewUpdateClient` library;
- a loopback status service at `http://127.0.0.1:13379/status`;
- a disposable `LAUTEST01` app that demonstrates plugin/fallback selection;
- a GitHub-ready XML feed generator with size, SHA-1, and SHA-256 metadata;
- a native download probe retained for hardware research.

The service intentionally returns status `1` (hook error) in this milestone.
Applications must use their own updater until the LiveArea hook and complete
download/install path pass their firmware gates. It does not falsely advertise
status `2` yet.

## Client integration

Initialize SceNet, link `libHomebrewUpdateClient.a`, and call:

```c
int status = HomebrewUpdateClientGetStatus();
```

The return values are compatible with the documented HomebrewUpdate contract:

| Value | Meaning | Application behavior |
| ---: | --- | --- |
| `-1` | Plugin not detected | Use the application's updater |
| `0` | Plugin disabled | Use the application's updater |
| `1` | Plugin loaded, hook unavailable | Use the application's updater |
| `2` | Plugin fully ready | Use the LiveArea update path |

The client timeout is 250 ms, so a missing plugin does not stall application
startup.

## Build

Install VitaSDK and configure with its toolchain:

```text
cmake -S . -B build
cmake --build build
```

Important outputs:

```text
build/libHomebrewUpdateClient.a
build/vita-homebrew-update.suprx
build/lau-test-app-01.00.vpk
```

Build the test update separately:

```text
cmake -S . -B build-01.01 -DLAU_TEST_APP_VERSION=01.01
cmake --build build-01.01
```

## Status-service hardware test

The plugin is a `*main` user plugin and borrows SceShell's initialized network
stack. Copy `vita-homebrew-update.suprx` to `ur0:tai/`, add it beneath `*main`
in `ur0:tai/config.txt`, then reboot. Install and run the 01.00 test app.

Expected result for the current milestone:

```text
Update service: 1
installed, hooks unavailable (use fallback)
```

Remove the config entry if the shell becomes unstable. The current service has
no code patch and binds only to loopback, but `*main` plugins should always be
tested cautiously.

With Vita Companion FTP active, the backup-aware installer can perform the
copy and config update:

```text
python host/install_plugin.py build/vita-homebrew-update.suprx --vita VITA_IP
```

It preserves a timestamped copy of `ur0:tai/config.txt`, backs up an older
plugin when present, uses staged uploads, and verifies both final files by
readback. Reboot the Vita after it succeeds.

## Update feed

The test VPK embeds:

```text
https://raw.githubusercontent.com/zm2283145/VitaHomebrewUpdate/main/test-feed/LAUTEST01-ver.xml
```

Generate a feed and release asset from a final VPK with:

```text
python host/prepare_update.py build-01.01/lau-test-app-01.01.vpk \
  --root test-feed --version 01.01 \
  --base-url https://github.com/OWNER/REPO/releases/download/TAG
```

The generator creates the update XML, change information, JSON metadata, and a
release-ready copy of the VPK. Never modify the VPK after generating its hashes.

## Roadmap

1. Validate the localhost service and fallback behavior on retail 3.65.
2. Parse per-title `sce_sys/homebrew_update.ini` configuration.
3. Add HTTPS XML lookup, semantic version comparison, and strict metadata
   validation.
4. Implement resumable background VPK download and native notification state.
5. Verify VPK structure, Title ID, version, size, and SHA-1 before promotion.
6. Add a firmware-allowlisted LiveArea refresh/update hook.
7. Install a completed update before title launch, with recovery-safe staging.

No signature bypass is planned. Homebrew VPK installation will use the existing
homebrew PromoterUtil path after strict validation, while official package
authentication remains untouched.

## License and third-party work

See `THIRD_PARTY.md` for adapted research and attribution. A project license
will be selected before the first stable release.
