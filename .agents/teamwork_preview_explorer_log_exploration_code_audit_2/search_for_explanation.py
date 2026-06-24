import json
import re

files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
]

output_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\explanation_search.txt"

with open(output_path, "w", encoding="utf-8") as out_f:
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
                    thinking = data.get("thinking", "")
                    
                    # Combine all string text
                    parts = []
                    if isinstance(content, str) and content: parts.append(content)
                    if isinstance(msg, str) and msg: parts.append(msg)
                    if isinstance(thinking, str) and thinking: parts.append(thinking)
                    
                    full_text = "\n".join(parts)
                    
                    # Check if it has speed, voltage and can/vesc, OR 'familiar' or 'slave'
                    lower_text = full_text.lower()
                    
                    # Let's check for specific keywords that would talk about the speed vs voltage issue
                    if ("speed" in lower_text and "voltage" in lower_text and "can" in lower_text) or "familiar" in lower_text or "slave esc" in lower_text:
                        step = data.get("step_index", "N/A")
                        out_f.write(f"\n--- STEP {step} (Line {line_no}, Type {stype}) ---\n")
                        out_f.write(full_text + "\n")
        except Exception as e:
            out_f.write(f"Error reading {fpath}: {e}\n")

print("Done scanning for explanation. Output in explanation_search.txt")
