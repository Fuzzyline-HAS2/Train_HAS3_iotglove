#!/usr/bin/env python3
import argparse
import hashlib
import pathlib
import re
from dataclasses import dataclass


ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "library_and_pin.h"


SEMVER_RE = re.compile(
    r"^v?(?P<major>0|[1-9]\d*)\."
    r"(?P<minor>0|[1-9]\d*)\."
    r"(?P<patch>0|[1-9]\d*)"
    r"(?:-(?P<kind>dev|rc)\.(?P<build>0|[1-9]\d*))?$"
)


@dataclass(frozen=True)
class FirmwareVersion:
    version: str
    version_code: int


def read_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def read_firmware_version(header: pathlib.Path = HEADER) -> FirmwareVersion:
    text = read_text(header)
    version_match = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', text)
    code_match = re.search(r"#define\s+FIRMWARE_VERSION_CODE\s+([0-9]+)", text)
    if not version_match or not code_match:
        raise SystemExit(f"Missing firmware version defines in {header}")
    return FirmwareVersion(version_match.group(1), int(code_match.group(1)))


def parse_semver(tag_or_version: str):
    match = SEMVER_RE.match(tag_or_version)
    if not match:
        raise SystemExit(f"Invalid firmware SemVer: {tag_or_version}")
    major = int(match.group("major"))
    minor = int(match.group("minor"))
    patch = int(match.group("patch"))
    kind = match.group("kind") or "prd"
    build = int(match.group("build") or "0")
    return major, minor, patch, kind, build


def channel_for(tag_or_version: str) -> str:
    _, _, _, kind, _ = parse_semver(tag_or_version)
    return kind


def version_code_for(tag_or_version: str) -> int:
    major, minor, patch, kind, build = parse_semver(tag_or_version)
    release_type = {"dev": 1, "rc": 2, "prd": 3}[kind]
    return (
        major * 10000000
        + minor * 100000
        + patch * 1000
        + release_type * 100
        + build
    )


def strip_v(tag_or_version: str) -> str:
    return tag_or_version[1:] if tag_or_version.startswith("v") else tag_or_version


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=["version", "version-code", "channel", "computed-code"])
    parser.add_argument("value", nargs="?")
    args = parser.parse_args()

    if args.command == "version":
        print(read_firmware_version().version)
    elif args.command == "version-code":
        print(read_firmware_version().version_code)
    elif args.command == "channel":
        if not args.value:
            raise SystemExit("channel requires a tag or version")
        print(channel_for(args.value))
    elif args.command == "computed-code":
        if not args.value:
            raise SystemExit("computed-code requires a tag or version")
        print(version_code_for(args.value))


if __name__ == "__main__":
    main()
