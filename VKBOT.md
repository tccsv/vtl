# VTL VK-бот — пульт публикации в сообществе ВКонтакте

Бот сообщества ВКонтакте, который управляет публикацией средствами проекта VTL.
В диалоге оператор выбирает **площадки** (флаги) и **формат разметки**, а командой
`/post` запускает публикацию — бот вызывает функции проекта
`VTL_PubicateMarkedText` / `VTL_PubicateAudioWithMarkedText`.

Боевой бинарь — [main_schulgin.c](main_schulgin.c). Демонстрационный (без FFmpeg,
с заглушками публикации) — [tools/vkbot/main_schulgin_demo.c](tools/vkbot/main_schulgin_demo.c).

---

## Архитектура

```
VTL/vkbot/
  VTL_vk_bot.{c,h}     — long-poll сообщества + сборка хаба и канала ответа
  VTL_vk_api.{c,h}     — вызовы VK API (https://api.vk.com) + percent-encode
  VTL_vk_dialog.{c,h}  — состояние диалогов и маршрутизация команд (одним модулем)
  test/VTL_vk_dialog_test.c — юнит-тесты разбора/состояния (без сети)
```

Библиотека `VTL_vkbot` лёгкая (parson + libcurl). Функции публикации проекта
прокидываются снаружи через `VTL_vk_Publishers`, поэтому модуль не тащит весь VTL
и тестируется автономно, а боевой бинарь подставляет реальные `VTL_Pubicate*`.

---

## Модули и функции

### `VTL_vk_bot` — обслуживание long-poll
[VTL/vkbot/VTL_vk_bot.h](VTL/vkbot/VTL_vk_bot.h)

| Функция / тип | Назначение |
|---|---|
| `VTL_vk_bot_Serve(token, group_id, pub)` | Поднимает Bots Long Poll сообщества и крутит обработку сообщений до SIGINT. |
| `VTL_vk_Publishers` | Указатели на функции публикации проекта (`text`, `audio`); могут быть NULL. |
| `VTL_vk_PostTextFn` / `VTL_vk_PostAudioFn` | Типы, совпадающие с `VTL_PubicateMarkedText` / `VTL_PubicateAudioWithMarkedText`. |

### `VTL_vk_api` — VK API
[VTL/vkbot/VTL_vk_api.h](VTL/vkbot/VTL_vk_api.h)

| Функция | Назначение |
|---|---|
| `VTL_vk_api_OpenLongPoll(token, group_id, *lp)` | `groups.getLongPollServer` → server/key/ts. |
| `VTL_vk_api_Check(*lp)` | Один `a_check` к long-poll серверу; отдаёт тело ответа (malloc). |
| `VTL_vk_api_Reply(token, peer_id, text)` | `messages.send` (с уникальным random_id). |
| `VTL_vk_api_Escape(in, out, cap)` | Percent-encode текста для query. |

`VTL_vk_LongPoll` хранит координаты сервера (`server`, `key`, `ts`).

### `VTL_vk_dialog` — состояние и маршрутизация
[VTL/vkbot/VTL_vk_dialog.h](VTL/vkbot/VTL_vk_dialog.h)

| Функция / тип | Назначение |
|---|---|
| `VTL_vk_Hub` | Набор диалогов + публикаторы + канал ответа (`emit`). |
| `VTL_vk_Dialog` | Выбор одного peer: площадки, разметка, файл. |
| `VTL_vk_hub_Reset(hub, pub, emit, ud)` | Инициализация хаба. |
| `VTL_vk_hub_Handle(hub, peer_id, text)` | Разбор сообщения и выполнение команды. |
| `VTL_vk_hub_Dialog(hub, peer_id)` | Диалог peer (или новый с дефолтами: VK, Markdown, `text.md`). |
| `VTL_vk_ParseTarget` / `VTL_vk_ParseMarkup` | Имя площадки/формата → флаг/enum (чистые, под тестами). |
| `VTL_vk_MarkupLabel` / `VTL_vk_TargetsLabel` | Человекочитаемые подписи. |

`VTL_vk_EmitFn` — куда уходит ответ: в бою `messages.send`, в демо — консоль.

---

## Команды бота — подробно

У каждого диалога (peer) своё состояние. **Дефолт:** площадки `VK`, разметка
`Markdown`, файл `text.md`. Перед именем команды допускается `/`. Неизвестная
команда → подсказка про `/help`.

### `/help`, `/start`
Справка по командам. Ничего не меняет.

