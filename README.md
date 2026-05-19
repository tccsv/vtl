# VTL — Video/Text/Audio publication Library

Библиотека на C11 для автоматической публикации медиаконтента (текст, аудио, видео, изображения) в Telegram, Reddit, Vimeo и другие платформы.

## Возможности

- Текст: Telegram MarkdownV2, HTML, BBCode → отправка в Telegram
- Аудио: чтение, перекодирование через FFmpeg, отправка с подписью
- Видео и субтитры: SRT парсинг, наложение, конвертация
- Изображения: фильтры и утилиты через FFmpeg
- Vimeo: TUS resumable upload видео, метаданные, приватность, плеер, домены встраивания, обложки, главы
- Reddit API, история публикаций в PostgreSQL

## Сборка

**Нужен только компилятор и CMake** — никаких глобальных `apt install ffmpeg` / `brew install ffmpeg` / `vcpkg install` / Python / meson / perl.

- **Linux / macOS** — внешние либы (FFmpeg, libcurl, libpq) лежат как pre-built `.so` / `.dylib` в `external_libs/`
- **Windows** — всё собирается из исходников в `external_sources/` через MSVC (FFmpeg, curl, libpq + NASM/VSNASM портативно в `external_sources/tools/`)

### Linux

```bash
sudo apt install build-essential cmake
cmake -S . -B build
cmake --build build -j
./app/VTL
```

### macOS

```bash
xcode-select --install
brew install cmake
cmake -S . -B build
cmake --build build -j
./app/VTL
```

### Windows

Нативно через MSVC (Visual Studio 2022/2026). MinGW/MSYS2/WSL/Python не нужны.

```powershell
cmake -S . -B build-source -A x64
cmake --build build-source --config Release -j
.\app\VTL.exe
```

Первая сборка идёт долго (~10–15 минут): собирается FFmpeg 7.1 (ShiftMediaProject) + libpq + curl. Последующие сборки инкрементальные.

## Запуск

В корне проекта должны лежать input-файлы (в репо не закоммичены, положить вручную):

- `text.md` — текст публикации
- `audio_ariel.mp3`, `audio_styuardessa.mp3`, `audio_xanadu.mp3` — аудиофайлы

Переменные окружения:

- `TG_BOT_TOKEN`, `TG_CHAT_ID` — для Telegram (без них `Text Pipeline` / `Audio Pipeline` упадут на старте)
- `VIMEO_ACCESS_TOKEN` — для Vimeo (нужен только если используешь vimeo-функции; без него вернётся `VTL_res_vimeo_kMissingToken`)

### Linux / macOS

```bash
export TG_BOT_TOKEN="<токен от @BotFather>"
export TG_CHAT_ID="<id из getUpdates>"
export VIMEO_ACCESS_TOKEN="<personal access token>"
./app/VTL
```

### Windows (PowerShell)

```powershell
$env:TG_BOT_TOKEN       = "<токен от @BotFather>"
$env:TG_CHAT_ID         = "<id из getUpdates>"
$env:VIMEO_ACCESS_TOKEN = "<personal access token>"
.\app\VTL.exe
```

### Windows (cmd.exe)

```cmd
set TG_BOT_TOKEN=<токен от @BotFather>
set TG_CHAT_ID=<id из getUpdates>
set VIMEO_ACCESS_TOKEN=<personal access token>
app\VTL.exe
```

Получить значения:
- `TG_BOT_TOKEN` — у [@BotFather](https://t.me/BotFather), команда `/newbot`
- `TG_CHAT_ID` — отправить сообщение боту, дёрнуть `https://api.telegram.org/bot<TOKEN>/getUpdates`, скопировать `chat.id` из ответа
- `VIMEO_ACCESS_TOKEN` — [Vimeo Developer](https://developer.vimeo.com/apps) → создать app → Personal Access Token со скоупами `upload`, `edit`, `private`, `video_files`

Без переменных всё что **до** публикации (AsciiDoc-парсер, бенчмарки параллелизма) отработает и покажет вывод — это удобно для проверки сборки.

## Структура проекта

```
VTL/
  publication/        — пайплайн публикации (текст, аудио)
  content_platform/   — Telegram, Reddit, Vimeo
  media_container/    — аудио, видео, субтитры, изображения
  user/               — история публикаций
  utils/              — файлы, строки, шифрование, HTTP, БД
external_libs/                    — pre-built бинари для Linux/macOS
  linux/                          — .so для Linux
  macos/                          — .dylib для macOS
external_sources/                 — исходники для source-build (Windows)
  parson/                         — JSON-парсер (используется везде)
  curl/                           — curl 8.x (Schannel TLS)
  libpq/                          — postgresql 18.3 (libpq client)
  ffmpeg/                         — FFmpeg 7.1 (ShiftMediaProject build)
  zlib/                           — zlib (для опциональных FFmpeg кодеков)
  tools/                          — NASM + VSNASM (для FFmpeg .asm)
cmake/
  Dependencies-{Windows,MacOS}.cmake — диспетчер зависимостей по платформам
  libpq/                          — собственный CMakeLists.txt для libpq под MSVC
    generated/                    — pre-generated headers (kwlist_d.h, pg_config*.h, ...)
  scripts/                        — bat/ps1 для FFmpeg source build
msvc/                             — артефакты FFmpeg-сборки (генерится локально, не в git)
```

## Интегрированные модули

| Ветка | Автор | Модуль |
|---|---|---|
| yarovitchuk | Яровитчук | Аудио (чтение, запись, перекодирование) |
| bulachev | Булачев | Enum-ы, исправления опечаток |
| lachugin | Лачугин | BBCode формат текста |
| ivannikov | Иванников | Telegram Bot API |
| ishatalov | Ишаталов | БД (PostgreSQL) |
| fedorov-subtitles-macOS | Федоров | Субтитры (SRT парсинг, наложение, конвертация, стили) |
| borisenkov-reddit | Борисенков | Reddit API, HTTP-клиент |
| zmaev-vimeo | Змаев | Vimeo (TUS upload, метаданные, приватность, плеер, обложки, главы) |

## Авторы

Команда проекта VTL.
