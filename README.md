# Транспортный справочник, часть D

В этой версии необходимо перевести весь ввод и вывод на формат JSON. Детали — в примере ниже.

Гарантируется, что вещественные числа не задаются в экспоненциальной записи, то есть обязательно имеют целую часть и, возможно, дробную часть через десятичную точку.

Каждый запрос к базе дополнительно получает идентификатор в поле id — целое число от 0 до 2147483647. Ответ на запрос должен содержать идентификатор этого запроса в поле request_id. Это позволяет выводить ответы на запросы в любом порядке.

Ключи словарей могут располагаться в произвольном порядке. Форматирование (то есть пробельные символы вокруг скобок, запятых и двоеточий) не имеет значения как во входном, так и в выходном JSON.

**Stop X (изменения запроса на добавление остановки)**

Новый формат запроса на добавление остановки: **Stop X: latitude, longitude, D1m to stop1, D2m to stop2, ...**

А именно, теперь после широты и долготы может содержаться список расстояний от этой остановки до соседних. По умолчанию предполагается, что расстояние от X до stop# равно расстоянию от stop# до X, если только расстояние от stop# до X не задано явным образом при добавлении остановки stop#

Гарантируется, что каждая из остановок stop# определена в некотором запросе Stop.

В рамках одного запроса Stop все stop# различны, и их количество не превосходит 100.

Все Di являются целыми положительными числами, каждое из которых не превышает 1000000 и не меньше расстояния по прямой между соответствующими остановками. После каждого расстояния обязательно следует буква m.

**Bus X (изменения запроса на информацию об автобусе)**

Новый формат ответа на запрос информации об автобусе: Bus X: R stops on route, U unique stops, L route length, C curvature

* L теперь вычисляется с использованием дорожного расстояния, а не географических координат.

* C (извилистость) — отношение длины маршрута, вычисленной с помощью дорожного расстояния, (новое L) к длине маршрута, вычисленной с помощью географического расстояния (L из предыдущих версий задачи). Таким образом, C — вещественное число, большее единицы. Оно может быть равно единице только в том случае, если автобус едет между остановками по кратчайшему пути (и тогда больше похож на вертолёт), а меньше единицы может оказаться только благодаря телепортации или хордовым тоннелям.

Гарантируется, что для любых двух соседних остановок любого маршрута так или иначе задано расстояние по дорогам.  

**Пример:**

#### Ввод

    {
        "base_requests": [
            {
            "type": "Stop",
            "road_distances": {
                "Marushkino": 3900
            },
            "longitude": 37.20829,
            "name": "Tolstopaltsevo",
            "latitude": 55.611087
            },
            {
            "type": "Stop",
            "road_distances": {
                "Rasskazovka": 9900
            },
            "longitude": 37.209755,
            "name": "Marushkino",
            "latitude": 55.595884
            },
            {
            "type": "Bus",
            "name": "256",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Biryusinka",
                "Universam",
                "Biryulyovo Tovarnaya",
                "Biryulyovo Passazhirskaya",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
            },
            {
            "type": "Bus",
            "name": "750",
            "stops": [
                "Tolstopaltsevo",
                "Marushkino",
                "Rasskazovka"
            ],
            "is_roundtrip": false
            },
            {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.333324,
            "name": "Rasskazovka",
            "latitude": 55.632761
            },
            {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 7500,
                "Biryusinka": 1800,
                "Universam": 2400
            },
            "longitude": 37.6517,
            "name": "Biryulyovo Zapadnoye",
            "latitude": 55.574371
            },
            {
            "type": "Stop",
            "road_distances": {
                "Universam": 750
            },
            "longitude": 37.64839,
            "name": "Biryusinka",
            "latitude": 55.581065
            },
            {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 5600,
                "Biryulyovo Tovarnaya": 900
            },
            "longitude": 37.645687,
            "name": "Universam",
            "latitude": 55.587655
            },
            {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Passazhirskaya": 1300
            },
            "longitude": 37.653656,
            "name": "Biryulyovo Tovarnaya",
            "latitude": 55.592028
            },
            {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Zapadnoye": 1200
            },
            "longitude": 37.659164,
            "name": "Biryulyovo Passazhirskaya",
            "latitude": 55.580999
            },
            {
            "type": "Bus",
            "name": "828",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Universam",
                "Rossoshanskaya ulitsa",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
            },
            {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.605757,
            "name": "Rossoshanskaya ulitsa",
            "latitude": 55.595579
            },
            {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.603831,
            "name": "Prazhskaya",
            "latitude": 55.611678
            }
        ],
        "stat_requests": [
            {
            "type": "Bus",
            "name": "256",
            "id": 1965312327
            },
            {
            "type": "Bus",
            "name": "750",
            "id": 519139350
            },
            {
            "type": "Bus",
            "name": "751",
            "id": 194217464
            },
            {
            "type": "Stop",
            "name": "Samara",
            "id": 746888088
            },
            {
            "type": "Stop",
            "name": "Prazhskaya",
            "id": 65100610
            },
            {
            "type": "Stop",
            "name": "Biryulyovo Zapadnoye",
            "id": 1042838872
            }
        ]
    }

#### Вывод

    [
        {
            "route_length": 5950,
            "request_id": 1965312327,
            "curvature": 1.36124,
            "stop_count": 6,
            "unique_stop_count": 5
        },
        {
            "route_length": 27600,
            "request_id": 519139350,
            "curvature": 1.31808,
            "stop_count": 5,
            "unique_stop_count": 3
        },
        {
            "request_id": 194217464,
            "error_message": "not found"
        },
        {
            "request_id": 746888088,
            "error_message": "not found"
        },
        {
            "buses": [],
            "request_id": 65100610
        },
        {
            "buses": [
            "256",
            "828"
            ],
            "request_id": 1042838872
        }
    ]

