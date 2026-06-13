#!/usr/bin/env python3
import argparse
import ctypes
from ctypes import wintypes
from pathlib import Path
import sys
import time
import winreg


DEFAULT_PC_BAUD = 921600
DEFAULT_NEXTION_BAUD = 9600
DEFAULT_UPLOAD_BAUD = 921600
DEFAULT_CHUNK_SIZE = 4096
COLOR_ENABLED = False


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3
FILE_ATTRIBUTE_NORMAL = 0x80
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value
PURGE_RXCLEAR = 0x0008
PURGE_TXCLEAR = 0x0004
CLRDTR = 6
CLRRTS = 4
STD_OUTPUT_HANDLE = -11
ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004

COLORS = {
    "reset": "\033[0m",
    "cyan": "\033[96m",
    "green": "\033[92m",
    "yellow": "\033[93m",
    "red": "\033[91m",
    "dim": "\033[90m",
}


class DCB(ctypes.Structure):
    _fields_ = [
        ("DCBlength", wintypes.DWORD),
        ("BaudRate", wintypes.DWORD),
        ("flags", wintypes.DWORD),
        ("wReserved", wintypes.WORD),
        ("XonLim", wintypes.WORD),
        ("XoffLim", wintypes.WORD),
        ("ByteSize", wintypes.BYTE),
        ("Parity", wintypes.BYTE),
        ("StopBits", wintypes.BYTE),
        ("XonChar", ctypes.c_char),
        ("XoffChar", ctypes.c_char),
        ("ErrorChar", ctypes.c_char),
        ("EofChar", ctypes.c_char),
        ("EvtChar", ctypes.c_char),
        ("wReserved1", wintypes.WORD),
    ]


class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ("ReadIntervalTimeout", wintypes.DWORD),
        ("ReadTotalTimeoutMultiplier", wintypes.DWORD),
        ("ReadTotalTimeoutConstant", wintypes.DWORD),
        ("WriteTotalTimeoutMultiplier", wintypes.DWORD),
        ("WriteTotalTimeoutConstant", wintypes.DWORD),
    ]


kernel32.CreateFileW.argtypes = [
    wintypes.LPCWSTR,
    wintypes.DWORD,
    wintypes.DWORD,
    wintypes.LPVOID,
    wintypes.DWORD,
    wintypes.DWORD,
    wintypes.HANDLE,
]
kernel32.CreateFileW.restype = wintypes.HANDLE
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL
kernel32.BuildCommDCBW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(DCB)]
kernel32.BuildCommDCBW.restype = wintypes.BOOL
kernel32.SetCommState.argtypes = [wintypes.HANDLE, ctypes.POINTER(DCB)]
kernel32.SetCommState.restype = wintypes.BOOL
kernel32.SetCommTimeouts.argtypes = [wintypes.HANDLE, ctypes.POINTER(COMMTIMEOUTS)]
kernel32.SetCommTimeouts.restype = wintypes.BOOL
kernel32.ReadFile.argtypes = [
    wintypes.HANDLE,
    wintypes.LPVOID,
    wintypes.DWORD,
    ctypes.POINTER(wintypes.DWORD),
    wintypes.LPVOID,
]
kernel32.ReadFile.restype = wintypes.BOOL
kernel32.WriteFile.argtypes = [
    wintypes.HANDLE,
    wintypes.LPCVOID,
    wintypes.DWORD,
    ctypes.POINTER(wintypes.DWORD),
    wintypes.LPVOID,
]
kernel32.WriteFile.restype = wintypes.BOOL
kernel32.PurgeComm.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel32.PurgeComm.restype = wintypes.BOOL
kernel32.EscapeCommFunction.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel32.EscapeCommFunction.restype = wintypes.BOOL
kernel32.QueryDosDeviceW.argtypes = [wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD]
kernel32.QueryDosDeviceW.restype = wintypes.DWORD
kernel32.GetStdHandle.argtypes = [wintypes.DWORD]
kernel32.GetStdHandle.restype = wintypes.HANDLE
kernel32.GetConsoleMode.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel32.GetConsoleMode.restype = wintypes.BOOL
kernel32.SetConsoleMode.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel32.SetConsoleMode.restype = wintypes.BOOL


def win_error(message):
    code = ctypes.get_last_error()
    return OSError(code, f"{message}: {ctypes.FormatError(code)}")


class UserCancelled(Exception):
    pass


