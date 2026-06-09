# VTL Telegram-бот — пульт публикации

Telegram-бот, который управляет публикацией контента средствами проекта VTL.
Сам бот ничего не публикует напрямую: он принимает от пользователя в чате выбор
**платформ** (флаги) и **формата разметки**, а затем вызывает функции
основного проекта — `VTL_PubicateMarkedText` и `VTL_PubicateAudioWithMarkedText`.

Боевой бинарь — [main_firyulin.c](main_firyulin.c). Демонстрационный (без FFmpeg,
с заглушками публикации) — [tools/bot/main_firyulin_demo.c](tools/bot/main_firyulin_demo.c).

---

## Архитектура

```
VTL/bot/
  VTL_bot.{c,h}          — главный цикл (long-polling) + sink ответов
  VTL_bot_data.h         — константы модуля
  api/VTL_bot_api.{c,h}  — обёртка над Telegram Bot API (поверх HTTP-клиента проекта)
  session/VTL_bot_session.{c,h} — состояние диалога: платформы/формат/файл + парсеры
  command/VTL_bot_command.{c,h} — разбор команд и диспетчеризация
  test/VTL_bot_session_test.c   — юнит-тесты логики выбора (без сети)
```

Зависимость от проекта развязана: библиотека `VTL_bot` лёгкая (только parson +
libcurl), а реальные функции публикации прокидываются снаружи через структуру
`VTL_bot_Handlers`. Это позволяет собирать и тестировать бота в изоляции, а в
боевом бинаре подставлять настоящие `VTL_Pubicate*`.

---

## Модули и функции

### `VTL_bot` — главный цикл
[VTL/bot/VTL_bot.h](VTL/bot/VTL_bot.h), [VTL/bot/VTL_bot.c](VTL/bot/VTL_bot.c)

| Функция / тип | Назначение |
|---|---|
| `VTL_bot_Run(token, handlers)` | Запускает long-polling и блокирует поток до Ctrl+C. Создаёт контекст команд и Telegram-sink ответов. |
| `VTL_bot_Handlers` | Структура с указателями на функции публикации проекта (`publish_text`, `publish_audio`). Любой может быть `NULL`. |
| `VTL_bot_PublishTextFn` / `VTL_bot_PublishAudioFn` | Типы указателей, совпадающие по сигнатуре с `VTL_PubicateMarkedText` / `VTL_PubicateAudioWithMarkedText`. |

### `session` — состояние выбора
[VTL/bot/session/VTL_bot_session.h](VTL/bot/session/VTL_bot_session.h)

Хранит для каждого чата выбор: платформы, формат разметки, файл текста
(в памяти, без БД). Парсеры — чистые функции, их и проверяют юнит-тесты.

| Функция | Назначение |
|---|---|
| `VTL_bot_session_TableInit(table)` | Инициализация таблицы сессий. |
| `VTL_bot_session_GetOrCreate(table, chat_id)` | Сессия чата (или новая с дефолтами: `W+TG`, `TelegramMD`, `text.md`). |
| `VTL_bot_session_ParsePlatform(token, *bit)` | Имя платформы (`tg/w/reddit/vimeo/yt/vk`) → бит флага. |
| `VTL_bot_session_ParseFormat(token, *markup)` | Имя формата (`md/telegram/html/bb/asciidoc/mediawiki`) → enum. |
| `VTL_bot_session_FormatName(markup)` | Человекочитаемое имя формата. |
| `VTL_bot_session_DescribePlatforms(flags, out, cap)` | Список выбранных платформ строкой. |

### `command` — обработка команд
[VTL/bot/command/VTL_bot_command.h](VTL/bot/command/VTL_bot_command.h)

| Функция / тип | Назначение |
|---|---|
| `VTL_bot_command_Handle(ctx, chat_id, text)` | Разбирает сообщение, ищет команду в таблице-диспетчере, выполняет. |
| `VTL_bot_Context` | Контекст: таблица сессий, `handlers`, sink ответов (`reply`, `reply_ud`). |
| `VTL_bot_ReplyFn` | Куда уходит ответ: в проде — Telegram, в локальном демо — консоль. |

Перед вызовом публикации команда проверяет, что файл существует
(`VTL_bot_command_FileReadable`) — функции проекта падают на отсутствующем файле.

### `api` — Telegram Bot API
[VTL/bot/api/VTL_bot_api.h](VTL/bot/api/VTL_bot_api.h)

`VTL_bot_api_GetMe`, `VTL_bot_api_DeleteWebhook`, `VTL_bot_api_GetUpdates`,
`VTL_bot_api_SendMessage` — обёртки над методами Bot API поверх HTTP-клиента
проекта (`VTL/utils/curl`).

---

## Команды бота — подробно

У каждого чата своё состояние (сессия). **Дефолт** при первом обращении:
платформы `W + TG`, формат `Telegram MD`, файл `text.md`. Сообщение без `/`
бот не понимает и предлагает `/help`. Неизвестная команда → подсказка про `/help`.

### `/start`, `/help`
Выводит справку: список команд с кратким описанием. Ничего не меняет.

