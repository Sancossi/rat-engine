# Дрёмма — проверка native preview Stride

## Результат

Первый набор доступен как штатные assets в
`games/rat-expedition/Rat.Expedition.Authoring/Assets/CanalCity/`: **97 файлов** —
12 моделей, 12 скелетов, 12 prefab, 45 текстур, 11 материалов, 2 клипа,
демонстрационная сцена, её фон и compositor. Игровые FBX и Blender-исходник
остаются в `Resources/CanalCity`. Native assets имеют стабильные UUID и
относительные ссылки на исходники и общие материалы.

Отдельный `Rat.Expedition.CanalCity.Preview` ссылается на Authoring и выбирает
своим `.sdpkg` корневые assets CanalCity. Он не объявляет общий каталог assets
повторно: импорт владения происходит через ProjectReference. Первая попытка
объявить одну папку в двух package вызвала duplicate-asset ошибку; исправленная
схема успешно собрана. Основная игровая сцена, Core, A1 loader, MCP, Stride checkout,
engine pin и статусы vault этим срезом не изменены.

Проверены реальные Direct3D11-запуски при **1280×720** и **1920×1080**, а также
запуск распакованного self-contained preview вне checkout; все три завершились
с кодом 0 и `status: passed`. Это техническая проверка preview на том же
developer PC, не приёмка A1, MCP reimport, Game Studio GUI, обычной игры или
проверка на чистом компьютере. Независимое ревью native-среза остаётся впереди.

## Проверяемые данные

Метаданные получены реальными `ThreeDAssetImporter.GetEntityInfo` и `Import`
из закреплённого Stride 4.4.0.0 для всех 14 FBX. Сохранены material names,
семантические mesh-node names, иерархия, длительности и SHA256 входов в
`Resources/CanalCity/native-import.json`. Генератор native assets отвергает
метаданные, если FBX изменился. FBX shader texture lookup не используется:
45 native текстур и PBR-графы создаются явно по `materials.json`, включая normal,
metalness, roughness через `Invert: true` и emission с интенсивностью 4.

`MergeMeshes: false` сохраняет семантические узлы. Stride дополнительно разделяет
меши по material slots, поэтому native draw meshes не обязаны совпадать с числом
Blender-объектов. У двери модель и оба клипа используют один скелет; из importer
прочитаны `door_hinge.001`, `door_standard_leaf`, `door_standard_frame`.
Runtime проверяет, что leaf meshes действительно привязаны к leaf node.

- Загружены 12 моделей и 12 prefab, у всех есть mesh/material slots и конечные
  ненулевые bounds. Вершины не заменяются примитивами; отдельный процедурный
  прямоугольный фон относится только к постановке preview.
- Native высоты четырёх фигур: 1,1999998 / 1,7999996 / 2,7999995 / 3,9999993 м.
  Мост 4,9999995 м, набережная 4,499999 м, фонарь 0,7499999 м. Это подтверждает
  преобразование FBX в Stride Y-up и метры без дополнительного масштаба.
- Оба native клипа имеют длительность 2 с. При `TimeFactor=0` и `PlayOnceHold`
  после изменения времени до съёмки проходят 20 кадров. Из actual skeleton
  transforms измерены углы относительно закрытого положения: открытие
  0 / 50,000008 / 99,999987°, закрытие 99,999987 / 50,000008 / 0°.
- Изменяется actual world matrix узла створки; world matrix каменной рамы
  остаётся постоянной. JSON содержит обе матрицы, quaternion петли и время
  каждого семпла, а не только длительность из metadata.
- Неверная ширина CLI (`--width 100`) даёт exit 1 и `error.log`; успешный JSON
  не создаётся. Закрытие приложения до окончания проверки также не считается
  успешным результатом.

Переносимые результаты: [720p](artifacts/2026-09-09-canal-city-native-preview/native-720.json),
[1080p](artifacts/2026-09-09-canal-city-native-preview/native-1080.json),
[распакованный пакет](artifacts/2026-09-09-canal-city-native-preview/native-extracted.json).
Реальные GPU PNG: [обзор](artifacts/2026-09-09-canal-city-native-preview/overview.png),
[закрыто](artifacts/2026-09-09-canal-city-native-preview/door-closed.png),
[середина](artifacts/2026-09-09-canal-city-native-preview/door-middle.png),
[открыто](artifacts/2026-09-09-canal-city-native-preview/door-open.png).
Кадры осмотрены исполнителем и ведущим: исправлена первоначальная камера сзади,
видны лица фигур, архитектура, материалы и открывающаяся створка.

