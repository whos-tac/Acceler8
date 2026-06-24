with open(r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_1\speed_logs.txt", "r", encoding="utf-8") as f:
    text = f.read()
    
# Let's search for "familiar" or similar
import re
matches = re.findall(r"(?i)(?:familiar|issue|bug|problem|reason|cause|wrong|fix|slave|master).{0,100}speed", text)
for m in matches[:30]:
    print(m)
