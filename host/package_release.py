#!/usr/bin/env python3
"""Package reproducible VitaHomebrewUpdate release assets."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import zipfile
from pathlib import Path


ARCHIVE_TIME = (2026, 1, 1, 0, 0, 0)


def add_file(archive: zipfile.ZipFile, source: Path, name: str) -> None:
    info = zipfile.ZipInfo(name, ARCHIVE_TIME)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    archive.writestr(info, source.read_bytes())


def git_output(root: Path, *arguments: str) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, default=Path("build-release"))
    parser.add_argument("--output", type=Path, default=Path("dist/v1.0.1"))
    parser.add_argument("--source-ref", default="HEAD")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    build = args.build_dir.resolve()
    output = args.output.resolve()
    dist_root = (root / "dist").resolve()
    if output == dist_root or not output.is_relative_to(dist_root):
        parser.error("--output must be a child directory of the repository's dist/")

    status = git_output(root, "status", "--porcelain")
    if status:
        parser.error("source tree must be clean before release packaging")
    source_revision = git_output(root, "rev-parse", args.source_ref)
    head_revision = git_output(root, "rev-parse", "HEAD")
    if source_revision != head_revision:
        parser.error("--source-ref must resolve to the checked-out HEAD")

    provenance_path = build / "vhbu-build-revision.txt"
    if not provenance_path.is_file():
        parser.error(f"missing build provenance: {provenance_path}")
    provenance = provenance_path.read_text(encoding="ascii").splitlines()
    if provenance != [source_revision, "clean"]:
        parser.error(
            "build artifacts were not configured from the clean source revision"
        )

    plugin = build / "vita-homebrew-update.suprx"
    library = build / "libHomebrewUpdateClient.a"
    for artifact in (plugin, library):
        if not artifact.is_file():
            parser.error(f"missing build artifact: {artifact}")

    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    shutil.copy2(plugin, output / plugin.name)
    shutil.copy2(root / "examples/homebrew_update.ini",
                 output / "homebrew_update.ini")
    shutil.copy2(root / "README.md", output / "README.md")
    shutil.copy2(root / "docs/INSTALL.md", output / "INSTALL.md")

    sdk = output / "vita-homebrew-update-client-v1.0.1.zip"
    with zipfile.ZipFile(sdk, "w") as archive:
        add_file(archive, root / "client/include/homebrew_update_client.h",
                 "include/homebrew_update_client.h")
        add_file(archive, root / "client/src/homebrew_update_client.c",
                 "src/homebrew_update_client.c")
        add_file(archive, root / "common/pending_update.h",
                 "src/pending_update.h")
        add_file(archive, library, "lib/libHomebrewUpdateClient.a")

    source = output / "vita-homebrew-update-v1.0.1-source.zip"
    subprocess.run(
        ["git", "archive", "--format=zip", f"--output={source}",
         source_revision],
        cwd=root,
        check=True,
    )

    (output / "BUILD_PROVENANCE.txt").write_text(
        "VitaHomebrewUpdate v1.0.1\n"
        f"source_commit={source_revision}\n"
        "build_state=clean\n"
        "candidate_124_sha256="
        "e2e650af272554b0649da209d51e09e7766aef21491f346bfbd3542d3f598202\n"
        "candidate_124_size=168053\n"
        "production_note=LAUTEST fallback configuration and research targets "
        "removed\n",
        encoding="ascii",
    )

    checksum_paths = sorted(
        path for path in output.iterdir()
        if path.is_file() and path.name != "SHA256SUMS"
    )
    checksums = "".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
        for path in checksum_paths
    )
    (output / "SHA256SUMS").write_text(checksums, encoding="ascii")
    print(checksums, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
