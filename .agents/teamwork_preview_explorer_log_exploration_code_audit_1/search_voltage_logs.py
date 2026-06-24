with open(r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\speed_logs.txt", "r", encoding="utf-8") as f:
    text = f.read()

import re
# Let's search for lines containing "voltage" or "display" or "telemetry" or "erpm" or "can"
matches = re.findall(r"(?i).*voltage.*", text)
print(f"Total voltage matches: {len(matches)}")
for m in matches[:20]:
    print(m[:140])
