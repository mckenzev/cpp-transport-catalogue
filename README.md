# Transport Catalogue

C++ проект для обработки транспортных маршрутов, построения карт и поиска оптимальных маршрутов

Этот проект — учебная реализация транспортного каталога: системы, которая принимает JSON-запросы, строит базу данных транспортной сети (остановки, автобусы, расстояния), визуализирует карту в формате SVG, а также ищет оптимальные маршруты с учётом времени ожидания и скорости движения.

---

## Возможности проекта

### 1. Построение транспортного каталога

* Хранение всех остановок с координатами
* Хранение автобусных маршрутов
* Учет дорожных расстояний между остановками
* Подсчёт статистики маршрутов: длина, кривизна, количество уникальных остановок

### 2. Генерация карты в SVG

Карта рендерится автоматически на основе:

* географических координат
* настроек визуализации (цветовая палитра, толщины линий, отступы)
* стилей подписей и маркеров остановок

Используется собственный SVG-рендер.

### 3. Поиск маршрутов (роутинг)

Поддерживает:

* поиск кратчайшего маршрута между двумя остановками
* учёт времени ожидания на остановке
* учёт скорости автобусов
* вывод маршрута как списка шагов: Wait и Bus

Реализовано на основе направленного графа и алгоритма Флойда-Уоршелла.

### 4. JSON API

Проект читает JSON следующего вида:

* base_requests — заполнение базы
* stat_requests — запросы на статистику
* render_settings — настройки рендера карты
* routing_settings — параметры поиска маршрута

Ответ формируется также в JSON.

---

## Архитектура проекта


```
┎── main.cpp
│
├── JsonReader              — парсер JSON, входная точка системы
│   ├── читает документ
│   ├── вызывает RequestHandler
│   └── формирует JSON-ответы
│
├── RequestHandler          — фасад между JSON и логикой
│   ├── TransportCatalogue  — база данных маршрутов
│   ├── MapRenderer         — SVG визуализация
│   └── TransportRouter     — поиск маршрутов
│
├── transport_catalogue     — хранение остановок, автобусов, расстояний
│
├── map_renderer            — построение карты
│
├── transport_router        — построение графа и поиск маршрутов
│
├── json / json_builder     — собственный JSON-парсер и JSON-конструктор
│
└── svg                     — собственная mini-библиотека для рендера SVG
```
_Каждый модуль полностью изолирован и общается через DTO структуры (domain::dto)._

---

## Ключевые технические детали

### Собственный JSON-парсер

Поддерживает:

* строки
* числа (int/double)
* null
* bool
* массивы
* словари
  Работает потоково.

### JSON Builder

Позволяет безопасно строить JSON-ответы в стиле Fluent API:

```cpp
Builder()
    .StartDict()
        .Key("key").Value(123)
        .Key("arr").StartArray().Value(1).Value(2).EndArray()
    .EndDict()
.Build();
```

### SVG-рендер

Построен вручную: Polyline, Circle, Text, Document.

Использует собственный SphereProjector для перевода координат в плоскость.

### Поиск маршрутов

Построение графа:

* каждая остановка - два узла (ожидание и движение)
* рёбра: ожидание, проезд на автобусе
  Ответ содержит последовательность действий.

---

## Используемые технологии

* C++20
* собственные структуры данных
* без сторонних библиотек
* алгоритмы работы с графами
* разбор JSON вручную
* SVG-рендер без внешних зависимостей

---

## Запуск проекта

### Сборка

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

---

### Использование

```bash
# Чтение из stdin, вывод в stdout
./transport_catalogue

# Или с файлами
./transport_catalogue < input.json > output.json
```

В папке src/ представлен пример входного файла input.json для тестирования.

## Пример входного запроса

### Минимальный пример запроса

```json
{
  "base_requests": [
    { 
      "type": "Stop", 
      "name": "A", 
      "latitude": 55.6, 
      "longitude": 37.5, 
      "road_distances": {"B": 1000} 
    },
    { 
      "type": "Stop", 
      "name": "B", 
      "latitude": 55.6, 
      "longitude": 37.56, 
      "road_distances": {} 
    },
    { 
      "type": "Bus", 
      "name": "42", 
      "stops": ["A", "B"], 
      "is_roundtrip": false 
    }
  ],
  "stat_requests": [
    { "id": 1, "type": "Bus", "name": "42" },
    { "id": 2, "type": "Map"}
  ],
  "render_settings": {
          "bus_label_font_size": 20,
          "bus_label_offset": [7, 15],
          "color_palette": ["green"],
          "height": 200,
          "line_width": 14,
          "padding": 30,
          "stop_label_font_size": 20,
          "stop_label_offset": [7, -3],
          "stop_radius": 5,
          "underlayer_color": "red",
          "underlayer_width": 3,
          "width": 200
      },
  "routing_settings": {"bus_wait_time": 2, "bus_velocity": 40}
}
```

### Полный пример

Полный работоспособный пример входного JSON с несколькими маршрутами и запросами доступен в файле `src/input.json`. Он включает:

* 3 автобусных маршрута (кольцевой и некольцевые)
* 10 остановок с координатами и дорожными расстояниями
* Запросы статистики по автобусам и остановкам
* Запросы построения маршрутов между различными остановками
* Запрос визуализации карты
* Настройки рендеринга и маршрутизации
---

## Формат ответа

Для каждого `stat_request` формируется ответ в формате:

```json
[
  {
    "curvature": 0.265302,
    "request_id": 1,
    "route_length": 2000,
    "stop_count": 3,
    "unique_stop_count": 2
  },
  {
    "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n  <polyline points=\"30,30 170,30 30,30\" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <text fill=\"red\" stroke=\"red\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"30\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">42</text>\n  <text fill=\"green\" x=\"30\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">42</text>\n  <text fill=\"red\" stroke=\"red\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"170\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">42</text>\n  <text fill=\"green\" x=\"170\" y=\"30\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">42</text>\n  <circle cx=\"30\" cy=\"30\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"170\" cy=\"30\" r=\"5\" fill=\"white\"/>\n  <text fill=\"red\" stroke=\"red\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"30\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">A</text>\n  <text fill=\"black\" x=\"30\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">A</text>\n  <text fill=\"red\" stroke=\"red\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"170\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">B</text>\n  <text fill=\"black\" x=\"170\" y=\"30\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">B</text>\n</svg>",
    "request_id": 2
  }
]
```

Для маршрута:
```json
[
  {
    "items": [
      {
        "stop_name": "Biryulyovo Zapadnoye",
        "time": 2,
        "type": "Wait"
      },
      {
        "bus": "828",
        "span_count": 2,
        "time": 3,
        "type": "Bus"
      },
      {
        "stop_name": "Biryusinka",
        "time": 2,
        "type": "Wait"
      },
      {
        "bus": "297",
        "span_count": 1,
        "time": 0.42,
        "type": "Bus"
      }
    ],
    "request_id": 5,
    "total_time": 7.42
  }
]
```

## Что можно улучшить

* Добавить сериализацию/десериализацию в файл
* Добавить юнит-тесты
* Подключить GUI для отображения SVG

---

## Назначение проекта

Этот проект демонстрирует:

* архитектурное мышление
* работу с графами
* умение проектировать API
* разработку модульной структуры
* реализацию сложной логики без сторонних библиотек