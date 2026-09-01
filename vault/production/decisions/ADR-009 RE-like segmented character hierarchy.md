---
type: adr
area: Game
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-8127-8460-cc23b51812ac
---

# ADR-009 RE-like segmented character hierarchy

## Context

Нужна структура персонажа под stylized 3D pixel и процедурную анимацию.

## Decision

Иерархия сегментов в духе классического Resident Evil:

голова; грудь; живот; руки из нескольких сегментов; ноги сегментами.

Анимация MVP — процедурная на этой иерархии (не flipbook-only).

## Consequences

- Asset pipeline: отдельные mesh-части + parent transforms.
- Animation code работает с сегментами, не только с одним skinned mesh.
- Можно позже добавить skinning внутри сегмента, не ломая иерархию.
