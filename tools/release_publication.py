"""An explicit version + application digest authorizes one stable release.

Without a matching request, the workflow only prepares a draft. Keeping an old
request in git cannot authorize publication of a later firmware version.
"""
import json
from pathlib import Path
import re


def publication_requested(request, manifest):
    if request is None:
        return False
    if not isinstance(request, dict) or set(request) != {"version", "sha256"}:
        raise ValueError("Publication request must contain exactly version and sha256")
    if not isinstance(request["version"], str) or not re.fullmatch(r"\d+\.\d+\.\d+", request["version"]):
        raise ValueError("Invalid publication version")
    if not isinstance(request["sha256"], str) or not re.fullmatch(r"[0-9a-f]{64}", request["sha256"]):
        raise ValueError("Invalid publication SHA-256")
    if request["version"] != manifest["version"]:
        return False
    if request["sha256"] != manifest["sha256"]:
        raise ValueError("Publication request does not match the application bytes")
    return True


if __name__ == "__main__":
    repo = Path(__file__).resolve().parent.parent
    request_file = repo / ".github/release-publication.json"
    request = json.loads(request_file.read_text()) if request_file.exists() else None
    manifest = json.loads((repo / "ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ota-manifest.json").read_text())
    print("true" if publication_requested(request, manifest) else "false")
