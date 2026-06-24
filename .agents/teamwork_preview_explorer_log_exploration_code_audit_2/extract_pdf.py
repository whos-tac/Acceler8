import fitz  # PyMuPDF

pdf_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\Resources\FLIPSKY FTESC CAN Protocol V1.4 English.pdf"
out_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\teamwork_preview_explorer_log_exploration_code_audit_2\can_protocol_pdf_text.txt"

doc = fitz.open(pdf_path)
with open(out_path, "w", encoding="utf-8") as f:
    for page_num in range(len(doc)):
        page = doc.load_page(page_num)
        f.write(f"--- PAGE {page_num + 1} ---\n")
        f.write(page.get_text())

print("PDF text extracted successfully.")