### `/platform <список>`
Задаёт набор платформ для публикации (флаги OR-ятся).
- **Без аргументов** — показывает текущий выбор, список доступных и пример.
- **С аргументами** — разбирает каждый токен (`tg`, `w`/`wiki`, `r`/`reddit`,
  `vimeo`, `yt`/`youtube`, `vk`; регистр не важен). Распознанные — устанавливаются
  (заменяя прежний выбор), нераспознанные перечисляются как пропущенные. Если не
  распознано ни одного — выбор не меняется, возвращается ошибка.
- **Пример:** `/platform tg w reddit` → «Платформы: W, TG, Reddit.»

### `/format <имя>`
Задаёт формат разметки текста (как именно конвертировать перед публикацией).
- **Без аргумента** — показывает текущий формат и список доступных.
- **С аргументом** — `md`/`standart`, `telegram`/`tg`, `html`, `bb`/`bbcode`,
  `adoc`/`asciidoc`, `wiki`/`mediawiki`. Неизвестный — ошибка, формат не меняется.
- **Пример:** `/format asciidoc` → «Формат: AsciiDoc.»

### `/file <путь>`
Задаёт путь к файлу текста, который будет опубликован.
- **Без аргумента** — показывает текущий файл.
- **С аргументом** — сохраняет путь (проверка существования — на этапе `/publish`).
- **Пример:** `/file text.adoc` → «Файл текста: text.adoc.»

### `/status`
Показывает текущие параметры публикации без изменений: файл, платформы, формат.

### `/publish`
Запускает **реальную публикацию текста** — главное действие бота.
Последовательность:
1. Если функция публикации не подключена (`handlers.publish_text == NULL`) —
   сообщает, что недоступно.
2. Проверяет, что выбранный файл существует и читается (иначе «Файл не найден» —
   защита от падения функций проекта на отсутствующем файле).
3. Вызывает `VTL_PubicateMarkedText(файл, флаги_платформ, формат)`; в консоль бота
   пишет строку с параметрами вызова.
4. Возвращает в чат результат: `OK`/`ОШИБКА` и числовой код `VTL_AppResult`, плюс
   напоминание файла/платформ/формата.
- **Пример:** при `text.adoc` + `TG` + `AsciiDoc` → проект конвертирует AsciiDoc и
  отправляет текст в Telegram-плечо; бот отвечает «Публикация текста: OK (код 0)».

### `/publish_audio <аудио>`
Публикует **аудио с текстовой подписью**. Требует имя аудиофайла аргументом.
1. Без аргумента — просит указать аудиофайл.
2. Проверяет подключённость `handlers.publish_audio`, существование аудиофайла и
   файла текста.
3. Вызывает `VTL_PubicateAudioWithMarkedText(аудио, файл_текста, формат, флаги)`.
4. Отвечает результатом (`OK`/`ОШИБКА` + код) и параметрами вызова.
- **Пример:** `/publish_audio audio_ariel.mp3`.

### `/platforms`
Показывает список поддерживаемых имён платформ (`tg, w, reddit, vimeo, yt, vk`).
Справочная команда, выбор не меняет.

### `/formats`
Показывает список поддерживаемых форматов разметки
(`md, telegram, html, bb, asciidoc, mediawiki`). Выбор не меняет.

### `/id`
Возвращает `chat_id` текущего чата (полезно, чтобы узнать значение для
`TG_CHAT_ID`).

### `/ping`
Проверка связи — бот отвечает `🏓 pong`.

---

## Связь с проектом

В боевом бинаре [main_firyulin.c](main_firyulin.c) реальные функции
подставляются в бота:

```c
VTL_bot_Handlers handlers;
handlers.publish_text  = VTL_PubicateMarkedText;
handlers.publish_audio = VTL_PubicateAudioWithMarkedText;
VTL_bot_Run(token, &handlers);
```

Вызываемые функции проекта (см. [VTL/publication/VTL_publication.h](VTL/publication/VTL_publication.h)):

- `VTL_PubicateMarkedText(file, flags, markup)` — конвертация разметки и публикация текста на выбранные платформы;
- `VTL_PubicateAudioWithMarkedText(audio, text, markup, flags)` — публикация аудио с подписью.

**Флаги платформ** (`VTL_CONTENT_PLATFORM_*`): `W`, `TG`, `R` (Reddit),
`VIMEO`, `YT`, `VK`. **Форматы разметки** (`VTL_markup_type_k*`): `StandartMD`,
`TelegramMD`, `HTML`, `BB`, `AsciiDoc`, `MediaWiki`.

---

## Сборка и запуск

```powershell
# Сборка боевого бинаря (в первый раз собирается FFmpeg)
tools\bot\build_bot.cmd

# Запуск: токен и chat_id берутся из tools\bot\secrets.env (не коммитится)
copy tools\bot\secrets.env.example tools\bot\secrets.env   # затем впиши значения
tools\bot\run_bot.cmd
```

`secrets.env`:
```
TG_BOT_TOKEN=<токен от @BotFather>
TG_CHAT_ID=<числовой id чата для публикации>
```

> `text.md` в репозитории нет (он генерируется/локальный) — для `/publish`
> используй существующий вход: `/file text.adoc` + `/format asciidoc`.

**Юнит-тесты** логики выбора (без сети):
```
cmake --build build --target vtl_bot_test
ctest --test-dir build -R VTL_bot_session_test
```
