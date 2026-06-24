import json
import re

log_files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl"
]

search_terms = ["speed", "can", "vesc", "familiar", "slave", "voltage", "telemetry"]

out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\search_results.txt"

with open(out_path, "w", encoding="utf-8") as out_f:
    for lf in log_files:
        out_f.write(f"==================================================\n")
        out_f.write(f"FILE: {lf}\n")
        out_f.write(f"==================================================\n")
        try:
            with open(lf, "r", encoding="utf-8") as f:
                for i, line in enumerate(f, 1):
                    try:
                        obj = json.loads(line)
                        content = obj.get("content", "")
                        thinking = obj.get("thinking", "")
                        output = obj.get("output", "")
                        
                        all_text = f"{content}\n{thinking}\n{output}"
                        # Check if any combination of keywords is in the text
                        # Let's search for "speed", "familiar", "telemetry", "vesc", "slave"
                        match_reasons = []
                        if "familiar" in all_text.lower():
                            match_reasons.append("familiar")
                        if "slave" in all_text.lower():
                            match_reasons.append("slave")
                        if "vesc" in all_text.lower() and "speed" in all_text.lower():
                            match_reasons.append("vesc + speed")
                        if "can" in all_text.lower() and "speed" in all_text.lower() and "voltage" in all_text.lower():
                            match_reasons.append("can + speed + voltage")
                            
                        if match_reasons:
                            out_f.write(f"Line {i} matched reasons: {match_reasons}\n")
                            # Write snippets
                            for text_type, txt in [("content", content), ("thinking", thinking), ("output", output)]:
                                if txt:
                                    out_f.write(f"  --- {text_type.upper()} ---\n")
                                    lines = txt.split('\n')
                                    for line_idx, l in enumerate(lines):
                                        l_lower = l.lower()
                                        if any(term in l_lower for term in ["speed", "vesc", "slave", "familiar", "telemetry", "voltage"]):
                                            out_f.write(f"    [{line_idx}]: {l[:160]}\n")
                            out_f.write("\n")
                    except Exception as e:
                        pass
        except Exception as e:
            out_f.write(f"Error reading file: {e}\n")

print("Done writing search results.")
