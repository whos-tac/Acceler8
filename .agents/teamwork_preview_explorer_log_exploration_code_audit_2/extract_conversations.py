import json

files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
]

out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\extracted_context.txt"

with open(out_path, "w", encoding="utf-8") as out_f:
    for fpath in files:
        out_f.write(f"\n==================== {fpath} ====================\n")
        try:
            with open(fpath, "r", encoding="utf-8") as f:
                for line_no, line in enumerate(f, 1):
                    try:
                        data = json.loads(line)
                    except Exception:
                        continue
                    
                    stype = data.get("type", "")
                    content = data.get("content", "")
                    msg = data.get("message", "")
                    
                    # We are looking for speed issue details, CAN telemetry, VESC CAN messages, "familiar CAN issue", etc.
                    # Let's search inside content or msg
                    full_text = ""
                    if isinstance(content, str):
                        full_text += content
                    if isinstance(msg, str):
                        full_text += msg
                        
                    lower_text = full_text.lower()
                    if "familiar" in lower_text or "slave" in lower_text or "vesc" in lower_text or "speed display" in lower_text or "telemetry" in lower_text:
                        step = data.get("step_index", "N/A")
                        out_f.write(f"\n--- STEP {step} (Line {line_no}, Type {stype}) ---\n")
                        out_f.write(full_text + "\n")
        except Exception as e:
            out_f.write(f"Error reading {fpath}: {e}\n")

print("Done extracting conversation blocks to:", out_path)
