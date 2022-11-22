![](https://github.com/khva/kvstor/workflows/C%2FC%2B%2B%20CI/badge.svg?branch=base_impl)


## Оглавление
- [Описание](#описание)
- [Примеры использования](#примеры-использования)
- [Как добавить библиотеку в ваш проект](#как-добавить-библиотеку-в-ваш-проект)
- [Дополнительно](#дополнительно)


## Описание
Библиотека kvstor реализует защищенное key-value хранилище.

Основные особенности библиотеки:
 - быстрый доступ к элементам, добавление, удаление, поиск за *O(1)*
 - ограничение размера хранилища (при переполнении наиболее старый элемент замещается новым)
 - потокобезопасность (thread-safe)
 - простота подключения (header-only / single-file)
 - отсутствие внешних зависимостей (в коде библиотеки используется только STL)
 - кроссплатформенность


## Примеры использования
Класс реализующий хранилище `kvstor::storage_t<>` находится в заголовке `include/kvstor.h`.

Добавление и поиск элемента:
```c++
    // инициализация хранилища с максимальным размером 4 и временем жизни элементов в 24 часа
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

Удаление элемента:
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

Печать элементов хранилища:
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

## Как добавить библиотеку в ваш проект
Весь код библиотеки содержится в одном файле `include/kvstor.h`. Наиболее простой способ добавления библиотеки в ваш проект:
 - скопировать файл `kvstor.h` в удобное для вас место, например: `third_party/kvstor/kvstor.h`
 - добавить в настройках проекта путь к `kvstor.h`, например для CMake проекта: `include_directories(third_party/kvstor)`

Также библиотеку можно добавить как cабмодуль:
 - `git submodule add https://github.com/khva/kvstor`
 - `git add .`
 - `git commit -m "add kvstor module"`
 - `git push origin master`


 ## Дополнительно
  - Библиотека проверялась на компиляторах gcc 11.3, MS Visual Studio 2022 с флагом `CXX_STANDARD 17`
  - Минимальная версия CMake 3.12
  - Для тестирования используется фреймворк [doctest](https://github.com/doctest/doctest) версия 2.4.9 (как часть проекта в директории `tests\doctest`)
