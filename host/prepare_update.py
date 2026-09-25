#!/usr/bin/env python3
"""Create a VitaHomebrewUpdate release manifest and matching assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from html import escape
from pathlib import Path
from urllib.parse import quote


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("artifact", type=Path)
    parser.add_argument("--root", type=Path, default=Path("release-feed"))
    parser.add_argument("--title-id", required=True)
    parser.add_argument("--version", required=True)
    location = parser.add_mutually_exclusive_group(required=True)
    location.add_argument("--host", help="PC address reachable by the Vita")
    location.add_argument("--base-url", help="Public directory or release URL")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--name", required=True)
    parser.add_argument("--content-id", required=True)
    parser.add_argument("--changes", required=True)
    args = parser.parse_args()

    artifact = args.artifact.resolve()
    if not artifact.is_file():
        parser.error(f"artifact does not exist: {artifact}")
    if len(args.title_id) != 9 or not args.title_id.isalnum() or args.title_id != args.title_id.upper():
        parser.error("--title-id must be exactly 9 uppercase letters/digits")

    args.root.mkdir(parents=True, exist_ok=True)
    safe_name = "".join(c for c in args.name if c.isalnum() or c in "-_")
    destination = args.root / f"{args.title_id}-{args.version}-{safe_name}.vpk"
    shutil.copyfile(artifact, destination)
    payload = destination.read_bytes()
    sha1 = hashlib.sha1(payload).hexdigest()
    sha256 = hashlib.sha256(payload).hexdigest()
    base_url = (
        args.base_url.rstrip("/")
        if args.base_url
        else f"http://{args.host}:{args.port}"
    )
    encoded_filename = quote(destination.name)
    xml_name = f"{args.title_id}-ver.xml"
    changes_name = f"{args.title_id}-changeinfo.xml"
    manifest = {
        "format": 1,
        "title_id": args.title_id,
        "version": args.version,
        "content_id": args.content_id,
        "url": f"{base_url}/{encoded_filename}",
        "size": destination.stat().st_size,
        "sha1": sha1,
        "sha256": sha256,
    }
    (args.root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    update_xml = f'''<?xml version="1.0" encoding="UTF-8"?>
<titlepatch status="alive" titleid="{escape(args.title_id)}">
  <tag name="{escape(args.title_id)}_T0" signoff="true">
    <package version="{escape(args.version)}" size="{destination.stat().st_size}"
        sha1sum="{sha1}" url="{escape(base_url)}/{encoded_filename}"
        psp2_system_ver="56623104" content_id="{escape(args.content_id)}">
      <paramsfo><title>{escape(args.name)}</title></paramsfo>
      <changeinfo url="{escape(base_url)}/{changes_name}"/>
    </package>
  </tag>
</titlepatch>
'''
    changes_xml = f'''<?xml version="1.0" encoding="UTF-8"?>
<changeinfo><changes><![CDATA[
{args.changes.replace("]]>", "] ]>")}
]]></changes></changeinfo>
'''
    (args.root / xml_name).write_text(update_xml, encoding="utf-8")
    (args.root / changes_name).write_text(changes_xml, encoding="utf-8")
    print(json.dumps(manifest, indent=2, sort_keys=True))
    print(f"Update XML: {base_url}/{xml_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
