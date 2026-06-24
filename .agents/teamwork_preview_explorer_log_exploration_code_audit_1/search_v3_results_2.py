with open(r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\search_results.txt", "r", encoding="utf-8") as f:
    text = f.read()

import re
# Let's search for "speed" and "can" or "slave" or "voltage"
matches = re.findall(r"(?i).*speed.*voltage.*", text)
print(f"Matches in Explorer 3 results: {len(matches)}")
for m in matches[:20]:
    print(m[:140])
