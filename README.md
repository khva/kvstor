# kvstor - защищенное key-value хранилище с ограничением по размеру

[![C++](https://img.shields.io/badge/c%2B%2B-17-informational.svg)](https://shields.io/)
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)
![](https://github.com/khva/kvstor/workflows/linux/badge.svg)
![](https://github.com/khva/kvstor/workflows/macos/badge.svg)
![](https://github.com/khva/kvstor/workflows/windows/badge.svg)


## Оглавление
- [Описание](#описание)
- [Примеры использования](#примеры-использования)
  - [Пример: добавление и поиск элемента](#пример-добавление-и-поиск-элемента)
  - [Пример: удаление элемента](#пример-удаление-элемента)
  - [Пример: добавление элемента с проверкой, что значение элемента не изменилось с прошлого обращения](#пример-добавление-элемента-с-проверкой-что-значение-элемента-не-изменилось-с-прошлого-обращения)
  - [Пример: печать элементов хранилища](#пример-печать-элементов-хранилища)
  - [Пример: получение дампа хранилища](#пример-получение-дампа-хранилища)
  - [Пример: построение хранилища из дампа](#пример-построение-хранилища-из-дампа)
- [Как добавить библиотеку в ваш проект](#как-добавить-библиотеку-в-ваш-проект)
- [Дополнительно](#дополнительно)


## Описание
Библиотека kvstor реализует защищенное key-value хранилище с ограничением по размеру.

Основные особенности библиотеки:
 - быстрый доступ к элементам, добавление, удаление, поиск за *O(1)*
 - ограничение максимального размера хранилища (при переполнении наиболее старый элемент замещается новым)
 - потокобезопасность (thread-safe)
 - простота подключения (header-only / single-file)
 - отсутствие внешних зависимостей (в коде библиотеки используется только STL)
 - кроссплатформенность


## Примеры использования
Класс реализующий хранилище `kvstor::storage_t<>` находится в заголовке `include/kvstor.h`.

**Важно:** для типа-значения *value_type* должны быть определены операции равенства и не равенства (*operator==() / operator!=()*).

### Пример: добавление и поиск элемента
```c++
// инициализация хранилища с максимальным размером 4
kvstor::storage_t<int, std::string> stor{ 4 };
stor.push(1, "first");
stor.push(2, "second");
stor.push(3, "third");
stor.push(2, "second again");  // по ключу 2 будет новое значение "second again"
stor.push(4, "fourth");
stor.push(5, "fourth");  // пара {1, "first"} будет замещена новой

const auto first = stor.find(1);
if (!first)
    std::cout << "item 1 does not exist" << std::endl;

const auto second = stor.find(2);
if (second)
    std::cout << "item 2 has value: " << *second << std::endl;
```
В результате выполнения будет выведено:
```
item 1 does not exist
item 2 has value: second again
```


### Пример: удаление элемента
```c++
kvstor::storage_t<int, std::string> stor{ 10 };
stor.push(1, "first");
stor.push(2, "second");
stor.push(3, "third");

stor.erase(2);
const auto second = stor.find(2);
if (!second)
    std::cout << "item 2 does not exist" << std::endl;
```
В результате выполнения будет выведено:
```
item 2 does not exist
```


### Пример: добавление элемента с проверкой, что значение элемента не изменилось с прошлого обращения

```c++
kvstor::storage_t<int, std::string> stor{ 10 };
auto expected = stor.find(1); // пустое значение, элемента по ключу 1 нет

bool flag = stor.compare_exchange(1, "10", expected);
// успешно: expected остается пустым, по ключу 1 находится значение "10"

flag = stor.compare_exchange(1, "11", expected);
// неуспешно: expected теперь содержит "10" - текущее значение элемента с ключом 1

flag = stor.compare_exchange(1, "11", expected);
// теперь успешно: expected по-прежнему содержит "10", а по ключу 1 находится новое значение "11"
```


### Пример: печать элементов хранилища
```c++
using stor_t = kvstor::storage_t<long, std::string>;
stor_t stor{ 4 };

stor.push(1, "first");
stor.push(2, "second");
stor.push(3, "third");
stor.push(2, "second again");
stor.push(4, "fourth");
stor.push(5, "fourth");

const size_t size = stor.size();
size_t count = 0;

auto print = [size, &count](long key, const std::string& value)
{
    ++count;
    std::cout << "{ " << key << ", " << value << " }" << (count < size ? ", " : " ");
};

std::cout << "{ ";
stor.map(print);
std::cout << "}" << std::endl;
```
В результате выполнения будет выведено:
```
{ { 5, fourth }, { 4, fourth }, { 2, second again }, { 3, third } }
```
Обратите внимание, что элементы идут в обратном порядке попадания в хранилище (наиболее новые сначала).


### Пример: получение дампа хранилища
```c++
    kvstor::storage_t<int, int> stor{ 10 };
    stor.push(1, 10);
    stor.push(2, 20);
    stor.push(3, 30);
    stor.push(4, 40);

    const std::vector<std::pair<int, int>> & dump = stor.dump();
    for (const auto & [key, value] : dump)
        std::cout << key << "\t" << value << std::endl;
```
В результате выполнения будет выведено:
```
4       40
3       30
2       20
1       10
```



### Пример: построение хранилища из дампа
```c++
    const std::vector<std::pair<int, int>> dump = { {5, 50}, {4, 40}, {3, 30}, {2, 20}, {1, 10}, };
    const kvstor::storage_t<int, int> stor{ dump, 3 };

    auto print = [](int key, int value)
    {
        std::cout << key << "\t" << value << std::endl;
    };
    stor.map(print);
```
В результате выполнения будет выведено:
```
5       50
4       40
3       30
```
Максимальный размер хранилища ограничен 3 элементами, поэтому будет добавлено только 3 первых элемента дампа.



## Как добавить библиотеку в ваш проект
Весь код библиотеки содержится в одном файле `include/kvstor.h`. Наиболее простой способ добавления библиотеки в ваш проект:
 - скопировать файл `kvstor.h` в удобное для вас место, например: `third_party/kvstor/kvstor.h`
 - добавить в настройках проекта путь к `kvstor.h`, например для CMake проекта: `include_directories(third_party/kvstor)`

Также библиотеку можно добавить как cабмодуль:
```bash
git submodule add https://github.com/khva/kvstor third_party/kvstor
```


## Дополнительно
 - Библиотека проверялась на компиляторах gcc 11.3, Apple clang 13, MS Visual Studio 2019/2022
 - Минимальная версия CMake 3.12
 - Для тестирования используется фреймворк [doctest](https://github.com/doctest/doctest) версия 2.4.9 (как часть проекта в директории `tests/doctest`)
