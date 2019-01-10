

-- Платформа - stream-platform
Хранение списка название менеджера / ключ менеджера
Проверка ключа менеджера
Получение клиента по url - пример: stream-cloud.ru/pymba86/cottage
Управление главными пользователями
Перенаправление команд с передачей id клиента

-- Менеджер - stream-manager
Менеджер отправляет список клиентов и ключ менеджера, кому должен уйти ответ (
                                        - платформа будет искать каждого клиента производительность
                                        + быстро делать)

Pub/ sub на параметры устройства - stream-control -> stream-sync
Pub/ sub на историю - stream-history -> stream-sync

-- Устройство - stream-device

TODO Удалить папки stream-groups stream-auth stream-users
TODO Переменовать stream-devices - stream-device