def setup_output(no_color=False):
    global COLOR_ENABLED
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")

    COLOR_ENABLED = (not no_color) and sys.stdout.isatty()
    if not COLOR_ENABLED:
        return

    handle = kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
    mode = wintypes.DWORD(0)
    if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
        kernel32.SetConsoleMode(handle, mode.value | ENABLE_VIRTUAL_TERMINAL_PROCESSING)


def paint(text, color):
    if not COLOR_ENABLED:
        return text
    return COLORS[color] + text + COLORS["reset"]


def section(title):
    print()
    print(paint("=" * 60, "cyan"))
    print(paint(title, "cyan"))
    print(paint("=" * 60, "cyan"))


def info(message):
    print(paint("[안내] ", "cyan") + message, flush=True)


def ok(message):
    print(paint("[완료] ", "green") + message, flush=True)


def warn(message):
    print(paint("[주의] ", "yellow") + message, flush=True)


def fail(message):
    print(paint("[오류] ", "red") + message, flush=True)


def fmt_size(size):
    mib = size / (1024 * 1024)
    return f"{mib:.2f} MiB"


class WinSerial:
    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.handle = None

    def __enter__(self):
        name = self.port
        if not name.startswith("\\\\.\\"):
            name = "\\\\.\\" + name

        handle = kernel32.CreateFileW(
            name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            None,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            None,
        )
        if handle == INVALID_HANDLE_VALUE:
            raise win_error(f"open {self.port}")

        self.handle = handle
        self._configure(self.baud)
        kernel32.EscapeCommFunction(self.handle, CLRDTR)
        kernel32.EscapeCommFunction(self.handle, CLRRTS)
        self.purge()
        return self

    def __exit__(self, exc_type, exc, tb):
        if self.handle is not None:
            kernel32.CloseHandle(self.handle)
            self.handle = None

    def _configure(self, baud):
        dcb = DCB()
        dcb.DCBlength = ctypes.sizeof(DCB)
        settings = f"baud={baud} parity=N data=8 stop=1"
        if not kernel32.BuildCommDCBW(settings, ctypes.byref(dcb)):
            raise win_error("BuildCommDCB")
        if not kernel32.SetCommState(self.handle, ctypes.byref(dcb)):
            raise win_error("SetCommState")

        timeouts = COMMTIMEOUTS()
        timeouts.ReadIntervalTimeout = 1
        timeouts.ReadTotalTimeoutMultiplier = 0
        timeouts.ReadTotalTimeoutConstant = 50
        timeouts.WriteTotalTimeoutMultiplier = 0
        timeouts.WriteTotalTimeoutConstant = 10000
        if not kernel32.SetCommTimeouts(self.handle, ctypes.byref(timeouts)):
            raise win_error("SetCommTimeouts")

    def purge(self):
        kernel32.PurgeComm(self.handle, PURGE_RXCLEAR | PURGE_TXCLEAR)

    def write(self, data):
        if isinstance(data, str):
            data = data.encode("ascii")
        data = bytes(data)
        total = 0
        while total < len(data):
            chunk = data[total:]
            written = wintypes.DWORD(0)
            buf = ctypes.create_string_buffer(chunk)
            ok = kernel32.WriteFile(self.handle, buf, len(chunk), ctypes.byref(written), None)
            if not ok:
                raise win_error("WriteFile")
            if written.value == 0:
                raise TimeoutError("serial write timeout")
            total += written.value

    def write_line(self, line):
        self.write(line + "\n")

    def read(self, size, timeout):
        deadline = time.monotonic() + timeout
        out = bytearray()
        while len(out) < size and time.monotonic() < deadline:
            remaining = min(size - len(out), 4096)
            buf = ctypes.create_string_buffer(remaining)
            read = wintypes.DWORD(0)
            ok = kernel32.ReadFile(self.handle, buf, remaining, ctypes.byref(read), None)
            if not ok:
                raise win_error("ReadFile")
            if read.value:
                out.extend(buf.raw[: read.value])
            else:
                time.sleep(0.001)
        return bytes(out)

    def read_line(self, timeout):
        deadline = time.monotonic() + timeout
        out = bytearray()
        while time.monotonic() < deadline:
            b = self.read(1, min(0.1, max(0.001, deadline - time.monotonic())))
            if not b:
                continue
            if b == b"\n":
                return out.decode("ascii", errors="replace").rstrip("\r")
            out.extend(b)
        return None


