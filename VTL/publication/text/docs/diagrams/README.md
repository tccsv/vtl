# Диаграммы модуля `VTL/publication/text` для draw.io

Все диаграммы — в формате **PlantUML** (`.puml`). Это самый удобный способ
держать редактируемые UML-диаграммы рядом с кодом и одновременно открывать их
в draw.io.

## Файлы

| Файл | Что показывает |
| --- | --- |
| `01_text_module_overview.puml` | Общая архитектура модуля: ядро данных, фасад `text_op`, процессоры форматов (markdown / telegram / asciidoc / bbcode), инфраструктура IO |
| `02_processor_architecture.puml` | Унифицированная внутренняя структура одного процессора (одинакова для markdown / telegram / asciidoc): `Marker → MarkerList → ScanContext → Scanners → Parser → Converter / Serializer` + threads-shim |
| `03_data_flow.puml` | Поток данных: `RawText → MarkerList → MarkedText → Output` |

## Как открыть в draw.io

### Способ 1 — вставить как фигуру в существующий лист

1. Открой draw.io (desktop или [app.diagrams.net](https://app.diagrams.net)).
2. **Arrange → Insert → Advanced → PlantUML…**
3. Открой нужный `.puml`, скопируй содержимое в окно (включая `@startuml … @enduml`).
4. Нажми **Insert** — диаграмма отрисуется как редактируемая группа фигур.

### Способ 2 — открыть как отдельную диаграмму

1. **Extras → Edit Diagram…** (Ctrl+E).
2. Перейди на вкладку **PlantUML**.
3. Вставь содержимое `.puml`.
4. **OK** — лист заменится диаграммой.

### Альтернатива — экспорт в PNG/SVG без draw.io

Если нужен только рендер (без редактирования), можно скормить файл
PlantUML-серверу:

```sh
plantuml 01_text_module_overview.puml         # локально, нужен plantuml.jar
# или онлайн: https://www.plantuml.com/plantuml -> вставить текст
```

## Почему PlantUML, а не нативный `.drawio` XML

- draw.io понимает PlantUML «из коробки» и превращает его в редактируемые
  фигуры.
- Нативный `.drawio` XML для class-диаграмм в несколько раз более громоздкий
  и неудобен для правки руками / в diff'ах.
- `.puml` ложится в git как обычный текст и осмысленно диффится.
