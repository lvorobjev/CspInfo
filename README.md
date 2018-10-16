# CspInfo
The utility for viewing information about installed crypto service providers and key containers

Утилита предназначена для разработчиков криптографического програмнмого обеспечения для отладки своих приложений, использующих функции CryptoAPI. Использование утилиты в других незаннонных целях преследуется по закону.

Возможности:
- просмотр доступных криптопровайлеров;
- получение списка контейнеров ключей для определенного криптопровайдера;
- получение сведений о криптографическом контейнере (идентификатор контейнера, содержащиеся в нем кличи).

Размер исходного кода: 12 КБ

Дата последнего изменения: 17.12.2017

Использование: двойной клик по списку детилизирует исформацию; для возврата не уровень выше выдерите пункт [..].

TODO:
1. обработка нажатия клаыиши ENTER для выбора пункта списка.

Программа не требует установки. Инструкция по сборке:
1. `make`
2. `make run`