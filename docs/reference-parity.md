# HomebrewUpdate reference parity checklist

This project does not ship a device-test build until the guarded path matches
the reference plugin's observable behavior and safety checks.

## Discovery and confirmation

- Read `sce_sys/homebrew_update.ini` for the active title.
- Support HTTP and HTTPS update XML URLs, including redirects.
- Compare installed and available versions before publishing the update icon.
- Display the native `Homebrew Update` confirmation with installed version,
  available version, formatted size, complete multiline release notes, and the
  `Do you want to download now` question.
- Display `Please wait...` after Download is confirmed and keep Shell input
  locked until BGDL registration completes or fails.

Offline comparison of the decrypted 3.65 retail Shell and HomebrewUpdate 1.6
ELF confirms that the reference hooks Shell offset `0x2AD898` (the same
controller method as `livearea_update_hook`). Shell action `0` calls its
cleanup routine, action `1` writes `0x80104301` to controller offset `0x10c`,
and action `2` starts the native update lookup. In the reference hook, action
`2` first reads the controller's nine-byte title ID at offset `0x92`, resolves
that title's update URL, and checks for an `http://` or `https://` prefix;
for that path it invokes the original controller's
action `1` and then starts its own update work. The opt-in custom-dialog
experiment now uses that action-2 handoff for the configured title rather
than action `0`; it records each handoff for device-side verification.
Shell's action `1` writes a server-error code, so the full modal and
input-lock behavior must still be checked on device before treating this
as reference parity. The custom-dialog build option remains off by default.
The custom branch checks the controller title field and forwards actions
for other titles unchanged. Start-button launch traces do not exercise this
action.

## Background download

- Register the VPK with the native background downloader.
- Preserve native notification-page download progress, pause, resume, and
  cancel behavior.
- Restore interrupted downloads after reboot when possible.
- Suppress the generic package install failure before exporting the completed
  VPK from `ux0:bgdl`.
- Never extract or promote a package from a BGDL completion hook.

## Verification and staging

- Verify exact file size and SHA-1 from the update XML.
- Validate ZIP/VPK structure and every entry CRC.
- Reject unsafe paths and malformed archives.
- Verify the VPK Title ID, application version, and Content ID.
- Delete invalid or incomplete packages and publish the matching reference
  error notification.
- Persist staged-update state across reboot.
- Rewrite the completed notification to `Waiting to Install` only after every
  check succeeds.

The first active notification candidate caused a full Shell lockup. Its
retained log ends inside the delayed service-worker call that removes the
native `LOGSTATUS0` row; verification and notification creation were never
reached. A second candidate that gated noisy identity-hook logging locked at
the same operation, proving that the database call location—not the logging
storm—was the immediate blocker. A third candidate moved the deletion into
the reference export callback and locked in the same helper before the VPK
move. The reference hook itself has the same post-original-call ordering, so
callback timing alone does not explain the deadlock.

The next candidate no longer opens or writes `app.db`. Open-source Shell
plugins demonstrate that `SceLsdb` can create the final actionable entry
directly:

- BetterHomebrewBrowser supplies a complete notification object to
  `sceLsdbSendNotification` and suppresses the generic BGDL toast at Shell
  offset `0xFD94C`.
- NeoVitaDB-Downloader provides a pure-C implementation using the private
  0x100-byte Shell notification object.
- Princess-of-Sleeping's SceShell Notification PoC documents the matching
  3.65 retail object initializer, UTF-8 setter, and cleanup offsets.

The direct candidate signature-checks those helpers, resolves
`sceLsdbSendNotification` by NID, creates a `Waiting to Install` notification
owned by the target title, and points its app-open action at the existing
Photos transport URI. The URI hook consumes that URI and opens the staged
Install/Cancel prompt. The SQL row suppression, plain NotificationUtil send,
identity substitution, row polling, and row rewrite are no longer part of
the active build.

Device testing proved that direct `sceLsdbSendNotification` safely creates
the intended `Waiting to Install` entry. The notification-center action does
not pass through Shell's public URI/name imports; its verified outer launch
gate title is `NPXS10015`. A candidate that combined direct replacement of
the native `LOGSTATUS0` row with a hook at `0xFD94C` wedged Shell during
delayed hook installation and was rolled back to candidate 110. Keep the
next test isolated: use the separate target-owned notification and intercept
only the verified `NPXS10015` gate while staged state is ready. Native toast
suppression and row replacement remain disabled until tested independently.

