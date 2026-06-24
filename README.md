# SciCalc Pro

A terminal-based scientific calculator with a full TUI (text user interface), written in a single C++ file. Features a color-coded button grid, expression parser, and keyboard navigation — no dependencies beyond the C++ standard library.

---

## Features

- **Scientific functions** — `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `cbrt`, `log`, `ln`, `abs`, `exp`, `ceil`, `floor`, `round`
- **Constants** — `pi`, `e`
- **Operators** — `+`, `-`, `*`, `/`, `^` (power), `%` (modulo)
- **Modes** — Radians / Degrees toggle
- **Navigation** — Arrow keys + Enter, or direct keyboard shortcuts
- **Display** — 24-bit ANSI color, expression history, error highlighting
- **Auto-resizing** — Centers itself on any terminal size

---

## Build

**Requirements:** A C++17 compiler (`g++` or `clang++`), Linux or macOS.

```bash
g++ -O2 -std=c++17 -o calc calc.cpp -lm
./calc
```

---

## Usage

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `0`–`9`, `.` | Digit / decimal input |
| `+` `-` `*` `/` | Arithmetic operators |
| `^` | Power (`x^y`) |
| `%` | Modulo |
| `(` `)` | Parentheses |
| `=` or `Enter` (on `=` button) | Evaluate |
| `Backspace` | Delete last character |
| `a` | Clear all (AC) |
| `s` | Insert `sin(` |
| `c` | Insert `cos(` |
| `t` | Insert `tan(` |
| `l` | Insert `log(` |
| `n` | Insert `ln(` |
| `r` | Insert `sqrt(` |
| `p` | Insert `pi` |
| `e` | Insert `e` |
| `m` | Toggle RAD / DEG mode |
| Arrow keys | Navigate button grid |
| `Enter` | Press selected button |
| `q` / `Esc` | Quit |

### Button Grid Navigation

Use **arrow keys** to move the selection highlight across the button grid, then press **Enter** to activate the selected button. The highlighted button is shown in bright blue.

---

## Expression Syntax

Expressions follow standard mathematical notation:

```
2 + 3 * 4        → 14      (operator precedence respected)
(2 + 3) * 4      → 20
2^10             → 1024
sin(pi/2)        → 1
sqrt(2)          → 1.41421356237
log(100)         → 2       (base-10)
ln(e)            → 1       (natural log)
1/(3+4)          → 0.142857142857
```

Functions must include parentheses: `sin(x)`, not `sin x`.

---

## Code Structure

The entire program lives in `calc.cpp` (~400 lines), organized into logical sections:

| Section | Contents |
|---------|----------|
| ANSI helpers | Macros and RGB color definitions |
| `RawMode` | RAII terminal raw-mode handler |
| Key reading | `read_key()` — maps escape sequences to enum values |
| `Parser` | Recursive-descent expression parser |
| `fmt()` | Formats doubles cleanly (strips trailing zeros) |
| Button definitions | `GRID` — the full button layout |
| `Calc` | Calculator state and actions (`eval`, `ins`, `back`, etc.) |
| Drawing | `draw()`, `fill()`, `rtext()`, `ctext()` — TUI rendering |
| `main()` | Event loop |

---

## Known Limitations

- **No memory / history recall** — results aren't stored between sessions
- **`-0` display** — typing `-0` shows `-0` instead of `0` (cosmetic only)
- **No implicit multiplication** — `2(3+4)` won't work; use `2*(3+4)`
- **Single expression** — no multi-line or variable assignment support
- **Terminal requirement** — needs a real TTY; won't run in pipes or non-interactive shells

---

## Error Handling

| Condition | Behavior |
|-----------|----------|
| Division by zero | Shows `Error` in red |
| `sqrt` of negative | Shows `Error` in red |
| `log`/`ln` of ≤ 0 | Shows `Error` in red |
| Mismatched parentheses | Shows `Error` in red |
| Unknown function name | Shows `Error` in red |
| Next input after error | Clears and starts fresh |
