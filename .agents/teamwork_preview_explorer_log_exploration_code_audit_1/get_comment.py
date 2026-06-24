import json

log_path = r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
with open(log_path, "r", encoding="utf-8") as f:
    for i, line in enumerate(f, 1):
        if i == 318:
            obj = json.loads(line)
            print("CONTENT:")
            print(obj.get("content", ""))
            print("THINKING:")
            print(obj.get("thinking", ""))
            break