def find_tft_files(folder):
    folder = Path(folder).expanduser().resolve()
    files = sorted({p.resolve() for p in folder.glob("*.tft")} | {p.resolve() for p in folder.glob("*.TFT")})
    return files


def default_tft_folder():
    script_path = Path(__file__).resolve()
    candidates = [
        script_path.parent.parent / "tft",
        script_path.parent.parent / "nextion_tft_uploader" / "tft",
        Path.cwd() / "tft",
    ]

    for folder in candidates:
        if folder.exists():
            return str(folder)

    return str(candidates[0])


def port_sort_key(port):
    text = port.upper()
    if text.startswith("COM") and text[3:].isdigit():
        return (0, int(text[3:]))
    return (1, text)


def list_com_ports_from_registry():
    ports = set()
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DEVICEMAP\SERIALCOMM") as key:
            index = 0
            while True:
                try:
                    _name, value, _value_type = winreg.EnumValue(key, index)
                except OSError:
                    break
                if isinstance(value, str) and value.upper().startswith("COM"):
                    ports.add(value.upper())
                index += 1
    except OSError:
        pass
    return ports


def list_com_ports_from_dos_devices():
    ports = set()
    size = 65536
    buffer = ctypes.create_unicode_buffer(size)
    copied = kernel32.QueryDosDeviceW(None, buffer, size)
    if not copied:
        return ports

    for name in buffer[:copied].split("\x00"):
        upper = name.upper()
        if upper.startswith("COM") and upper[3:].isdigit():
            ports.add(upper)
    return ports


def list_com_ports():
    ports = list_com_ports_from_registry() | list_com_ports_from_dos_devices()
    return sorted(ports, key=port_sort_key)


def choose_tft(args):
    if args.tft:
        path = Path(args.tft).expanduser().resolve()
        if not path.exists():
            raise FileNotFoundError(path)
        section("TFT 파일 선택")
        ok(f"지정된 파일을 사용합니다: {path.name}")
        return path

    folder = Path(args.folder or ".").expanduser().resolve()
    files = find_tft_files(folder)
    if not files:
        raise FileNotFoundError(f"No .tft files found in {folder}")

    section("TFT 파일 선택")
    info(f"TFT 폴더: {folder}")

    if args.index is not None:
        if args.index < 1 or args.index > len(files):
            raise ValueError(f"--index must be 1..{len(files)}")
        ok(f"{args.index}번 파일을 선택했습니다: {files[args.index - 1].name}")
        return files[args.index - 1]

    if len(files) == 1:
        ok(f"업로드할 파일을 자동 선택했습니다: {files[0].name} ({fmt_size(files[0].stat().st_size)})")
        return files[0]

    info("업로드할 TFT 파일을 선택해주세요.")
    for i, path in enumerate(files, start=1):
        print(f"  [{i}] {path.name} ({fmt_size(path.stat().st_size)})")

    while True:
        raw = input(paint(f"번호를 입력하세요 [1-{len(files)}]: ", "yellow")).strip()
        if raw.isdigit() and 1 <= int(raw) <= len(files):
            ok(f"선택된 파일: {files[int(raw) - 1].name}")
            return files[int(raw) - 1]
        warn("목록에 있는 번호를 다시 입력해주세요.")


