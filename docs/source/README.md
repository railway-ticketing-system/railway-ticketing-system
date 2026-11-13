# Specification sources

`requirements.json` contains the 31 identified functional, non-functional and security requirements. `test_cases.json` contains 46 detailed cases with inputs, steps, expected results and exact names of automated tests where available. The source/test name references were checked against the executed XML.

Regenerate UML assets with `python3 scripts/submission/build_diagrams.py`, then run `python3 scripts/submission/build_documents.py` in a virtual environment with the reporting requirements installed. The builder creates the three editable Word documents in `docs/`. Version 2.0 is dated 12 November 2026; change revision metadata deliberately when authoring a subsequent revision.

Convert the Word files to PDF with LibreOffice using an isolated user profile. Render and inspect every page after changing text, tables or diagrams. The final PDFs and editable documents are submission artifacts; page PNGs used for internal layout review are not required submissions. Keep the original baseline in Git rather than overwriting its history.
