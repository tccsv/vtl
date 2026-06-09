ПЕРЕНОС ИЗМЕНЕНИЙ VTL (Telegram- и VK-боты)
===========================================

1. Распакуй содержимое архива в КОРЕНЬ проекта VTL, заменяя файлы
   (структура папок совпадает: VTL/, tools/, cmake/, корневые файлы).

2. Если в целевом проекте остался старый бот "задачи/напоминания" - удали:
       VTL/bot/store/
       VTL/bot/reminder/
       VTL/bot/test/VTL_bot_store_test.c
   (в новой версии вместо них VTL/bot/session/ и VTL_bot_session_test.c)

3. Секреты НЕ входят в архив. Создай их из шаблонов и впиши свои значения:
       copy tools\bot\secrets.env.example   tools\bot\secrets.env
       copy tools\vkbot\secrets.env.example tools\vkbot\secrets.env

4. Сборка и запуск:
       Telegram:  tools\bot\build_bot.cmd     -> tools\bot\run_bot.cmd
       VK:        tools\vkbot\build_vkbot.cmd  -> tools\vkbot\run_vkbot.cmd

Описание функционала: BOT.md (Telegram), VKBOT.md (VK).