with open(r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_3\search_results.txt", "r", encoding="utf-8") as f:
    for i, line in enumerate(f, 1):
        if "familiar" in line.lower() or "slave" in line.lower():
            print(f"Line {i}: {line[:150]}")
