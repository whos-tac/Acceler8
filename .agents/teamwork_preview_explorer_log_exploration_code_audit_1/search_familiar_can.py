import json
import re

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\familiar_can_search.txt"

with open(out_path, "w", encoding="utf-8") as out_f:
    for lf in log_files:
        out_f.write(f"=== {lf} ===\n")
        try:
            with open(lf, "r", encoding="utf-8") as f:
                for i, line in enumerate(f, 1):
                    if "familiar" in line.lower() or "can" in line.lower() or "speed" in line.lower():
                        try:
                            obj = json.loads(line)
                            for k in ["content", "thinking", "output"]:
                                val = obj.get(k, "")
                                if val and isinstance(val, str) and ("familiar" in val.lower() or "speed" in val.lower()):
                                    out_f.write(f"Line {i} ({k}):\n")
                                    # Print matching sentences
                                    sentences = re.split(r'[.!?\n]', val)
                                    for s in sentences:
                                        if "familiar" in s.lower() or "speed" in s.lower():
                                            out_f.write(f"  - {s.strip()}\n")
                        except Exception:
                            pass
        except Exception as e:
            out_f.write(f"Error: {e}\n")

print("Done.")
