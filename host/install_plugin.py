#!/usr/bin/env python3
"""Install the VitaHomebrewUpdate user plugin through Vita Companion FTP."""

from __future__ import annotations

import argparse
import ftplib
import hashlib
import io
import json
from datetime import datetime, timezone
from pathlib import Path


REMOTE_DIRECTORY = "/ur0:/tai"
CONFIG_NAME = "config.txt"
PLUGIN_NAME = "vita-homebrew-update.suprx"
CONFIG_ENTRY = f"ur0:tai/{PLUGIN_NAME}"
MAX_CONFIG_SIZE = 256 * 1024
MAX_PLUGIN_SIZE = 2 * 1024 * 1024


def read_remote(ftp: ftplib.FTP, name: str, limit: int) -> bytes:
    chunks: list[bytes] = []
    size = 0

    def collect(block: bytes) -> None:
        nonlocal size
        size += len(block)
        if size > limit:
            raise RuntimeError(f"remote file exceeds safety limit: {name}")
        chunks.append(block)

    ftp.retrbinary(f"RETR {name}", collect)
    return b"".join(chunks)


def read_optional(ftp: ftplib.FTP, name: str, limit: int) -> bytes | None:
    try:
        return read_remote(ftp, name, limit)
    except ftplib.error_perm as exc:
        if str(exc).startswith("550"):
            return None
        raise


def upload_verified(ftp: ftplib.FTP, name: str, data: bytes) -> None:
    ftp.storbinary(f"STOR {name}", io.BytesIO(data))
    received = read_remote(ftp, name, max(len(data), 1))
    if received != data:
        raise RuntimeError(f"FTP readback mismatch: {name}")


def updated_config(original: bytes) -> tuple[bytes, bool]:
    if b"\x00" in original:
        raise RuntimeError("taiHEN config contains a NUL byte")
    try:
        text = original.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise RuntimeError("taiHEN config is not valid UTF-8") from exc

    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    active_entries = [
        line.strip()
        for line in lines
        if line.strip() and not line.lstrip().startswith("#")
    ]
    if CONFIG_ENTRY in active_entries:
        return original, False

    main_indexes = [index for index, line in enumerate(lines) if line.strip() == "*main"]
    if len(main_indexes) != 1:
        raise RuntimeError("expected exactly one *main section in ur0:tai/config.txt")
    insert_at = main_indexes[0] + 1
    while insert_at < len(lines) and not lines[insert_at].strip().startswith("*"):
        insert_at += 1
    lines.insert(insert_at, CONFIG_ENTRY)
    while len(lines) > 1 and lines[-1] == "" and lines[-2] == "":
        lines.pop()
    return newline.join(lines).encode("utf-8"), True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("plugin", type=Path)
    parser.add_argument("--vita", required=True)
    parser.add_argument("--port", type=int, default=1337)
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

    plugin = args.plugin.resolve()
    if not plugin.is_file():
        parser.error(f"plugin does not exist: {plugin}")
    plugin_data = plugin.read_bytes()
    if not plugin_data or len(plugin_data) > MAX_PLUGIN_SIZE:
        parser.error("plugin size is outside the accepted range")

    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    config_backup = f"{CONFIG_NAME}.pre-vhbu-{stamp}"
    plugin_backup = f"{PLUGIN_NAME}.pre-vhbu-{stamp}"
    config_part = f"{CONFIG_NAME}.vhbu-part"
    plugin_part = f"{PLUGIN_NAME}.part"

    with ftplib.FTP() as ftp:
        ftp.connect(args.vita, args.port, timeout=args.timeout)
        ftp.login()
        ftp.voidcmd("TYPE I")
        ftp.cwd(REMOTE_DIRECTORY)

        original_config = read_remote(ftp, CONFIG_NAME, MAX_CONFIG_SIZE)
        new_config, config_changed = updated_config(original_config)
        old_plugin = read_optional(ftp, PLUGIN_NAME, MAX_PLUGIN_SIZE)

        upload_verified(ftp, config_backup, original_config)
        if old_plugin is not None:
            upload_verified(ftp, plugin_backup, old_plugin)
        upload_verified(ftp, plugin_part, plugin_data)
        upload_verified(ftp, config_part, new_config)

        if old_plugin is not None:
            ftp.delete(PLUGIN_NAME)
        ftp.rename(plugin_part, PLUGIN_NAME)

        if config_changed:
            ftp.delete(CONFIG_NAME)
            try:
                ftp.rename(config_part, CONFIG_NAME)
            except Exception:
                upload_verified(ftp, CONFIG_NAME, original_config)
                raise
        else:
            ftp.delete(config_part)

        final_plugin = read_remote(ftp, PLUGIN_NAME, MAX_PLUGIN_SIZE)
        final_config = read_remote(ftp, CONFIG_NAME, MAX_CONFIG_SIZE)
        if final_plugin != plugin_data or final_config != new_config:
            raise RuntimeError("final FTP readback verification failed")

    print(
        json.dumps(
            {
                "config_backup": f"ur0:tai/{config_backup}",
                "config_changed": config_changed,
                "config_entry": CONFIG_ENTRY,
                "plugin": f"ur0:tai/{PLUGIN_NAME}",
                "plugin_backup": (
                    f"ur0:tai/{plugin_backup}" if old_plugin is not None else None
                ),
                "plugin_sha256": hashlib.sha256(plugin_data).hexdigest(),
                "plugin_size": len(plugin_data),
                "status": "installed_and_verified",
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
