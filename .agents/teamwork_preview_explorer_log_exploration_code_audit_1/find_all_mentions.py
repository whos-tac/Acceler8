import json

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

for lf in log_files:
    print(f"=== {lf} ===")
    count = 0
    with open(lf, "r", encoding="utf-8") as f:
        for i, line in enumerate(f, 1):
            if "can" in line.lower() and ("issue" in line.lower() or "problem" in line.lower() or "fail" in line.lower() or "speed" in line.lower() or "display" in line.lower() or "slave" in line.lower() or "master" in line.lower()):
                try:
                    obj = json.loads(line)
                    content = obj.get("content", "")
                    thinking = obj.get("thinking", "")
                    text = f"{content} {thinking}"
                    if "familiar" in text.lower() or "speed" in text.lower() or "slave" in text.lower() or "voltage" in text.lower():
                        # print snippet
                        print(f"Line {i}: {text[:160]}...")
                        count += 1
                        if count >= 30:
                            break
                except Exception:
                    pass
