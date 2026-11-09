# Rupt / Рупт

Современный язык программирования с собственным интерпретатором, байткодом и системой пакетов **Bricks**.  
Modern programming language with its own interpreter, bytecode, and **Bricks** package system.

---

## Русский

### Описание
**Rupt** — язык программирования, сочетающий простоту Python, современный синтаксис, высокую производительность и низкоуровневый контроль. Реализован на C. Имеет свой лексер, парсер, AST, интерпретатор, байткод, стандартную библиотеку, систему пакетов Bricks и расширяемую архитектуру.

### Основные возможности
- Собственный лексер, парсер и AST
- Динамическая типизация с опциональными аннотациями (`name: String = "…"`)
- Интерпретатор с REPL и запуском `.rupt` файлов
- Типы: Int, Float, Bool, String, None, List, Tuple, Set, Map, Function, Class, Instance, Module
- Условия, циклы, функции (аргументы, значения по умолчанию, variadic, лямбды), классы и наследование
- Исключения `try / except / finally / raise`
- Модули и импорт (`import`, `from … import …`)
- **Bricks** — система внешних библиотек (`brick install`, `remove`, `update`, `list`, `search`, `publish`, `info`)
- Стандартная библиотека: math, os, json, random, time, path, sys, string и др.
- Асинхронность `async fn` / `await`
- Обработка ошибок с расшифровкой (type, message, location, suggestions)
- **Графика:** `gfx_init`, `gfx_color("#RRGGBB")`, `gfx_pixel`, `gfx_line`, `gfx_rect`, `gfx_circle`, `gfx_fill_rect`, `gfx_fill_circle`, `gfx_show`, `gfx_save`, `gfx_close`
- Формат скомпилированного байткода `.rbc`
- Форматер, линтер, тесты, профилировщик, отладчик (раздел инструментов)

### Быстрый старт
```text
rupt program.rupt
rupt repl
rupt build
rupt fmt
rupt lint
rupt test
rupt debug
```

Пример кода:
```rupt
rupt
fn fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

for i in range(10):
    print(fibonacci(i))
```

Пример графики:
```rupt
rupt
gfx_init(30, 15)
gfx_color("#FF0000")
gfx_draw_rect(2, 2, 8, 4)
gfx_color("#00FF00")
gfx_fill_rect(12, 6, 8, 4)
gfx_color("#0000FF")
gfx_draw_circle(20, 8, 3)
gfx_show()
gfx_save("pic.ppm")
gfx_close()
```

### Установка (Windows)
1. Скачайте `installer/installer.exe`.
2. Запустите установщик.
3. Перезапустите терминал.

Или вручную: скопируйте `rupt.exe`, `brick.exe`, папки `std`, `registry`, `bricks` и добавьте `bin` в PATH.

Пример работы:
```
$ rupt program.rupt
0
1
1
2
3
5
8
13
21
34
by cit
```

### Bricks (пакеты)
```text
brick install <name>
brick remove <name>
brick update <name>
brick list
brick search <name>
brick publish
brick info <name>
```

### Разработка
Rupt развивается благодаря контрибьюторам. Любой вклад (код, документация, тесты, идеи) помогает языку расти.

---

## English

### Description
**Rupt** — a modern programming language combining Python-like simplicity, modern syntax, high performance, and low-level control. Implemented in C. It has its own lexer, parser, AST, interpreter, bytecode, standard library, and **Bricks** package system.

### Features
- Custom lexer, parser, and AST
- Dynamic typing with optional annotations (`name: String = "…"`)
- Interpreter with REPL and `.rupt` file execution
- Types: Int, Float, Bool, String, None, List, Tuple, Set, Map, Function, Class, Instance, Module
- Conditions, loops, functions (arguments, defaults, variadic, lambdas), classes and inheritance
- Exceptions `try / except / finally / raise`
- Modules and imports (`import`, `from … import …`)
- **Bricks** — external library system (`brick install`, `remove`, `update`, `list`, `search`, `publish`, `info`)
- Standard library: math, os, json, random, time, path, sys, string, and more
- Async `async fn` / `await`
- Professional error output (type, message, source location, suggestions)
- **Graphics:** `gfx_init`, `gfx_color("#RRGGBB")`, `gfx_pixel`, `gfx_line`, `gfx_rect`, `gfx_circle`, `gfx_fill_rect`, `gfx_fill_circle`, `gfx_show`, `gfx_save`, `gfx_close`
- Compiled bytecode format `.rbc`
- Formatter, linter, tests, profiler, debugger

### Quick Start
```text
rupt program.rupt
rupt repl
rupt build
rupt fmt
rupt lint
rupt test
rupt debug
```

Example:
```rupt
rupt
fn fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

for i in range(10):
    print(fibonacci(i))
```

Graphics example:
```rupt
rupt
gfx_init(30, 15)
gfx_color("#FF0000")
gfx_draw_rect(2, 2, 8, 4)
gfx_color("#00FF00")
gfx_fill_rect(12, 6, 8, 4)
gfx_color("#0000FF")
gfx_draw_circle(20, 8, 3)
gfx_show()
gfx_save("pic.ppm")
gfx_close()
```

### Installation (Windows)
1. Download `installer/installer.exe`.
2. Run the installer.
3. Restart the terminal.

Or manually: copy `rupt.exe`, `brick.exe`, `std`, `registry`, `bricks` folders and add `bin` to PATH.

Example:
```
$ rupt program.rupt
0
1
1
2
3
5
8
13
21
34
by cit
```

### Bricks (packages)
```text
brick install <name>
brick remove <name>
brick update <name>
brick list
brick search <name>
brick publish
brick info <name>
```

### Development
Rupt grows thanks to contributors. Any contribution (code, docs, tests, ideas) helps the language evolve.

---

## License

MIT License. See [LICENSE.md](LICENSE.md).

---

© 2026 Rupt contributors