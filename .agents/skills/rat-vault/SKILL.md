---
name: rat-vault
description: Maintain rat-engine vault cards, sprint membership, wikilinks, and review evidence. Use when updating project work tracking or knowledge notes.
---

# Maintain the rat-engine vault

Use `AGENTS.md` for the shared execution and status contract. Product knowledge lives
in `vault/`, engineering specifications in `docs/`. Search existing cards first;
copy new cards from `vault/templates/`. Keep scope and provenance from user requests.

Wikilinks resolve filenames, not headings. Use `[[file-slug|Readable title]]`; for
duplicates use the vault-relative path. `production/Task board.base` includes bugs
and tasks; update its Current sprint filter together with sprint activation.
Preserve bug statuses and track review separately instead of using task status enums.

Run `python scripts/check_vault.py` after edits. It checks scalar frontmatter,
required fields, enums, links outside fenced/inline code, sprint membership and board
consistency. Do not mass-correct ambiguous link targets: inspect their intended meaning.
Do not rewrite historical acceptance as if later fixes had already existed.
