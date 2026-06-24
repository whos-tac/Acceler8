import sys
import re
sys.stdout.reconfigure(encoding='utf-8')

with open(r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_boot_reliability_2\schematic_text.txt", "r", encoding="utf-8") as f:
    text = f.read()

# Search for word boundary BL word boundary
matches = re.finditer(r'\bBL\b', text, re.IGNORECASE)
for m in matches:
    start = max(0, m.start() - 50)
    end = min(len(text), m.end() + 50)
    print(f"Match at {m.start()}: {text[start:end].strip()}")
