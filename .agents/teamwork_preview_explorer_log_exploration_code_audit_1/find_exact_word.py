with open(r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl", "r", encoding="utf-8") as f:
    for i, line in enumerate(f, 1):
        if "familiar" in line.lower():
            print(f"File 1 Line {i}: {line[:300]}")

with open(r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl", "r", encoding="utf-8") as f:
    for i, line in enumerate(f, 1):
        if "familiar" in line.lower():
            print(f"File 2 Line {i}: {line[:300]}")