Candidate 115 validates the complete actionable-notification path on 3.65.
The notification center calls `sceAppMgrLaunchAppByName` for `NPXS10004`
with mode `0x70000` and passes the transport URI as its parameter. Intercepting
that exact title in the outer name-import hook while staged state is ready
prevents Photos from opening and queues the existing Install/Cancel prompt.
The successful trace records:

- `test_waiting_notification_result = 0`
- `notification_name_action = 0x70000`
- `notification_name_parameter =
  photo:browse?category=ALL&hbu=LAUTEST08:1`

Candidate 115 is therefore the current notification baseline (SHA-256
`3191714edf7fdf89ca26979f90d691343626208d3e9a905297f82e348f7b4ff9`,
167194 bytes). Preserve it on-device while implementing the remaining
Download Complete -> Checking File -> Waiting to Install transition in a
separate candidate.

The next candidate takes ownership of the existing BGDL notification key
`LOGSTATUS0/<task-id>` rather than adding a second title-owned line. It follows
BetterHomebrewBrowser's narrow suppression contract at Shell offset `0xFD94C`:
the export callback creates `ux0:bgdl/t/<task-id>/.installed`, and the toast
hook returns early only for argument zero when that exact task marker exists.
The hook additionally requires the plugin's active task ID to match while its
download state is between registration and verification; unrelated Shell
state changes never touch the filesystem or inspect a task object.

Device testing showed that leaving the `0xFD94C` hook installed from Shell
startup makes the native update lookup fail with `0x80410136` before BGDL
registration, even when the hook forwards every call. It is now installed
only after `lau_bgdl_enqueue` returns a valid task ID and released as soon as
that task's completion row is replaced or capture fails.

Candidates 116 through 119 were configured in fresh build directories without
the candidate-115 cache setting `VHBU_ENABLE_CUSTOM_DIALOG=ON`. Those builds
therefore exposed Sony's stock update dialog and did not exercise the proven
Homebrew Update handoff. All subsequent notification candidates must set that
CMake option explicitly.
The worker then replaces the same LSDB item with `Download complete`, advances
it to non-actionable `Checking File...` before hashing/extraction/metadata
checks, and replaces it with the candidate-115 actionable
`Waiting to Install` state only after all checks pass. No direct `app.db`
access or native-row SQL mutation is used.

Candidate 120 is the proven single-row baseline: device testing confirmed the
same `LOGSTATUS0/<task-id>` row advances through all three states, remains
actionable at `Waiting to Install`, and does not emit `Could not install`.
It set the notification display byte only for `Download complete`, however,
so collapsed notifications did not pop out again for `Checking File...` or
`Waiting to Install`. Candidate 121 changes only that display byte to `1` for
all three publishes. Device testing with the `01.29` release confirmed that
`Download complete`, `Checking File...`, and `Waiting to Install` each pop out
when the row is collapsed. Candidate 121 is the new baseline; candidate 120
remains the immediate fallback.

Candidate 122 adds the final successful-install lifecycle state. Only after
the promoter operation succeeds, the installed `param.sfo` passes
post-install verification, and the progress UI closes, the plugin replaces
the same `LOGSTATUS0/<task-id>` row with non-actionable `Install completed`.
The replacement has action type `0` and no executable title or argument, so
it cannot reopen the installer and remains a normal clearable notification.
Device testing with the `01.30` release confirmed the completed row is
non-clickable and can be deleted normally. Candidate 122 is the new baseline;
candidate 121 remains the immediate fallback.

Candidate 123 addresses reboot resurrection after a user clears the completed
notification. Candidate 122 left the finished `ux0:bgdl/t/<task-id>` directory
behind after installation; Shell could reconcile that stale task during the
next boot and generate `Could not install` even though installation had
succeeded. After promoter success and installed-SFO verification, candidate
123 removes only the exact positive task-ID directory through a separately
guarded filesystem API and logs `test_bgdl_task_cleanup_result`. It does not
touch `app.db`, active downloads, or arbitrary BGDL paths. Device testing with
the `01.32` release confirmed that, after installation, clearing `Install
completed`, and rebooting again, `Could not install` does not return.
Post-reboot inspection showed an empty `ux0:bgdl/t` directory and zero matching
notification rows in a read-only `app.db` snapshot. Candidate 123 is the new
baseline; candidate 122 remains the immediate fallback.

