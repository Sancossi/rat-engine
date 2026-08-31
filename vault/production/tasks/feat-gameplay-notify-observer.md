---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Gameplay notify observer

Intent: тонкая шина сигналов (GPP Observer) для UI/аудио: item picked, dialog shown, landed — без подмены [[Event System]]. Брать когда [[feat: Audio play-queue stub]] уже есть и геймплей иначе тянет `Audio*` вглубь.

Acceptance: подписка/пост без синглтона; RM-команды не ходят через шину; тесты без окна.