Сохранённый текущий LDR/Gamma pipeline даёт более светлую кладку и насыщенные
жёлтые фонари относительно Cycles. Финальный художественный свет, bloom, тени,
сглаживание и визуальная идентичность Blender этим срезом не квалифицированы.

## Сборка и воспроизведение

Команды из `games/rat-expedition`; для нового прогона выбирается новая папка evidence:

```powershell
dotnet restore Rat.Expedition.CanalCity.Preview/Rat.Expedition.CanalCity.Preview.csproj --locked-mode --configfile NuGet.Config
dotnet restore tools/canal_city/NativeImport/NativeImport.csproj --locked-mode --configfile NuGet.Config
dotnet run --project tools/canal_city/NativeImport/NativeImport.csproj -c Release --no-restore -- Rat.Expedition.Authoring/Resources/CanalCity ../../build/canal-city/new-native-import.json
python tools/canal_city/native_assets.py --import-report ../../build/canal-city/new-native-import.json --authoring Rat.Expedition.Authoring --replace-owned
dotnet build Rat.Expedition.CanalCity.Preview/Rat.Expedition.CanalCity.Preview.csproj -c Release --no-restore
dotnet publish Rat.Expedition.CanalCity.Preview/Rat.Expedition.CanalCity.Preview.csproj -c Release --no-restore --self-contained true -o ../../build/canal-city/new-native-publish
python tools/canal_city/package_preview.py --publish ../../build/canal-city/new-native-publish --zip ../../build/canal-city/new-native-preview.zip
```

Исполняемый файл принимает `--evidence-dir <absolute-path> --width 1920 --height 1080
--smoke-frames 300`. Его рабочая папка — каталог exe. Для автоматической фоновой
проверки используется PowerShell `Start-Process -WindowStyle Hidden -PassThru -Wait`;
необходимо проверить `ExitCode`, `native-preview.json` и PNG. CLI ограничивает
разрешение 640–3840 × 480–2160 и длительность 260–1200 кадров.

Логи этой сессии: `build/canal-city/native-build-02.log` (1031 успешный asset step),
`native-build-03.log` (финальный показ), `native-publish.log`. Ошибок в финальных
сборках нет; предупреждение CS0162 происходит из существующего generated
assetcompiler launcher. Python vault checker и 25 репозиторных тестов прошли.

Частный cache сохранён. Все 142 пакета preview и 127 пакетов NativeImport
совпадают по lock contentHash с исходным Authoring.Windows cohort и локальным
`.nupkg.metadata`; `--locked-mode` проходит. Shared Stride feed, содержащий другие
байты 43 пакетов, не подменял cache. Editor integration pin и runtime package
cohort не выдаются за одну и ту же сборку. Полный Release Game Studio не строился,
поскольку этап остаётся открытым.

## Пакет и границы реестра

Финальный ZIP: `build/canal-city/Rat-CanalCity-NativePreview-notices-win-x64.zip`,
88 214 164 байта; SHA256
`b04f4e44ec925314fdb1215571ee100bb5facf97c07e5ac34a23c136caf11f4a`.
Он распакован в `C:/Users/bogor/AppData/Local/Temp/RatCanalCity-Notices-6vaa8ybt`;
exe оттуда запущен при 1920×1080. Внутри есть compiled `data/db/bundles/default.bundle`,
собственный CLR/hostfxr, `Content/credits.json`, OFL, лицензия проекта, Stride LICENSE
и THIRD PARTY, а также inventory/доступные notices всех 142 locked packages.
FBX, `.blend`, native source scenes и SDK projects отсутствуют.
[Evidence пакета](artifacts/2026-09-09-canal-city-native-preview/package.json) содержит
проверенные пути и хеш; build-manifest честно отмечает сборку из текущей незакоммиченной
рабочей копии, ожидающей независимого ревью.

У 12 записей каталога добавлены реальные `native_preview` paths и
`native_preview_status: validated_isolated_native_preview`; отдельный runtime JSON
лежит в `native-preview-validation.json`. Производственный `native_status` остаётся
`pending_A1_qualification`. Повторная генерация native assets сбрасывает только
preview qualification до `authored_pending_qualification`. Остальные 123 модели
не изготовлены и не помечены готовыми. Реимпорт через MCP, Save/reopen редактора,
production occlusion/traversal, подключение к обычной игре и закрытие A1 не
подменяются созданием этих новых native assets.
