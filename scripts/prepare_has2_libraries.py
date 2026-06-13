#!/usr/bin/env python3
import argparse
import pathlib
import shutil


LIBRARIES = {
    "HAS2_Wifi": {
        "sentence": "HAS2 HTTP WiFi helper library.",
        "depends": "ArduinoJson",
        "includes": "HAS2_Wifi.h",
    },
    "HAS2_MQTT": {
        "sentence": "HAS2 MQTT helper library.",
        "depends": "PubSubClient",
        "includes": "HAS2_MQTT.h",
    },
    "HAS2_Vibration": {
        "sentence": "HAS2 vibration motor helper library.",
        "depends": "",
        "includes": "HAS2_Vibration.h",
    },
}


SKIP_DIRS = {".git", ".claude", ".omc", ".vscode", "__pycache__"}


def copytree_clean(src: pathlib.Path, dst: pathlib.Path) -> None:
    if dst.exists():
        shutil.rmtree(dst)

    def ignore(_, names):
        return [name for name in names if name in SKIP_DIRS]

    shutil.copytree(src, dst, ignore=ignore)


def write_library_properties(dst: pathlib.Path, name: str, meta: dict) -> None:
    depends = f"depends={meta['depends']}\n" if meta["depends"] else ""
    content = (
        f"name={name}\n"
        "version=1.0.0\n"
        "author=Fuzzyline-HAS2\n"
        "maintainer=Fuzzyline-HAS2\n"
        f"sentence={meta['sentence']}\n"
        f"paragraph={meta['sentence']}\n"
        "category=Communication\n"
        f"url=https://github.com/Fuzzyline-HAS2/{name}\n"
        "architectures=esp32\n"
        f"includes={meta['includes']}\n"
        f"{depends}"
    )
    (dst / "library.properties").write_text(content, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-root",
        default=str(pathlib.Path.home() / "OneDrive" / "문서" / "Arduino" / "libraries"),
    )
    parser.add_argument("--out", default="build/has2-libraries")
    args = parser.parse_args()

    source_root = pathlib.Path(args.source_root)
    out_root = pathlib.Path(args.out)
    out_root.mkdir(parents=True, exist_ok=True)

    for name, meta in LIBRARIES.items():
        src = source_root / name
        if not src.exists():
            raise SystemExit(f"Missing source library: {src}")
        dst = out_root / name
        copytree_clean(src, dst)
        write_library_properties(dst, name, meta)
        print(f"Prepared {dst}")

    print("Create private GitHub repos, commit each folder, and tag each as v1.0.0.")


if __name__ == "__main__":
    main()
