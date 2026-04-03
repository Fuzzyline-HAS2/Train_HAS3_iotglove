import os
import re
import sys
import shutil
import subprocess
import glob

# Windows CMD 이모지 출력을 위한 설정
sys.stdout.reconfigure(encoding='utf-8')

# ============================================================
# [사용법] 이 파일을 각 기기 저장소의 scripts/ 폴더에 복사한 뒤
#          아래 변수들을 수정하세요.
#
#   SKETCH_NAME  : 메인 .ino 파일 이름 (확장자 제외)
#   VERSION_FILE : #define FIRMWARE_VER 가 있는 파일 경로
# ============================================================
SCRIPT_DIR    = os.path.dirname(os.path.abspath(__file__))
BASE_DIR      = os.path.dirname(SCRIPT_DIR)

# ── 기기 저장소에 맞게 변경 ─────────────────────────────────
SKETCH_NAME   = "updated_IoTglove"                                      # 메인 .ino 이름 (확장자 제외)
VERSION_FILE  = os.path.join(BASE_DIR, "library_and_pin.h")            # FIRMWARE_VER 가 있는 파일
VERSION_MACRO = "FIRMWARE_VER"
# ────────────────────────────────────────────────────────────

SKETCH_FILE   = os.path.join(BASE_DIR, f"{SKETCH_NAME}.ino")           # 빌드 탐색 기준 경로
OUTPUT_BIN    = os.path.join(BASE_DIR, "update.bin")
OUTPUT_SIG    = os.path.join(BASE_DIR, "update.sig")
VERSION_TXT   = os.path.join(BASE_DIR, "version.txt")

# ============================================================
# 비밀키: scripts/secrets.py 에서 관리 (GitHub 비공개)
# ============================================================
try:
    sys.path.insert(0, SCRIPT_DIR)
    from secrets import HMAC_SECRET
except ImportError:
    print("❌ 오류: scripts/secrets.py 파일이 없습니다.")
    print("   secrets.py.example 을 secrets.py 로 복사한 뒤 비밀키를 설정하세요.")
    sys.exit(1)

# ============================================================
# 버전 관련 함수
# ============================================================
def get_current_version():
    with open(VERSION_FILE, "r", encoding="utf-8") as f:
        content = f.read()
    match = re.search(rf'#define\s+{VERSION_MACRO}\s+(\d+)', content)
    return int(match.group(1)) if match else None

def increment_version(current_ver):
    new_ver = current_ver + 1
    with open(VERSION_FILE, "r", encoding="utf-8") as f:
        content = f.read()
    new_content = re.sub(
        rf'#define\s+{VERSION_MACRO}\s+\d+',
        f'#define {VERSION_MACRO} {new_ver}',
        content
    )
    with open(VERSION_FILE, "w", encoding="utf-8") as f:
        f.write(new_content)
    return new_ver

# ============================================================
# 빌드된 .bin 파일 탐색
#   대상 파일명: <SKETCH_NAME>.ino.bin
#   Arduino IDE 2.x: BASE_DIR/build/<보드>/updated_IoTglove.ino.bin
#   Arduino IDE 1.x: BASE_DIR/updated_IoTglove.ino.bin
# ============================================================
def find_newest_bin():
    target_name = f"{SKETCH_NAME}.ino.bin"

    search_patterns = [
        os.path.join(BASE_DIR, "build", "**", target_name),   # Arduino IDE 2.x
        os.path.join(BASE_DIR, target_name),                    # Arduino IDE 1.x
    ]

    candidates = []
    for pattern in search_patterns:
        candidates.extend(glob.glob(pattern, recursive=True))

    # output 파일(update.bin) 자기 자신 제외
    candidates = [f for f in candidates if os.path.abspath(f) != os.path.abspath(OUTPUT_BIN)]

    if not candidates:
        return None

    return max(candidates, key=os.path.getmtime)