Candidate 124 fixes the title-scoped client status endpoint. The client test
app calls `HomebrewUpdateClientGetStatusForTitle`, but the server still
accepted only the early literal `/status/LAUTEST01` route, causing configured
titles such as `LAUTEST08` to receive HTTP 404 and report `-1` while the
global service correctly published `2`. The route now accepts exactly one
nine-character title ID and returns status only when that title exists in the
dynamically discovered configuration list. It no longer performs legacy
late-hook installation as a side effect of a status query. Device testing
confirmed the existing test app now reports status `2`. Candidate 124 is the
new baseline; candidate 123 remains the immediate fallback.

Candidate 124 is not compatible with the VitaDB background daemon on the
tested device: opening a configured title's update path with both enabled can
lock Shell and corrupt LiveArea background rendering. The conflict trace
shows no URL misrouting or foreign BGDL task capture before the lock. Memory
pressure is the leading cause: candidate 124 reserves 538 KiB of BSS plus two
64 KiB worker stacks, while the daemon reserves a roughly 1.3 MiB taipool, a
256 KiB stack, and 1 MiB each for HTTP and SSL initialization. Candidate 125
removes unused notification-database scratch storage, reduces XML and HTTPS
buffers to their bounded protocol needs, caps per-title cached changeinfo at
4 KiB, and reduces the worker stacks. Candidate 124 remains the daemon-off
rollback baseline until coexistence is hardware-tested.

Candidate 125 reduced BSS from 538 KiB to 200 KiB and reduced combined worker
stacks by 48 KiB. It booted normally with the VitaDB daemon disabled and
reported service status `2`. With the daemon enabled, boot initially appeared
normal, but VitaCompanion FTP reads became unstable after the daemon's delayed
network initialization, and opening the configured update hard-locked Shell
and damaged LiveArea background rendering exactly as before. Memory reduction
therefore did not resolve the conflict. The daemon was disabled again and
candidate 124 was restored and readback-verified. Treat the VitaDB daemon as a
confirmed incompatible `*main` plugin until a narrower root cause is proven;
do not ship candidate 125 as a coexistence fix.

Candidate 124 was also hardware-tested with an active PKGj download. PKGj
continued downloading while a configured Homebrew Update package was queued.
The Homebrew Update package received BGDL task ID `4`, the PKGj game completed
and installed normally, and the Homebrew Update package then completed its
download without notification or task interference. This validates practical
PKGj coexistence for the tested sequence. The registration trace still showed
one activation of the narrowly timed `hbu_aux_b_hook` fallback before the BGDL
service supplied task ID `4`; the fallback should still be tightened to prove
task ownership before broader production claims.

The same test left both completion notifications visible and rebooted without
clearing them. PKGj's notification persisted, while Homebrew Update's
`Install completed` row disappeared. A read-only post-reboot `app.db` snapshot
contained no `LOGSTATUS0` or `LAUTEST08` rows, and `ux0:bgdl/t` was empty.
This is accepted behavior: the final row remains task-owned and is transient
once the verified-install path removes its BGDL task tree. Most importantly,
reboot did not create a duplicate or resurrect `Could not install`.

Relocated static analysis of HomebrewUpdate 1.6 confirms that it removes the
native task row synchronously in the BGDL export callback, before moving the
VPK. On the successful integrity path it then captures the maximum event
rowid, arms title-identity substitution only around the plain notification
send, requires the substitution to fire, polls up to 30 times at 100 ms for
the resulting title-owned row, and rewrites it in a single
`BEGIN IMMEDIATE`/`UPDATE`/`SELECT changes()`/`COMMIT` transaction. The next
candidate follows that ordering. Its URI hook validates the title and numeric
task suffix before opening the staged-install prompt, preventing Photos from
receiving the transport URI. It remains undeployed pending a clean build and
review of the private database resolver. The resolver must prefer
`SceSqliteVsh`; accepting generic `SceSqlite` first can bind the wrong
implementation inside Shell. Updater-owned serialization is also bounded so
a stale guard fails rather than spinning Shell indefinitely.

## Installation entry points

- Opening `Waiting to Install` and tapping the LiveArea update icon again must
  reach the same staged-install confirmation.
- The confirmation text must state that the update is already downloaded and
  ask whether it should be installed now.
- Detect a running target application, ask to close it, close only that title,
  wait for shutdown, and apply the reference stabilization delay.

