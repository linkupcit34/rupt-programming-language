# Rupt

## Русский

Rupt — современный язык программирования с собственным интерпретатором, написанным на C.  
Философия: простота для новичка, мощь для реальных приложений.

### Принципы

- Собственный лексер, парсер и AST.
- Динамическая типизация с опциональными аннотациями.
- Интерпретатор с REPL и запуском файлов.
- Модульная архитектура: compiler / runtime / std / bricks.
- Расширяемость: Bricks, native-модули, JIT-ready дизайн.

### Запуск

```text
rupt program.rupt
rupt run program.rupt
rupt
```

### Синтаксис

```rupt
name = "Rupt"
print("Hello {name}")

fn add(a, b):
    return a + b

class User:
    fn __init__(self, name):
        self.name = name

    fn greet(self):
        print("Hello", self.name)
```

### Типы данных

- Int, Float, Bool, String, None
- List, Tuple, Set, Map
- Function, Class, Instance, Module, Range

### Управляющие конструкции

- if / elif / else
- for / while
- break / continue / pass
- try / except / finally / raise
- match / case
- with

### Функции

- позиционные, именованные, значения по умолчанию
- variadic (*args, **kwargs)
- замыкания, рекурсия, lambda
- генераторы (базовая поддержка)

### Коллекции

- List: индексация, срезы, comprehensions
- Map: ключ-значение
- Set: уникальные элементы
- Tuple: неизменяемые наборы

### Модули

```rupt
import math
from math import sqrt
```

Поиск модулей: текущая директория, `std/`, `bricks/<name>/`.

### Стандартная библиотека

Модули: math, os, json, random, time, path, sys, string, collections, regex, datetime, terminal, environment, logging, files, network, http, crypto, hash, threading, process, subprocess, async, itertools, functools.

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

Пакеты хранятся в `bricks/`, реестр — в `registry/`.  
Пример brick: `rupt_timer`.

### Ошибки

Профессиональный вывод: тип ошибки, файл, строка, колонка, stack trace, подсказки.

### Сборка

```text
build.bat
```

## English

Rupt is a modern programming language with a C-based interpreter.  
Philosophy: simple for beginners, powerful for real applications.

### Principles

- Custom lexer, parser, and AST.
- Dynamic typing with optional annotations.
- Interpreter with REPL and file execution.
- Modular architecture: compiler / runtime / std / bricks.
- Extensible: Bricks, native modules, JIT-ready design.

### Run

```text
rupt program.rupt
rupt run program.rupt
rupt
```

### Syntax

```rupt
name = "Rupt"
print("Hello {name}")

fn add(a, b):
    return a + b

class User:
    fn __init__(self, name):
        self.name = name

    fn greet(self):
        print("Hello", self.name)
```

### Data Types

- Int, Float, Bool, String, None
- List, Tuple, Set, Map
- Function, Class, Instance, Module, Range

### Control Flow

- if / elif / else
- for / while
- break / continue / pass
- try / except / finally / raise
- match / case
- with

### Functions

- positional, keyword, default arguments
- variadic (*args, **kwargs)
- closures, recursion, lambda
- generators (basic support)

### Collections

- List: indexing, slicing, comprehensions
- Map: key-value
- Set: unique elements
- Tuple: immutable collections

### Modules

```rupt
import math
from math import sqrt
```

Module search: current directory, `std/`, `bricks/<name>/`.

### Standard Library

Modules: math, os, json, random, time, path, sys, string, collections, regex, datetime, terminal, environment, logging, files, network, http, crypto, hash, threading, process, subprocess, async, itertools, functools.

### Bricks (Packages)

```text
brick install <name>
brick remove <name>
brick update <name>
brick list
brick search <name>
brick publish
brick info <name>
```

Packages live in `bricks/`, registry in `registry/`.  
Example brick: `rupt_timer`.

### Errors

Professional output: error type, file, line, column, stack trace, suggestions.

### Build

```text
build.bat
```