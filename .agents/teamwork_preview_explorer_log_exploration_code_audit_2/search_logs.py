import json
import re

files = [
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript_full.jsonl",
    r"C:\Users\thatw\.gemini\antigravity\brain\5cf1497a-ae31-4b1b-9fcd-ac294acc2dd0\.system_generated\logs\transcript.jsonl"
]

keywords = ["speed", "can", "vesc", "familiar", "slave", "voltage"]

output_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\log_search_results.txt"

with open(output_path, "w", encoding="utf-8") as out_f:
    for fpath in files:
        out_f.write(f"\n=== Searching {fpath} ===\n")
        try:
            with open(fpath, "r", encoding="utf-8") as f:
                for line_no, line in enumerate(f, 1):
                    try:
                        data = json.loads(line)
                    except Exception:
                        continue
                    
                    content_str = json.dumps(data)
                    matched = []
                    for kw in keywords:
                        if re.search(r'\b' + re.escape(kw) + r'\b', content_str, re.IGNORECASE):
                            matched.append(kw)
                    
                    if matched:
                        step = data.get("step_index", "N/A")
                        stype = data.get("type", "N/A")
                        source = data.get("source", "N/A")
                        out_f.write(f"Line {line_no} (Step {step}, Type {stype}, Src {source}): Matched {matched}\n")
                        
                        # Print content preview if present
                        content = data.get("content", "")
                        if isinstance(content, str) and content:
                            for cline in content.splitlines():
                                if any(re.search(r'\b' + re.escape(kw) + r'\b', cline, re.IGNORECASE) for kw in keywords):
                                    out_f.write(f"  [content] {cline[:150]}\n")
                        
                        # Print message preview if present
                        msg = data.get("message", "")
                        if isinstance(msg, str) and msg:
                            for mline in msg.splitlines():
                                if any(re.search(r'\b' + re.escape(kw) + r'\b', mline, re.IGNORECASE) for kw in keywords):
                                    out_f.write(f"  [message] {mline[:150]}\n")
                                    
                        # If command execution output, show some stdout
                        cmd = data.get("command", "")
                        if cmd:
                            out_f.write(f"  [command] {cmd}\n")
        except Exception as e:
            out_f.write(f"Error reading {fpath}: {e}\n")

print("Done scanning logs. Results written to:", output_path)