# ============================================================
# GitHub 푸시
# ============================================================
def git_push(version):
    print("\n☁️ GitHub 에 업로드 중...")
    try:
        with open(VERSION_TXT, "w", encoding="utf-8") as f:
            f.write(str(version))
        print(f"📝 version.txt → v{version}")

        files_to_add = [
            "update.bin",
            "update.sig",
            "version.txt",
            os.path.relpath(VERSION_FILE, BASE_DIR).replace("\\", "/"),
        ]
        subprocess.run(["git", "-C", BASE_DIR, "add"] + files_to_add, check=True)
        subprocess.run(
            ["git", "-C", BASE_DIR, "commit", "-m", f"Firmware Update v{version}"],
            check=True
        )
        subprocess.run(["git", "-C", BASE_DIR, "push"], check=True)
        print("✅ GitHub 업로드 완료!")
    except subprocess.CalledProcessError as e:
        print(f"❌ Git 오류: {e}")
        print("   git 설치 및 저장소 연결 상태를 확인하세요.")

# ============================================================
# 메인
# ============================================================
def main():
    print("🚀 SecureOTA 배포 자동화 시작...")
    print(f"   스케치  : {SKETCH_FILE}")
    print(f"   버전 파일: {VERSION_FILE}")
    print(f"   버전 매크로: {VERSION_MACRO}")

    # 1. 비밀키 검증
    if HMAC_SECRET == "CHANGE_THIS_TO_YOUR_SECRET":
        print("❌ 오류: scripts/secrets.py 의 HMAC_SECRET 을 설정하세요.")
        return

    # 2. 현재 버전 읽기 및 증가
    cur_ver = get_current_version()
    if cur_ver is None:
        print(f"❌ 오류: {VERSION_FILE} 에서 '#define {VERSION_MACRO}' 를 찾을 수 없습니다.")
        return

    print(f"\n현재 버전: v{cur_ver}")
    new_ver = increment_version(cur_ver)
    print(f"🔼 버전 변경: v{cur_ver} → v{new_ver}  ({VERSION_FILE} 업데이트됨)")

    # 3. 아두이노 IDE 에서 바이너리 내보내기 대기
    print(f"\n⏳ [행동 필요] 아두이노 IDE 에서 Ctrl+Alt+S 를 실행하세요.")
    print(f"   (스케치 → 컴파일된 바이너리 내보내기)")
    print(f"   탐색 대상: {SKETCH_NAME}.ino.bin")
    print(f"   완료되면 Enter 를 누르세요...")
    input()

    # 4. .bin 파일 탐색
    print("🔎 빌드 파일 탐색 중...")
    bin_file = find_newest_bin()
    if not bin_file:
        print(f"❌ {SKETCH_NAME}.ino.bin 파일을 찾을 수 없습니다.")
        print(f"   탐색 위치: {BASE_DIR}/build/**/")
        print(f"   아두이노 IDE 에서 Ctrl+Alt+S 를 먼저 실행하세요.")
        return

    print(f"   발견: {os.path.relpath(bin_file, BASE_DIR)}")

    # 5. update.bin 으로 복사
    try:
        shutil.copy2(bin_file, OUTPUT_BIN)
        print(f"📦 → update.bin 복사 완료")
    except Exception as e:
        print(f"❌ 파일 복사 실패: {e}")
        return

    # 6. HMAC-SHA256 서명 생성
    sign_script = os.path.join(SCRIPT_DIR, "sign_firmware.py")
    result = subprocess.run(
        [sys.executable, sign_script, OUTPUT_BIN, HMAC_SECRET, OUTPUT_SIG],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"❌ 서명 실패:\n{result.stderr}")
        return
    print("🔏 서명 완료 → update.sig (32 bytes)")

    # 7. GitHub 푸시
    git_push(new_ver)

    print(f"\n🎉 배포 완료! v{new_ver} 이(가) GitHub 에 업로드되었습니다.")
    print("   서버에서 device_state = \"github\" 를 전송하면 기기가 업데이트됩니다.")

if __name__ == "__main__":
    main()
