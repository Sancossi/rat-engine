---
type: adr
area: Engine
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-81fa-9aa9-fc12ef2575a1
---

# ADR-001 Language and runtime

## Context

Нужен явный выбор языка/рантайма и граф. API для rat-engine + Rat Project.

## Decision

Используем **текущий стек** (зафиксирован реализацией bootstrap):

- **Language:** C++20
- **Build:** CMake 3.24+ + FetchContent
- **Render:** bgfx (Windows MVP: Direct3D11)
- **Window / input:** GLFW (`GLFW_NO_API` → native HWND)
- **Editor UI:** Dear ImGui (docking)
- **Platform MVP:** Windows x64

## Consequences

- Tooling = MSVC/Clang-cl + CMake; без Qt.
- Game/event data остаётся data-driven (JSON), не привязана к скриптовому runtime.
- Смена языка (Rust/C#) = новый ADR + миграция; не планируется в MVP.
