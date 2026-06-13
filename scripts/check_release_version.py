#!/usr/bin/env python3
import argparse

from firmware_metadata import read_firmware_version, strip_v, version_code_for


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tag", required=True, help="Git tag such as v1.2.4-dev.1")
    args = parser.parse_args()

    firmware = read_firmware_version()
    tag_version = strip_v(args.tag)
    if firmware.version != tag_version:
        raise SystemExit(
            f"Tag/version mismatch: tag {args.tag} != firmware {firmware.version}"
        )

    computed_code = version_code_for(args.tag)
    if firmware.version_code != computed_code:
        raise SystemExit(
            "Firmware version code mismatch: "
            f"{firmware.version_code} != computed {computed_code}"
        )

    print(f"Release version OK: {args.tag} / code {firmware.version_code}")


if __name__ == "__main__":
    main()
