# Rupt Extension for VS Code

Syntax highlighting, snippets, and language support for the [Rupt programming language](https://github.com/linkupcit34/rupt-programming-language).

## Features

- Syntax highlighting for `.rupt` files (keywords, types, built-ins, strings, numbers, operators)
- Code snippets (`fn`, `class`, `if`, `for`, `while`, `try`, `match`, `asyncfn`, `print`)
- Language configuration (comments, brackets, auto-closing pairs)

## Install

1. Open VS Code Extensions view (`Ctrl+Shift+X`).
2. Click `...` → `Install from VSIX...` or run:
   ```bash
   code --install-extension rupt.vsix
   ```
3. Open any `.rupt` file.

## Example Rupt code

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
gfx_show()
gfx_save("pic.ppm")
gfx_close()
```

## Files

- `syntaxes/rupt.tmLanguage.json` — TextMate grammar for syntax highlighting
- `language-configuration.json` — comments, brackets, auto-closing pairs
- `snippets/rupt.code-snippets` — code snippets

## License

MIT License. See [LICENSE.md](LICENSE.md).
