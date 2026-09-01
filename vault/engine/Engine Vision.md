---
type: note
tags: [engine]
---

# Engine Vision

## Цели

- Дать runtime для **3D pixel ortho** игр с **RPG Maker-like events**.
- Поддержать **edit-in-playmode** (карта + события) в одном процессе с игрой.
- Предсказуемый API сцены, ввода, ассетов и цикла кадра (bgfx).

## Non-goals

- Не клонировать Unity/Unreal editor целиком.
- Не тащить turn-based battle stack в MVP.
- Не блокировать игру ожиданием «идеального» оффлайн-редактора.

## Целевые платформы

- Primary: Windows desktop.
- Later: по ADR.

## Принципы API

- Явные владения ресурсами; минимум скрытого глобального состояния.
- Gameplay data-driven (events), код движка — стабильный runtime.
- Ошибки видимы в debug; логируемые failures в release.

## Product constraints (из [[GDD]])

- Camera: ortho 3/4 + pixel-stable present — [[ADR-003 Ortho pixel-stable camera]]
- Map: hybrid free move + grid snap; events on tile & volume — [[ADR-004 Hybrid map movement and events]]
- Conflict: exploration + light field action — [[ADR-005 Conflict model exploration plus field action]]

## Метрики успеха движка

- [ ] Пустой проект стартует быстро
- [ ] Play ↔ Edit без перезапуска процесса
- [ ] Event v1 прогоняет vertical slice квест
- [ ] Нет заметного shimmer на ortho pixel upscale
