# Снимок редактора, игры и vault после P1.1a

Пользователь 2026-09-09 разрешил опубликовать редактор, игру и vault целиком после текущего этапа. [Карточка публикации](../../vault/production/tasks/publish-editor-game-vault-snapshot.md) ведётся родительским агентом. Целевая ветка — `publish/stride-p11-editor-game-vault`, origin — `https://github.com/sancossi/rat-engine`. Подготовка снимка не закрывает полный P1/Sprint 19 и не начинает P1.2.

## Состав и происхождение

- Текущая Stride/C# игра уже зафиксирована: runtime `bbd7f3d`, verification harness `4c6bab3`, [камера/проверенный ZIP](2026-09-09-stride-camera-framing.md). README теперь направляет к игре, закреплённой исходной основе и vault. Чистый Stride checkout с upstream `e2c786a45f69917bf233793f6a097b150e2fe264` остаётся отдельно; код движка не копируется и собственный fork/patch не создаётся.
- `af7041d` сохраняет ранее оставленную локальную C++ реализацию: `apps/game`, перенос общих helpers в `apps/platform`, совместимые editor headers, sprite renderer/shaders, CMake/game-release, `src/game/expedition`, headless regressions, JSON-сцену и оригинальный PNG с исходником/credits. Выбраны 45 явных путей; Git показывает 43 изменения с учётом двух переименований. Runtime-код при подготовке публикации не редактировался. Исторические README/evidence получили пояснение и ссылки на точные архивные контракты `-rat-engine-2026-09-08`; прежняя приёмка не переносится на Stride.
- Сохраняются существующий ladder follow-up на `feat-mgs3-ladder-climb` и локальное правило Cursor о сборке редактора после закрытия этапа. 45 vault/Obsidian путей оказались идентичны index после Git-нормализации переносов строк: обновлена только информация index, содержательных изменений этих заметок/настроек нет. Статусы публикации/P1/Sprint 19 остаются у родительского агента.

Перед staging просмотрены 18 содержательно изменённых tracked путей и 29 untracked файлов, включая удалённые старые пути helpers. Проверка имён/размеров и характерных private-key/token/credential patterns не обнаружила кандидатов на секреты, новые executable/архивы или caches. Это ограниченная проверка состава снимка. Наибольший исходный кандидат — 22 348 bytes. Единственный добавленный бинарный asset — оригинальный PNG, 1 374 bytes, SHA256 `09fdb9babe34019e773d3db86fd5585ccb1cf0240d5b71f6649ab9a1d8b8d75b`; in-memory генерация из `assets/expedition/rat_sprite.py` и текущая Stride-копия дают те же байты.

`build/`, dependency caches, local game `.packages`, публикационные ZIP и GPU captures остаются вне Git; `git check-ignore` подтвердил это для игрового executable и NuGet package. Снимок включает исходники/контент/документы, а не загруженные зависимости или готовые builds.

## Проверки

Scoped staged `git diff --check` и `python scripts/check_vault.py` прошли. Родительский агент отдельно выполнил:

- Полный C++ Release `scripts/verify.ps1`: exit 0, 24 Python tests, 720 CTest passed и один symlink skip из 721. Лог `C:/Users/bogor/AppData/Local/Temp/rat-publication-release-verify.log`; editor `C:/5_gamedev/rat-engine/build/dev-release/apps/editor/rat-editor.exe`.
- Stride GameStudio Release: exit 0, 60.45 с, пять upstream NU5100 warnings, ноль ошибок. Логи `C:/5_gamedev/stride/logs/rat-foundation/20260909-032914-138/` и `C:/Users/bogor/AppData/Local/Temp/rat-publication-stride-editor.log`; editor `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.

Последняя игровая проверка относится к неизменённому ZIP P1.1a: 12 Core и 10 реальных executable сценариев, включая GPU 720p/1080p, края/zoom/focus и ожидаемые отказы контента; точные commits/artifacts/границы — в [отчёте камеры](2026-09-09-stride-camera-framing.md). Публикация не добавляет заявления о ручном GUI плейтесте или новой remote CI проверке. Текущий `.github` workflow проверяет C++/vault; Stride game CI здесь не добавлен.

## Передача на публикацию

Независимое read-only ревью состава и фактический push/remote SHA записывает родительский агент после готовности снимка. Доступ к origin подтверждён его `git push --dry-run`. Remote main и локальная история расходятся; отдельная новая ветка сохраняет обе истории без merge, force push или переписывания main. Исполнитель подготовил локальные commits и не выполнял push/PR.
