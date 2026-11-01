from pathlib import Path
from docx import Document

root = Path(__file__).resolve().parent.parent
source_path = root / "Documents" / "RequirementsAndArchitecture.docx"
plan_path = Path(r"C:\Users\Anders Sandberg\.copilot\plans\plan-add-jpg-source-support-to-aiml-imageextractor.md")
output_path = root / "Documents" / "AIML_ImageExtractor.docx"

doc = Document(source_path)
guide = [
    ("Heading 1", "User Guide"),
    ("Heading 2", "Open a JPG/JPEG still image"),
    ("Normal", "Choose File > Open and select a .jpg or .jpeg file. The image opens as a single frame, and the Properties pane identifies it as JPEG and shows one frame."),
    ("Heading 2", "Extract and label an image"),
    ("Normal", "Drag a rectangle around the bird and release the mouse button, then choose a species or Unknown. The cropped PNG is stored in speciesImages.db with the source file path and frame 0. Image > Detect birds (D) and Image > Auto extract detected birds (A) can also be used on the still image."),
]
plan_items = []
for line in plan_path.read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    if line.startswith("#"):
        level = min(len(line) - len(line.lstrip("#")), 3)
        plan_items.append((f"Heading {level}", line.lstrip("# ").strip()))
    elif line.startswith("- "):
        plan_items.append(("List Paragraph", line[2:]))
    else:
        plan_items.append(("Normal", line))

nodes = [doc.add_paragraph(text, style=style)._p for style, text in guide]
nodes.extend([doc.add_paragraph("JPG/JPEG Capability Plan", style="Heading 1")._p])
nodes.extend(doc.add_paragraph(text, style=style)._p for style, text in plan_items)
body = doc._element.body
for node in reversed(nodes):
    body.insert(0, node)
doc.save(output_path)
print(f"Created {output_path} from {source_path}; included {len(plan_items)} plan paragraphs and the new User Guide.")
