---
type: adr
area: Engine
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-8198-9d54-d04bbc851461
---

# ADR-008 Parallel and Autorun runtime limits

## Context

Parallel events легко убивают кадр и отладку, если без лимитов.

## Decision (MVP)

- Max **8** active Parallel per map
- Max **32** Parallel commands executed per frame (sum)
- Max **1** Autorun at a time (blocks player control until done)
- **No nested** Parallel started from Parallel in v1
- No busy-loop without Wait/yield
- On exceed: skip + warn/log (visible in Edit)

## Consequences

- Runtime must count budget per frame.
- Authors design with Wait; heavy logic split across frames.
- Limits tunable later via config if content needs more.
