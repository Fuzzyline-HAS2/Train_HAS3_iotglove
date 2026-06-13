#!/usr/bin/env python3
import argparse
import hmac
import hashlib
import os
import pathlib


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--secret-env", default="HMAC_SECRET")
    args = parser.parse_args()

    secret = os.environ.get(args.secret_env)
    if not secret:
        raise SystemExit(f"Missing {args.secret_env} environment variable")

    firmware_path = pathlib.Path(args.firmware)
    out_path = pathlib.Path(args.out)
    signature = hmac.new(
        secret.encode("utf-8"),
        firmware_path.read_bytes(),
        hashlib.sha256,
    ).digest()
    if len(signature) != 32:
        raise SystemExit("HMAC-SHA256 signature must be 32 bytes")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(signature)
    print(f"Wrote {out_path} ({len(signature)} bytes)")


if __name__ == "__main__":
    main()
