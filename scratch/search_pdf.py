import pypdf
import re

pdf_path = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\Resources\FLIPSKY FTESC UART Protocol V1.6 English.pdf"

reader = pypdf.PdfReader(pdf_path)
print(f"Total pages: {len(reader.pages)}")

# Print all text from all pages to look for details
full_text = ""
for idx, page in enumerate(reader.pages):
    text = page.extract_text()
    full_text += f"\n--- PAGE {idx+1} ---\n" + text

# Let's write the full extracted text to a text file for our viewing!
output_txt = r"c:\Users\thatw\Documents\Apollo-8\DashBoard\scratch\pdf_extracted.txt"
with open(output_txt, "w", encoding="utf-8") as f:
    f.write(full_text)

print(f"Extracted all text to {output_txt}")

# Let's search for keywords: 'keep', 'live', 'heart', '25', '19' (command 25 is 0x19 in hex)
keywords = ["keep", "live", "heart", "19H", "25", "fail", "safe"]
for kw in keywords:
    matches = []
    for idx, page in enumerate(reader.pages):
        text = page.extract_text()
        if re.search(kw, text, re.IGNORECASE):
            matches.append(idx + 1)
    print(f"Keyword '{kw}' matches pages: {matches}")
