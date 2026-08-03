#!/usr/bin/env python3
"""Create a deterministic, Vellum-ready rM2 release payload.

This packages already-built ARMv7 artifacts. It intentionally does not invoke
Docker or a compiler: the build environment is provenance, not a runtime
requirement of the release archive.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import subprocess
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "backend/entry": ROOT / "build-rm2" / "remarkable_chess_backend",
    "resources.rcc": ROOT / "packaging" / "appload-frontend" / "resources.rcc",
    "manifest.json": ROOT / "packaging" / "appload-frontend" / "manifest.json",
    "icon.png": ROOT / "packaging" / "icon.png",
    "LICENSE": ROOT / "LICENSE",
    "ASSET-NOTICES.md": ROOT / "packaging" / "appload-frontend" / "ASSET-NOTICES.md",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_armv7_executable(path: Path, name: str) -> None:
    output = subprocess.check_output(["file", "-b", str(path)], text=True)
    if "ELF 32-bit" not in output or "ARM" not in output:
        raise SystemExit(f"{name} is not an ARMv7 ELF executable: {output.strip()}")


def git_value(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def add_file(archive: tarfile.TarFile, name: str, path: Path) -> None:
    info = archive.gettarinfo(str(path), arcname=name)
    info.uid = 0
    info.gid = 0
    info.uname = "root"
    info.gname = "root"
    info.mtime = 0
    with path.open("rb") as source:
        archive.addfile(info, source)


def add_bytes(archive: tarfile.TarFile, name: str, content: bytes) -> None:
    info = tarfile.TarInfo(name)
    info.size = len(content)
    info.mode = 0o644
    info.uid = 0
    info.gid = 0
    info.uname = "root"
    info.gname = "root"
    info.mtime = 0
    archive.addfile(info, io.BytesIO(content))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="Release version, e.g. 0.1.0")
    parser.add_argument("--backend", type=Path, default=REQUIRED["backend/entry"], help="Built ARMv7 AppLoad backend")
    parser.add_argument("--stockfish", required=True, type=Path, help="Built ARMv7 Stockfish binary")
    parser.add_argument(
        "--nnue",
        type=Path,
        help="Optional external NNUE model. Omit when Stockfish embeds its network.",
    )
    parser.add_argument("--build-image", default="", help="Optional build provenance only")
    args = parser.parse_args()

    if args.version.startswith("v") or "/" in args.version:
        raise SystemExit("--version must be a plain version such as 0.1.0")

    files = dict(REQUIRED)
    files["backend/entry"] = args.backend.resolve()
    files["backend/stockfish"] = args.stockfish.resolve()
    if args.nnue is not None:
        files["nn-ab28990d4ea3.nnue"] = args.nnue.resolve()
    for name, path in files.items():
        if not path.is_file():
            raise SystemExit(f"missing required artifact {name}: {path}")

    if args.nnue is not None and not sha256(args.nnue).startswith("ab28990d4ea3"):
        raise SystemExit("NNUE SHA-256 does not match nn-ab28990d4ea3.nnue")
    require_armv7_executable(files["backend/entry"], "backend/entry")
    require_armv7_executable(files["backend/stockfish"], "backend/stockfish")

    prefix = f"remarkable-chess-rm2-v{args.version}"
    checksums = "\n".join(
        f"{sha256(path)}  {name}" for name, path in sorted(files.items())
    ) + "\n"
    provenance = {
        "app_version": args.version,
        "architecture": "armv7",
        "source_commit": git_value("rev-parse", "HEAD"),
        "source_tree_clean": not bool(git_value("status", "--porcelain")),
        "build_image": args.build_image or None,
        "nnue": "external" if args.nnue is not None else "embedded-in-stockfish",
        "payload_format": 1,
    }

    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    output = dist / f"{prefix}.tar.gz"
    with output.open("wb") as raw:
        with gzip.GzipFile(fileobj=raw, mode="wb", mtime=0, filename="") as compressed:
            with tarfile.open(fileobj=compressed, mode="w") as archive:
                for name, path in sorted(files.items()):
                    add_file(archive, f"{prefix}/{name}", path)
                add_bytes(archive, f"{prefix}/SHA256SUMS", checksums.encode())
                add_bytes(
                    archive,
                    f"{prefix}/BUILD-PROVENANCE.json",
                    (json.dumps(provenance, indent=2, sort_keys=True) + "\n").encode(),
                )

    print(output)
    print(f"sha256  {sha256(output)}")


if __name__ == "__main__":
    main()
