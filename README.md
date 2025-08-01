# ConcurrentCalc
Калькулятор, в неблокирующей манере принимающий выражения с длительным периодом вычислений

## Сборка и запуск

- Скачать архив из этого репозитория, распаковать на компьютере.
- Открыть терминал в директории `ConcurrentCalc-main`
- Собрать проект
```
$ ls
Backend             calcmainwindow.h  LibDoit   Model      ui
calcmainwindow.cpp  CMakeLists.txt    main.cpp  README.md
$ mkdir build
$ cd build/
$ cmake .. && cmake --build .
```
- Запустить программу
```
$ ./bin/ConcurrentCalc
```

## Использование

Программа представляет собой простой калькулятор с возможностью задания арифметического выражения и установки таймаута для него, по истечении которого выражение считается решенным. Такой подход позволяет имитировать длительные вычисления.

В поле ввода можно добавить выражение, например `-2 * -5 + 3 /-7 * -14 + 15` и нажать `=`. Выражение отправится на расчет.

⚠️ Сам по себе расчет не представляет математической ценности, так как операции считаются без приоритета, просто последовательно применяясь к уже вычисленному результату. Не вздумайте пользоваться этим калькулятором в научных целях! ⚠️

В левом верхнем углу есть опции программы. Первая из них (если считать сверху) очищает вывод, который осуществляется в процессе работы программы. Вторая переключает вычислителя с внутренней логики приложения на внешнюю, библиотечную (плагин).

## Способы использования

- Повыставлять задержку вычислений в разные значения и путем вставки (вместе с `=`) `-2 * -5 + 3 /-7 * -14 + 15 - 1.725=` посмотреть, что будет.
- Попробовать сделать то же, но переключив вычислителя.
- Очистить вывод.
- Произвести все три действия с выражением, где присутствует деление на `0`, например: `2 * 5 + -3 /-7 * -14 * 2.5 /0=`.

## Ограничения

- Они есть, но их непросто найти.

## Технологии

- С++17
- Qt
- CMake
