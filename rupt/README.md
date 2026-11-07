# Rupt

## Русский

Rupt — современный язык программирования с собственным интерпретатором на C.

### Запуск

```text
rupt program.rupt
rupt run program.rupt
rupt
```

### Пример

```rupt
fn fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

for i in range(10):
    print(fibonacci(i))
```

### Сборка

```text
build.bat
```

## English

Rupt is a modern programming language with a C-based interpreter.

### Run

```text
rupt program.rupt
rupt run program.rupt
rupt
```

### Example

```rupt
fn fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

for i in range(10):
    print(fibonacci(i))
```

### Build

```text
build.bat
```