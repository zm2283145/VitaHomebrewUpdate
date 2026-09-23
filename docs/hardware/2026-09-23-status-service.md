# Status service hardware gate — 2026-09-23

## Result

Passed on a retail PS Vita at `10.1.1.93` using the installed `LAUTEST01`
version 01.00 test application.

The application reported:

```text
Update service: 1
installed, hooks unavailable (use fallback)
```

This proves that the `*main` plugin loaded, its worker started, SceNet became
available through late binding, the loopback listener accepted a connection on
TCP port 13379, and the client parsed the JSON status response.

## Proven plugin

```text
file: vita-homebrew-update.suprx
size: 6506 bytes
sha256: 992e1c047caa05103d5f36ccd83c6ae3645d3e35321bc75909371dc751c26cc0
```

The plugin was installed at `ur0:tai/vita-homebrew-update.suprx` beneath the
`*main` section. The FTP installer verified the plugin and config by readback
and preserved timestamped recovery copies before replacement.

## Device log

```text
module_start result=0 hex=0x00000000
thread_started result=1073807557 hex=0x400100C5
server_thread_started result=0 hex=0x00000000
net_resolve_retry result=-1878982654 hex=0x90010002
socket_ready result=6 hex=0x00000006
service_ready result=1 hex=0x00000001
```

The initial network export lookup failed while SceNet was unavailable, then
succeeded after six half-second retries. This validates the delayed-resolution
design and explains why a normal application-style SUPRX could not load at
SceShell startup.

## Safety conclusion

Status `1` is the correct published value. Applications detect the plugin but
continue using their own updater. Status `2` remains gated on a validated,
firmware-allowlisted LiveArea hook and must not be enabled early.
