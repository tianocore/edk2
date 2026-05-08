#!/usr/bin/env python3
"""
Update CodeQL CLI dependency versions and CodeQL query pack version.

This script updates:
  - BaseTools/Plugin/CodeQL/codeqlcli_ext_dep.yaml
  - BaseTools/Plugin/CodeQL/codeqlcli_linux_ext_dep.yaml
  - BaseTools/Plugin/CodeQL/codeqlcli_windows_ext_dep.yaml
  - BaseTools/Plugin/CodeQL/CodeQlQueries.qls

SHA256 digests are pulled from the GitHub release metadata and the
codeql/cpp-queries version is pulled from qlpack.yml in the corresponding
CodeQL CLI branch. If no CodeQL version is supplied on the command line, the
latest published CodeQL CLI release is used.

Note: this script depends on GitHub release-asset digests being present in the
release metadata. GitHub started exposing those digests on 2025-06-03:
https://github.blog/changelog/2025-06-03-releases-now-expose-digests-for-release-assets/
Releases published before that change may not have `digest` values in the API.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Dict


SCRIPT_DIR = Path(__file__).resolve().parent

EXT_DEP_FILES = {
    "codeql.zip": SCRIPT_DIR / "codeqlcli_ext_dep.yaml",
    "codeql-linux64.zip": SCRIPT_DIR / "codeqlcli_linux_ext_dep.yaml",
    "codeql-win64.zip": SCRIPT_DIR / "codeqlcli_windows_ext_dep.yaml",
}
QUERY_FILE = SCRIPT_DIR / "CodeQlQueries.qls"


def _http_get(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "edk2-codeql-updater"})
    with urllib.request.urlopen(req) as resp:
        return resp.read()


def _http_get_json(url: str) -> dict:
    return json.loads(_http_get(url).decode("utf-8"))


def _http_get_text(url: str) -> str:
    return _http_get(url).decode("utf-8")


def _extract_sha256_from_text(text: str) -> str:
    # Expected content is either "<sha256>" or "<sha256> <filename>".
    token = text.strip().split()[0]
    if not re.fullmatch(r"[0-9a-fA-F]{64}", token):
        raise ValueError(f"Invalid sha256 text content: {text.strip()!r}")
    return token.lower()


def fetch_latest_codeql_version() -> str:
    latest_release_url = (
        "https://api.github.com/repos/github/codeql-cli-binaries/releases/latest"
    )
    latest_release = _http_get_json(latest_release_url)
    tag_name = latest_release.get("tag_name") or ""
    if not isinstance(tag_name, str) or not tag_name:
        raise ValueError("Unable to determine latest CodeQL version from GitHub")
    return tag_name.lstrip("v")


def fetch_release_sha256_map(codeql_version: str) -> Dict[str, str]:
    release_url = (
        "https://api.github.com/repos/github/codeql-cli-binaries/releases/tags/"
        f"v{codeql_version}"
    )
    release = _http_get_json(release_url)
    assets = {asset["name"]: asset for asset in release.get("assets", [])}

    sha_map: Dict[str, str] = {}
    for asset_name in EXT_DEP_FILES:
        asset = assets.get(asset_name)
        if asset is None:
            raise KeyError(
                f"Release v{codeql_version} does not include required asset: {asset_name}"
            )

        digest = asset.get("digest") or ""
        if digest.startswith("sha256:"):
            sha_map[asset_name] = digest.split(":", 1)[1].lower()
            continue

        sha_asset = assets.get(f"{asset_name}.checksum.txt")
        if sha_asset:
            sha_text = _http_get_text(sha_asset["browser_download_url"])
            sha_map[asset_name] = _extract_sha256_from_text(sha_text)
            continue

        raise KeyError(
            f"Unable to find SHA256 for {asset_name} in release v{codeql_version}"
        )

    return sha_map


def fetch_cpp_queries_version(codeql_version: str) -> str:
    qlpack_url = (
        "https://raw.githubusercontent.com/github/codeql/"
        f"codeql-cli/v{codeql_version}/cpp/ql/src/qlpack.yml"
    )
    qlpack_text = _http_get_text(qlpack_url)

    # qlpack.yml for cpp queries uses:
    #   name: codeql/cpp-queries
    #   version: <pack version>
    if "name: codeql/cpp-queries" not in qlpack_text:
        raise ValueError(
            f"Unable to validate cpp queries pack in qlpack.yml for v{codeql_version}"
        )

    match = re.search(r"(?m)^\s*version:\s*([0-9A-Za-z.\-_]+)\s*$", qlpack_text)
    if not match:
        raise ValueError(
            f"Unable to parse codeql/cpp-queries version for v{codeql_version}"
        )
    return match.group(1)


def read_text(path: Path) -> str:
    with path.open("r", encoding="utf-8", newline="") as f:
        return f.read()


def detect_newline_style(text: str) -> str:
    if "\r\n" in text:
        return "\r\n"
    return "\n"


def normalize_newlines(text: str, newline: str) -> str:
    normalized = text.replace("\r\n", "\n")
    if newline == "\r\n":
        return normalized.replace("\n", "\r\n")
    return normalized


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="")


def replace_or_fail(pattern: str, replacement: str, text: str, path: Path) -> str:
    new_text, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise ValueError(f"Expected one match for pattern {pattern!r} in {path}")
    return new_text


def update_ext_dep_file(path: Path, asset_name: str, codeql_version: str, sha256: str) -> bool:
    original = read_text(path)
    newline_style = detect_newline_style(original)
    text = original
    source_url = (
        "https://github.com/github/codeql-cli-binaries/releases/download/"
        f"v{codeql_version}/{asset_name}"
    )

    text = replace_or_fail(
        r'("source"\s*:\s*")[^"]+(")',
        rf'\g<1>{source_url}\g<2>',
        text,
        path,
    )
    text = replace_or_fail(
        r'("version"\s*:\s*")[^"]+(")',
        rf'\g<1>{codeql_version}\g<2>',
        text,
        path,
    )
    text = replace_or_fail(
        r'("sha256"\s*:\s*")[0-9a-fA-F]{64}(")',
        rf'\g<1>{sha256}\g<2>',
        text,
        path,
    )

    if text != original:
        write_text(path, normalize_newlines(text, newline_style))
        return True
    return False


def update_queries_file(path: Path, cpp_queries_version: str) -> bool:
    original = read_text(path)
    newline_style = detect_newline_style(original)
    text = replace_or_fail(
        r"(from:\s*codeql/cpp-queries@)[0-9A-Za-z.\-_]+",
        rf"\g<1>{cpp_queries_version}",
        original,
        path,
    )
    if text != original:
        write_text(path, normalize_newlines(text, newline_style))
        return True
    return False


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Update CodeQL CLI versions and cpp query pack version."
    )
    parser.add_argument(
        "--codeql-version",
        help=(
            "CodeQL CLI version (for example: 2.24.1). If omitted, the latest "
            "published release is used."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Calculate and print updates without writing files.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    codeql_version = (
        args.codeql_version.lstrip("v")
        if args.codeql_version
        else fetch_latest_codeql_version()
    )

    try:
        sha_map: Dict[str, str] = fetch_release_sha256_map(codeql_version)

        for asset_name, sha in sha_map.items():
            if not re.fullmatch(r"[0-9a-f]{64}", sha):
                raise ValueError(f"Invalid SHA256 value for {asset_name}: {sha}")

        cpp_queries_version = fetch_cpp_queries_version(codeql_version)

        print(f"CodeQL version: v{codeql_version}")
        print(f"codeql/cpp-queries version: {cpp_queries_version}")
        for asset_name in sorted(sha_map):
            print(f"{asset_name} sha256: {sha_map[asset_name]}")

        if args.dry_run:
            print("Dry run: no files were modified.")
            return 0

        changed_files = []
        for asset_name, path in EXT_DEP_FILES.items():
            if update_ext_dep_file(path, asset_name, codeql_version, sha_map[asset_name]):
                changed_files.append(path)
        if update_queries_file(QUERY_FILE, cpp_queries_version):
            changed_files.append(QUERY_FILE)

        if changed_files:
            print("Updated files:")
            for path in changed_files:
                print(f"  - {path}")
        else:
            print("No file changes were necessary.")
        return 0

    except urllib.error.HTTPError as err:
        print(f"HTTP error while fetching release data: {err}", file=sys.stderr)
    except urllib.error.URLError as err:
        print(f"Network error while fetching release data: {err}", file=sys.stderr)
    except (KeyError, ValueError) as err:
        print(f"Error: {err}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