def wait_relay_ready(serial, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        serial.write_line("NXUP_HELLO")
        probe_deadline = time.monotonic() + 0.5
        while time.monotonic() < probe_deadline:
            line = serial.read_line(0.1)
            if line and line.startswith("NXUP_READY"):
                return
    raise TimeoutError("ESP32 relay did not respond. Check firmware, COM port, and PC baud.")


def probe_relay_port(port, args):
    try:
        with WinSerial(port, args.pc_baud) as serial:
            time.sleep(args.scan_boot_delay / 1000.0)
            serial.purge()
            wait_relay_ready(serial, args.scan_timeout / 1000.0)
            return True, None
    except Exception as exc:
        return False, str(exc)


def choose_detected_port(matches, args):
    if len(matches) == 1:
        return matches[0]

    warn("ESP32 업로드 포트가 여러 개 감지되었습니다.")
    for i, port in enumerate(matches, start=1):
        print(f"  [{i}] {port}")

    while True:
        raw = input(paint(f"사용할 포트 번호를 입력하세요 [1-{len(matches)}]: ", "yellow")).strip()
        if raw.isdigit() and 1 <= int(raw) <= len(matches):
            return matches[int(raw) - 1]
        warn("목록에 있는 번호를 다시 입력해주세요.")


def confirm_upload(port, tft_path, args):
    if args.yes:
        return

    section("업로드 확인")
    print(f"  TFT 파일 : {tft_path}")
    print(f"  ESP32 포트: {paint(port, 'green')}")
    raw = input(paint("이 포트로 업로드를 시작할까요? [Y/n]: ", "yellow")).strip().lower()
    if raw not in ("", "y", "yes"):
        raise UserCancelled("사용자가 업로드를 취소했습니다.")


def resolve_upload_port(args, tft_path):
    requested = (args.port or "auto").strip()
    if requested.lower() != "auto":
        port = requested.upper()
        info(f"자동 검색 대신 지정된 포트를 사용합니다: {port}")
        confirm_upload(port, tft_path, args)
        return port

    ports = list_com_ports()
    if not ports:
        raise RuntimeError("No COM ports found.")

    section("ESP32 포트 자동 검색")
    info("USB로 연결된 ESP32 업로드 포트를 찾는 중입니다.")
    info("검색된 COM 포트: " + ", ".join(ports))
    matches = []
    for port in ports:
        print(f"  - {port} 확인 중... ", end="", flush=True)
        found, reason = probe_relay_port(port, args)
        if found:
            print(paint("ESP32 업로더 감지", "green"), flush=True)
            matches.append(port)
        else:
            print(paint("건너뜀", "dim"), flush=True)
            if args.verbose_scan and reason:
                print(f"    {reason}")

    if not matches:
        raise RuntimeError("No ESP32 relay responded. Check USB cable, firmware, and that no serial monitor is open.")

    port = choose_detected_port(matches, args)
    ok(f"사용할 ESP32 포트: {port}")
    confirm_upload(port, tft_path, args)
    return port


def start_upload(serial, file_size, args):
    verify = 0 if args.no_connect else 1
    if args.resume:
        serial.write_line(f"RESUME {file_size} {args.upload_baud}")
    else:
        serial.write_line(f"BEGIN {file_size} {args.nextion_baud} {args.upload_baud} {verify}")

    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        line = serial.read_line(0.5)
        if not line:
            continue
        if line.startswith("INFO "):
            info("Nextion 응답 확인: " + line[5:])
            continue
        if line.startswith("BEGIN_OK "):
            return int(line.split()[1])
        if line.startswith("ERR "):
            raise RuntimeError(f"Relay begin failed: {line}")
    raise TimeoutError("Timed out waiting for BEGIN_OK.")


def wait_data_ack(serial, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = serial.read_line(0.5)
        if not line:
            continue
        if line.startswith("DATA_OK "):
            return int(line.split()[1])
        if line.startswith("DONE "):
            print(line, flush=True)
            continue
        if line.startswith("ERR "):
            raise RuntimeError(f"Relay upload failed: {line}")
    raise TimeoutError("Timed out waiting for DATA_OK.")


def wait_done(serial, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = serial.read_line(0.5)
        if not line:
            continue
        if line.startswith("DONE "):
            return line
        if line.startswith("ERR "):
            raise RuntimeError(f"Relay finish failed: {line}")
    return "DONE timeout_waiting_for_finish_line"


def upload_tft(path, args):
    file_size = path.stat().st_size
    if file_size <= 0:
        raise ValueError(f"TFT file is empty: {path}")

    section("업로드 시작")
    info(f"ESP32 포트 {args.port}에 연결합니다. 통신속도: {args.pc_baud} bps")
    with WinSerial(args.port, args.pc_baud) as serial:
        time.sleep(args.boot_delay / 1000.0)
        serial.purge()

        info("ESP32 업로드 모드 응답을 기다리는 중입니다.")
        wait_relay_ready(serial, args.ready_timeout / 1000.0)
        ok("ESP32 업로더와 연결되었습니다.")

        chunk_size = start_upload(serial, file_size, args)
        if chunk_size < 1 or chunk_size > DEFAULT_CHUNK_SIZE:
            raise ValueError(f"Relay returned invalid chunk size: {chunk_size}")

        section("업로드 진행")
        info(f"파일: {path.name} ({fmt_size(file_size)}, {file_size} bytes)")
        info(f"전송 단위: {chunk_size} bytes")
        sent = 0
        last_logged = -1
        with path.open("rb") as f:
            while sent < file_size:
                f.seek(sent)
                data = f.read(min(chunk_size, file_size - sent))
                if not data:
                    raise IOError(f"Failed to read TFT file at byte {sent}")

                serial.write_line(f"DATA {len(data)}")
                serial.write(data)

                sent = wait_data_ack(serial, 15)
                percent = int((sent * 100.0) / file_size)
                if args.progress_percent > 0 and (
                    percent == 100 or percent >= last_logged + args.progress_percent
                ):
                    last_logged = percent
                    print(paint(f"진행률 {percent:3d}%  {sent}/{file_size}", "green"), flush=True)

        done = wait_done(serial, 35)
        section("업로드 완료")
        if done == "DONE ready":
            ok("Nextion이 업로드 완료 신호를 보냈습니다.")
        elif done.startswith("DONE "):
            warn("Nextion의 마지막 ready 알림은 받지 못했지만, 파일 전송은 100% 완료되었습니다.")
            info(f"장치 응답: {done}")
        else:
            info(f"장치 응답: {done}")
        ok("업로드가 완료되었습니다. Nextion 화면이 재부팅될 수 있습니다.")


def parse_args():
    parser = argparse.ArgumentParser(description="Upload a Nextion .tft through the ESP32 IoT glove relay.")
    parser.add_argument("--port", default="auto", help="ESP32 COM port, or auto. Default: auto")
    parser.add_argument("--folder", default=default_tft_folder(), help="Folder containing .tft files. Shows a selection menu when multiple exist.")
    parser.add_argument("--tft", help="Direct path to a .tft file. Bypasses folder selection.")
    parser.add_argument("--index", type=int, help="Select the Nth .tft file from --folder without prompting.")
    parser.add_argument("--pc-baud", type=int, default=DEFAULT_PC_BAUD)
    parser.add_argument("--nextion-baud", type=int, default=DEFAULT_NEXTION_BAUD)
    parser.add_argument("--upload-baud", type=int, default=DEFAULT_UPLOAD_BAUD)
    parser.add_argument("--boot-delay", type=int, default=2500, help="Delay after opening COM port, ms.")
    parser.add_argument("--ready-timeout", type=int, default=10000, help="ESP32 relay ready timeout, ms.")
    parser.add_argument("--scan-boot-delay", type=int, default=2500, help="Delay after opening a COM port while scanning, ms.")
    parser.add_argument("--scan-timeout", type=int, default=3500, help="Per-port ESP32 relay scan timeout, ms.")
    parser.add_argument("--progress-percent", type=int, default=5)
    parser.add_argument("--no-connect", action="store_true", help="Skip Nextion connect probe before upload.")
    parser.add_argument("--resume", action="store_true", help="Resume an already-started Nextion upload.")
    parser.add_argument("--list-only", action="store_true", help="Only list/select the TFT file, do not upload.")
    parser.add_argument("--detect-only", action="store_true", help="Select TFT and detect ESP32 port, but do not upload.")
    parser.add_argument("-y", "--yes", action="store_true", help="Do not ask for upload confirmation.")
    parser.add_argument("--verbose-scan", action="store_true", help="Show why skipped COM ports did not match.")
    parser.add_argument("--no-color", action="store_true", help="Disable colored terminal output.")
    return parser.parse_args()


def main():
    args = parse_args()
    setup_output(args.no_color)
    try:
        tft = choose_tft(args)
        info(f"선택된 TFT 경로: {tft}")
        if args.list_only:
            return 0

        args.port = resolve_upload_port(args, tft)
        if args.detect_only:
            ok(f"ESP32 업로드 포트 감지 완료: {args.port}")
            return 0

        upload_tft(tft, args)
        return 0
    except UserCancelled as exc:
        warn(str(exc))
        return 0
    except KeyboardInterrupt:
        fail("사용자가 작업을 중단했습니다.")
        return 130
    except Exception as exc:
        fail(str(exc))
        print()
        warn("확인해주세요:")
        print("  1. ESP32가 USB로 연결되어 있는지 확인")
        print("  2. Arduino Serial Monitor나 다른 업로더가 COM 포트를 사용 중인지 확인")
        print("  3. ESP32에 Nextion TFT 업로드가 통합된 펌웨어가 들어있는지 확인")
        print("  4. 업로드할 .tft 파일이 tft 폴더 안에 있는지 확인")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