### `/targets <список>`
Выбор площадок (флаги OR-ятся).
- **Без аргументов** — показывает текущий выбор и список доступных.
- **С аргументами** — токены `vk`, `w`/`wiki`, `tg`/`telegram`, `r`/`reddit`,
  `vimeo`, `yt`/`youtube` (регистр не важен). Нераспознанные пропускаются; если
  не распознан ни один — выбор не меняется.
- **Пример:** `/targets vk w` → «Ок, площадки: VK+W».

### `/markup <тип>`
Формат разметки текста.
- **Без аргумента** — показывает текущий формат и список.
- **С аргументом** — `md`/`standart`, `tg`/`telegram`, `html`, `bb`/`bbcode`,
  `adoc`/`asciidoc`, `wiki`/`mediawiki`. Неизвестный — ошибка.
- **Пример:** `/markup asciidoc` → «Разметка: AsciiDoc».

### `/source <файл>`
Путь к файлу текста.
- **Без аргумента** — показывает текущий файл.
- **С аргументом** — сохраняет путь (проверка наличия — на этапе `/post`).
- **Пример:** `/source text.adoc`.

### `/show`
Показывает текущий выбор: файл, площадки, разметка.

### `/post`
Запускает публикацию текста:
1. Проверяет, что публикатор подключён и что файл существует (иначе «Нет файла»).
2. Вызывает `VTL_PubicateMarkedText(файл, флаги, разметка)`; в консоль пишет
   строку `[vk] post text: ...`.
3. Отвечает результатом (`опубликовано`/`ошибка` + код `VTL_AppResult`).

### `/post_audio <файл>`
Публикация аудио с подписью. Требует имя аудиофайла. Проверяет наличие аудио и
файла текста, затем вызывает `VTL_PubicateAudioWithMarkedText(аудио, текст,
разметка, флаги)`.

### `/me`
Возвращает `peer_id` текущего диалога.

### `/ping`
Проверка связи — отвечает `pong`.

---

## Связь с проектом

В [main_schulgin.c](main_schulgin.c) реальные функции подставляются в бота:

```c
VTL_vk_Publishers pub;
pub.text  = VTL_PubicateMarkedText;
pub.audio = VTL_PubicateAudioWithMarkedText;
VTL_vk_bot_Serve(token, group_id, &pub);
```

**Флаги площадок** (`VTL_CONTENT_PLATFORM_*`): `VK`, `W`, `TG`, `R`, `VIMEO`, `YT`.
**Форматы** (`VTL_markup_type_k*`): `StandartMD`, `TelegramMD`, `HTML`, `BB`,
`AsciiDoc`, `MediaWiki`.

> **Важно про доставку.** В текущем проекте `VTL_PubicateMarkedText` реально
> отправляет текст **только в Telegram** (плечо `TG`, нужны `TG_BOT_TOKEN` и
> `TG_CHAT_ID` в окружении). Плечо `W` лишь генерирует файлы локально, а для
> `VK`/`Reddit`/`Vimeo`/`YT` отправителей в проекте пока нет — выбор такой площадки
> вызов проходит, но наружу ничего не уходит (функция возвращает код 0). Бот при
> этом свою задачу выполняет — вызывает функцию публикации проекта с выбранными
> флагами.

---

## Сборка и запуск

```powershell
# Сборка боевого бинаря (в первый раз собирается FFmpeg)
tools\vkbot\build_vkbot.cmd

# Запуск: ключ и id берутся из tools\vkbot\secrets.env (не коммитится)
copy tools\vkbot\secrets.env.example tools\vkbot\secrets.env   # затем впиши значения
tools\vkbot\run_vkbot.cmd
```

`secrets.env`:
```
VK_TOKEN=<ключ доступа сообщества>
VK_GROUP_ID=<числовой id сообщества>
```

**Настройки сообщества ВК:** Управление → Работа с API → **Long Poll API** включить,
версия `5.199`, в типах событий отметить **«Входящее сообщение»** (message_new).
Также включить сообщения сообщества.

> `text.md` в репозитории нет — для `/post` используй существующий вход:
> `/source text.adoc` + `/markup asciidoc`.

**Юнит-тесты** (без сети):
```
cmake --build build --target vtl_vkbot_test
ctest --test-dir build -R VTL_vk_dialog_test
```

---

## Демо без VK и FFmpeg

[tools/vkbot/main_schulgin_demo.c](tools/vkbot/main_schulgin_demo.c) — публикаторы
замокированы. Если заданы `VK_TOKEN`+`VK_GROUP_ID` — реальный long-poll; иначе
локальный разбор команд из stdin (ответы в консоль).

```
cmake --build build --target main_schulgin_demo
app\main_schulgin_demo.exe
```
