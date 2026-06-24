import json

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\can_speed_matches.txt"

with open(out_path, "w", encoding="utf-8") as out_f:
    for lf in log_files:
        out_f.write(f"=== {lf} ===\n")
        try:
            with open(lf, "r", encoding="utf-8") as f:
                for i, line in enumerate(f, 1):
                    try:
                        obj = json.loads(line)
                        for key in ["content", "thinking", "output"]:
                            val = obj.get(key, "")
                            if val and isinstance(val, str):
                                val_lower = val.lower()
                                if "can" in val_lower and "speed" in val_lower and ("issue" in val_lower or "problem" in val_lower or "display" in val_lower or "show" in val_lower or "work" in val_lower or "zero" in val_lower):
                                    out_f.write(f"Line {i} ({key}):\n")
                                    # Write lines that contain these
                                    lines = val.split('\n')
                                    for line_idx, l in enumerate(lines):
                                        l_lower = l.lower()
                                        if "can" in l_lower and "speed" in l_lower:
                                            out_f.write(f"  [{line_idx}]: {l[:150]}\n")
                                    out_f.write("\n")
                    except Exception:
                        pass
        except Exception as e:
            out_f.write(f"Error: {e}\n")

print("Done writing matches.")
