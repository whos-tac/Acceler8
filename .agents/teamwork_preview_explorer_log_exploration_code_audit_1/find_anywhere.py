import os

search_dir = r"c:\Users\thatw\Documents\Apollo-8\DashBoard"
target_words = ["familiar", "can issue", "can telemetry", "speed display", "slave esc"]

for root, dirs, files in os.walk(search_dir):
    for f in files:
        if f.endswith(".md") or f.endswith(".txt") or f.endswith(".cpp") or f.endswith(".h"):
            fp = os.path.join(root, f)
            try:
                with open(fp, "r", encoding="utf-8", errors="ignore") as file:
                    content = file.read()
                    content_lower = content.lower()
                    for word in target_words:
                        if word in content_lower:
                            print(f"File {fp} contains '{word}'")
            except Exception as e:
                pass