On the 3.65 retail `LAUTEST08` test title (installed 01.15, staged 01.16),
passive Shell traces show that pressing Start reaches the internal command
handler at offset `0x1CF8E2`. Its 36-byte payload has command type `1` followed
by `LAUTEST08`; the handler returns `0`. A subsequent command has type `0` and
title `NPXS19999` and also returns `0`. The failed
`sceAppMgrGetStatusByName` call at offset `0x29C060` is followed by a
successful lookup at `0x1CF974` with app ID `0x11004`; the former is not by
itself proof of the launch failure. The reference plugin's successful
installation log also treats `0x80802012` as "target app not running."
A subsequent passive hook confirmed that
the enclosing Shell routine at `0x29BFC6` returns a nonnegative value
(which varies between runs) to its caller at `0x29C7C8`, which treats only
negative results as failures. A later passive hook on
the `SceLsdb` import at caller offset `0x29C8E6` installed successfully but
recorded no call when Start failed; the caller at `0x29C7C8` has multiple
branches before that import. One branch compares a title to the firmware
string `NPXS10006`; that comparison is not evidence of an `LAUTEST08`
registration failure. The visible transient "cannot find application" error
still needs a precise rejection branch. Inspect the remaining branches and
compare the reference plugin offline before another device probe. Do not
bypass a guessed check or edit `app.db` based on these calls.

Read-only `app.db` snapshots after the reference plugin installed 01.15 and
after the custom plugin's unsuccessful Start have identical `LAUTEST08`
`tbl_appinfo` values except a modification timestamp. In particular, the
installed version, executable path, content ID, and `0x5142196B` key
(value `512`, queried by the later Shell `SceLsdb` call) match.
The reference's successful installation log does not establish that Start
subsequently launched the app. Start follows Shell's application-launch path;
the update-icon action is a separate LiveArea path. Keep the launch symptom
separate from update-icon parity until each is independently demonstrated.

An opt-in device experiment intercepts only the previously traced type-1,
36-byte Start command for the configured title, at the verified retail Shell
dispatcher callsite, while a verified staged update is pending. It accepts
that command without launching the old executable and queues the existing
install worker. Other commands and titles still use the original handler.
This Start-to-install behavior is distinct from the reference plugin's
update-icon handoff and must be verified on device; it does not write
`app.db`.

The 01.17 device test disproved this as a complete Start solution:
`start_command_staged_install_count` incremented, but Shell flashed
"cannot find application" before returning to LiveArea and showing our
running-application install prompt. Canceling preserved the staged 01.17
package. Shell's earlier launch-preparation routine at `0x29C744` had
already returned a nonnegative app ID before the command hook ran, so
swallowing the later command does not cancel the launch attempt. Do not
mistake the eventual custom install prompt for native Start parity or
promote this experimental interception as the final behavior.

Returning zero from the inner `0x29BFC6` launch-preparation helper also left
the visible sequence unchanged. The outer routine at `0x29C744` continued
building the launch command after that helper returned.

The next guarded experiment hooks the outer `0x29C744` launch gate itself.
Before calling the original routine, it resolves the title from the gate's
string object at offset `0x20`. For the configured title only, while a
verified staged update is pending, it queues installation and returns zero
without letting the original routine build an AppMgr launch command. Dialogs
and promotion remain asynchronous; after successful installation the
existing completion UI submits a fresh launch rather than blocking Shell's
UI thread and attempting to resume the intercepted call. Inner launch hooks
are passive again. This remains a device experiment, not established parity.

The 01.17 device test validated the outer gate: the old launch error did not
appear, the native progress UI opened, promotion completed, and 01.17 was
installed. A Start-gate install now skips the redundant Install complete
Launch/Cancel prompt and submits the saved intent as a fresh title launch
after the progress UI closes. Update-icon installations retain the explicit
Launch/Cancel completion prompt.

## Native installation UI

- Lock Shell input for the installation transition.
- Display the native `Installing application` / `Installing...` progress UI.
- Promote synchronously with PromoterUtil and check its final state/result.
- Verify the installed version from `ux0:app/<TITLE_ID>/sce_sys/param.sfo`.
- Publish `Install complete` only after verification succeeds.
- Ask `Do you want to launch this application now?` and honor both choices.
- Unlock Shell and keep LiveArea responsive after success, cancellation, and
  every failure path.

## Persistence and compatibility

- Maintain multiple pending titles without one title blocking another.
- Reconcile the pending database and staged packages at startup.
- Preserve the local status API on TCP port 13379.
- Preserve the client-library fallback contract for applications with their
  own updater.
- Keep unrelated applications, networking, and launch behavior unchanged.
