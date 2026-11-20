# OptiLang

```text
 ██████╗  ██████╗  ████████╗ ██╗ ██╗       █████╗  ███╗   ██╗  ██████╗
██╔═══██╗ ██╔══██╗ ╚══██╔══╝ ██║ ██║      ██╔══██╗ ████╗  ██║ ██╔════╝
██║   ██║ ██████╔╝    ██║    ██║ ██║      ███████║ ██╔██╗ ██║ ██║  ███╗
██║   ██║ ██╔═══╝     ██║    ██║ ██║      ██╔══██║ ██║╚██╗██║ ██║   ██║
╚██████╔╝ ██║         ██║    ██║ ███████╗ ██║  ██║ ██║ ╚████║ ╚██████╔╝
 ╚═════╝  ╚═╝         ╚═╝    ╚═╝ ╚══════╝ ╚═╝  ╚═╝ ╚═╝  ╚═══╝  ╚═════╝
```

OptiLang is a C++17 compiler for a small language for linear and mixed-integer
optimization. Write decision variables, an objective, and constraints in a
`.opt` file to find the best allocation, cost, or profit allowed by the model.

```text
.opt source
    -> Flex lexer -> Bison parser -> abstract syntax tree (AST)
    -> symbol table -> semantic analysis -> sparse linear IR
    -> GLPK optimizer (with solve;) -> status and solution
```

CMake builds the compiler and generates the Flex and Bison sources. CTest runs
the C++ unit tests and command-line tests. GLPK uses simplex for continuous
models and integer optimization for models with integer variables.

## Build and run

On Arch Linux or Omarchy, install GCC, Make, CMake, Flex, Bison, and GLPK:

```bash
sudo pacman -S --needed gcc make cmake flex bison glpk
```

From the root of the cloned repository:

```bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake -j
./build/cmake/optilang examples/simple.opt
```

Use any C++17-compatible toolchain with CMake 3.16 or later. The
`ctest --test-dir` command below requires CMake 3.20 or later.

## Mixed-integer example

[examples/simple.opt](examples/simple.opt) maximizes profit under a shared
capacity. The integer variable `x` earns 3 per unit and the real variable `y`
earns 5, so all 100 units of capacity go to `y`.

```text
param capacity = 100;

var x: int;
var y: real;

maximize profit: 3*x + 5*y;

subject to:
    c1: x + y <= capacity;
    c2: x >= 0;
    c3: y >= 0;

solve;
print x, y, profit;
```

Solution excerpt:

```text
Method: MILP
Status: OPTIMAL
x = 0
y = 100
profit = 500
```

For a continuous minimization example, run
`./build/cmake/optilang examples/continuous.opt`. It returns `x = -2.5` and
`cost = -1.5`.

## Language at a glance

- Write optional `param` declarations first. Parameters are evaluated at
  compile time and may reference parameters declared later.
- Declare at least one `var` with type `real` or `int`. All variables are
  unrestricted until the model constrains them.
- Define an optional named `maximize` or `minimize` objective followed by
  an optional `subject to:` section. Constraints use `<=`, `>=`, or `=`
  and may have labels.
- Use affine expressions with `+`, `-`, multiplication by a constant,
  parentheses, and decimal or scientific-notation literals. Start a comment
  with `#`.
- Use `solve;` to optimize a model with an objective. An optional trailing
  `print` selects decision-variable or objective values. By default all
  solution values are printed when available.

The executable returns `0` for successful validation or a reported optimum,
`1` for input or compiler errors, and `2` when solving finishes without a
proven optimum.

## Tests

```bash
ctest --test-dir build/cmake --output-on-failure
```

With CMake 3.16–3.19, run `(cd build/cmake && ctest --output-on-failure)`.

## Repository layout

| Path | Contents |
|---|---|
| `src/` | Lexer, parser, driver, semantic analysis, IR lowering, and GLPK backend |
| `include/core/` | AST, symbol table, and compiler interfaces |
| `examples/` | Runnable continuous and mixed-integer models |
| `tests/` | C++ unit tests and CTest command-line cases |
| `docs/` | [Language specification](docs/STEP1_LANGUAGE_SPEC.md) and [AST and symbol table](docs/STEP2_AST_SYMBOL_TABLE.md) |
| `CMakeLists.txt` | Build and test configuration |

## Known limitations

- OptiLang supports affine models. Division, strict inequalities, arrays,
  and functions are outside the language.
- Computation uses floating point and GLPK tolerances. For example, the
  current simplex configuration can misclassify a bound with a coefficient
  of `1e-10`. Use coefficients and units of comparable scale.
- The current GLPK backend can abort on a variable name or constraint label
  longer than 255 bytes.
- The MILP backend may report `UNKNOWN` when the continuous relaxation is
  unbounded and the integer model's status has not been certified.

## Credits

See the repository's Git history for OptiLang contributions. The compiler
uses Flex and GNU Bison for parsing and GNU GLPK for optimization.
