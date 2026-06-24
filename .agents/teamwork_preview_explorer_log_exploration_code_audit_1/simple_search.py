with open(r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl", "r", encoding="utf-8") as f:
    for i, line in enumerate(f, 1):
        if "familiar" in line.lower() or "slave" in line.lower() or "telemetry" in line.lower() or "speed" in line.lower():
            if "can" in line.lower():
                print(f"Line {i} matches: {line[:120]}")
