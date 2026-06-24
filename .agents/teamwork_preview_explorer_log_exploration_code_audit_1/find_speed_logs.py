import json

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\speed_logs.txt"

with open(out_path, "w", encoding="utf-8") as out_f:
    for lf in log_files:
        out_f.write(f"=== {lf} ===\n")
        try:
            with open(lf, "r", encoding="utf-8") as f:
                for i, line in enumerate(f, 1):
                    try:
                        obj = json.loads(line)
                        content = obj.get("content", "")
                        thinking = obj.get("thinking", "")
                        
                        # We want to find references to "speed" and "can" or "slave" or "display"
                        # and particularly look for "familiar" or "root cause"
                        text = f"{content} {thinking}"
                        text_l = text.lower()
                        if "speed" in text_l and "can" in text_l:
                            if "display" in text_l or "telemetry" in text_l or "issue" in text_l or "voltage" in text_l:
                                out_f.write(f"Line {i}:\n")
                                if content:
                                    out_f.write(f"  CONTENT:\n{content}\n")
                                if thinking:
                                    out_f.write(f"  THINKING:\n{thinking}\n")
                                out_f.write("-" * 40 + "\n")
                    except Exception:
                        pass
        except Exception as e:
            out_f.write(f"Error: {e}\n")

print("Done writing speed logs.")
