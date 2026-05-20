# Диаграммы модуля `VTL/publication/text`

В папке два эквивалентных по содержанию набора диаграмм:

| Файл | Формат | Когда использовать |
| --- | --- | --- |
| `VTL_text_module.drawio` | draw.io (native XML, 3 страницы) | открыть двойным кликом в draw.io / app.diagrams.net, редактировать визуально |
| `01_text_module_overview.puml` | PlantUML | редактировать как текст в IDE; рендерить локально (`plantuml`) или вставить в draw.io |
| `02_processor_architecture.puml` | PlantUML | то же самое |
| `03_data_flow.puml` | PlantUML | то же самое |

## Содержание

Оба формата покрывают одни и те же три диаграммы:

1. **Overview** — общая архитектура модуля: ядро данных, фасад `text_op`, процессоры форматов (markdown / telegram / html / asciidoc / bbcode), инфраструктура IO. `asciidoc` помечен как нереализованный в фасаде (в `Init` его пока нет).
2. **Processor template** — унифицированная внутренняя структура одного процессора (одинакова для markdown / telegram / html / asciidoc): `MarkerKind → Marker → MarkerList → ScanContext → Scanners → Parser → Converter / Serializer / ThreadsShim`.
3. **Data flow** — поток данных: `RawText → MarkerList → MarkedText → Output` с разделением на Parse phase (может быть параллельной) и Serialize phase (всегда один поток).

## draw.io

### Открыть `.drawio` напрямую

1. Двойной клик на `VTL_text_module.drawio` (если установлен [draw.io Desktop](https://github.com/jgraph/drawio-desktop/releases)), либо
2. Открыть [app.diagrams.net](https://app.diagrams.net) → **File → Open from → Device** → выбрать файл.

Внизу окна — три вкладки (`1. Overview`, `2. Processor template`, `3. Data flow`), переключаться между ними обычным кликом.

### Вставить `.puml` как шаблон в свой документ

1. **Arrange → Insert → Advanced → PlantUML…**
2. Скопировать содержимое нужного `.puml` (с `@startuml … @enduml`), вставить, **Insert**.

## PlantUML

```sh
plantuml 01_text_module_overview.puml          # локально (нужен plantuml.jar)
# или онлайн: https://www.plantuml.com/plantuml -> вставить текст
```

## Чем отличаются `.drawio` и `.puml`

- `.drawio` — визуальный редактор, удобно для презентаций и финального оформления. Хранит точные координаты, цвета, стили линий. Хуже диффится в git.
- `.puml` — текстовый формат, компактный, отлично диффится, легко править в IDE. draw.io умеет его импортировать, но как одну собранную фигуру, а не как набор индивидуально редактируемых элементов.

Оба формата поддерживаются — используй то, что удобнее для текущей задачи.
