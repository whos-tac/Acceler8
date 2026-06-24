import json
import re
import sys

# Reconfigure stdout to handle UTF-8 on Windows
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

patterns = [
    re.compile(r"speed", re.IGNORECASE),
    re.compile(r"can", re.IGNORECASE),
    re.compile(r"vesc", re.IGNORECASE),
    re.compile(r"familiar", re.IGNORECASE),
    re.compile(r"slave", re.IGNORECASE),
]

for lf in log_files:
    print(f"=== Searching {lf} ===")
    try:
        with open(lf, "r", encoding="utf-8") as f:
            for i, line in enumerate(f, 1):
                try:
                    obj = json.loads(line)
                    found = False
                    matched_text = ""
                    for key in ["content", "thinking", "output"]:
                        val = obj.get(key, "")
                        if val and isinstance(val, str):
                            # Look for terms
                            v_lower = val.lower()
                            if ("speed" in v_lower or "vesc" in v_lower or "slave" in v_lower or "familiar" in v_lower) and ("can" in v_lower or "telemetry" in v_lower or "display" in v_lower):
                                found = True
                                # Find context around the matches
                                snippet = ""
                                lines = val.split('\n')
                                for line_no, l in enumerate(lines):
                                    l_lower = l.lower()
                                    if any(term in l_lower for term in ["speed", "vesc", "slave", "familiar"]):
                                        snippet += f"  Line {line_no}: {l[:150]}\n"
                                matched_text += f"[{key}]:\n{snippet[:400]}\n"
                    if found:
                        print(f"Line {i} in jsonl matched:\n{matched_text}")
                except Exception as e:
                    # Fallback to simple regex on raw line
                    if any(p.search(line) for p in patterns):
                        print(f"Line {i} (raw match): {line[:200]}")
    except Exception as e:
        print(f"Error reading {lf}: {e}")
