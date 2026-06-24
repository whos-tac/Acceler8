import os

search_dir = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\src"
for root, dirs, files in os.walk(search_dir):
    for f in files:
        if f.endswith(".cpp") or f.endswith(".h"):
            fp = os.path.join(root, f)
            with open(fp, "r", encoding="utf-8", errors="ignore") as file:
                for line_no, line in enumerate(file, 1):
                    if "erpm" in line.lower() or "speed_kmh" in line.lower():
                        if "g_vehicle_state" in line:
                            print(f"{fp}:{line_no}: {line.strip()}")
