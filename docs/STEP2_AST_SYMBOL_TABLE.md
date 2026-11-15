# Step 2 — Shared AST and symbol table (revised)

The frontend (Person 1, Flex/Bison) creates `Program` and its expression nodes; the backend (Person 2) validates and lowers the AST. The parser does **not** simplify source expressions.

## File responsibilities

- `include/optilang/ast.hpp`: common expression and statement node types, source spans and helper constructors.
- `include/optilang/symbol_table.hpp`: declaration metadata, name lookup, and optional individual bounds recorded by later semantic analysis.
- `tests/ast_symbol_demo.cpp`: verifies the interfaces independently of Flex/Bison.

## AST outline

```text
Program
  parameters: [ParameterDecl(name, Expr)]
  variables:  [VariableDecl(name, type)]               <-- NO BOUNDS HERE
  objective:  ObjectiveDecl(sense, name, Expr)
  constraints: [ConstraintDecl(label?, left, op, right)]
  solve:      optional source location
  print:      optional list of identifiers

Expr = NumberExpr | NameExpr | UnaryExpr | BinaryExpr
```

The revised variable declaration in C++:

```cpp
struct VariableDecl {
    std::string name;
    VariableType type;       // Integer or Real
    SourceSpan span{};
};
```

`ConstraintDecl` is unchanged, so bound-like statements remain ordinary constraints:

```cpp
struct ConstraintDecl {
    std::string name;        // Empty = unnamed
    ExprPtr left;
    Relation relation;       // <=, >=, =
    ExprPtr right;
    SourceSpan span{};
};
```

## Symbol table

The symbol table stores evaluated constants and metadata. A variable starts with *no individual finite bounds*.

```cpp
struct VariableSymbol {
    VariableType type;
    std::optional<double> lowerBound;
    std::optional<double> upperBound;
};
```

Both optionals are empty when the variable is declared. **They are not parsed from `var`.** At a later stage, semantic analysis may populate them from constraints; for instance `x >= 0;` and `x <= 40;` imply `[0,40]`. Do not infer a single-variable bound from a multi-variable inequality without additional reasoning.

For `int` variables, the backend can tighten fractional individual bounds using `ceil(lowerBound)` and `floor(upperBound)` and detect empty integer intervals.

`SymbolTable::insert()` rejects duplicate names. `find()` is read-only, while `findMutable()` exists for later semantic bound inference. All names share one namespace in v1.

## Division between teammates

| Person 1 — Parser | Person 2 — Semantics |
|---|---|
| Build expression nodes | Evaluate parameter values |
| Build variable declarations containing name and type only | Register variables as unbounded |
| Build named and unnamed constraint nodes | Validate linearity, symbols, arithmetic |
| Preserve line/column spans | Simplify expressions and infer simple bounds |
| Parse `solve`/`print` | Validate `print`, generate normalized linear IR |

## Example AST-to-symbol processing

Input:

```text
var x: int;
var y: real;
var z: real;
maximize profit: 3*x + 5*y;
subject to:
    minX: x >= 0;
    maxX: x <= 40;
    capacity: x + y <= 100;
solve;
print x, y, z, profit;
```

Initially all three `VariableSymbol`s have empty lower/upper bounds. After **later** analysis, `x` has `[0,40]`, but `y` and `z` still have no individual finite bounds. The general `x+y <= 100` constraint stays in the linear model.

## Build smoke test

From this folder:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude \
    tests/ast_symbol_demo.cpp -o ast_demo
./ast_demo
```

Expected: `Updated Step 1/2 AST and symbol table smoke test passed.`

This is an AST and symbol-table contract test only; the Flex/Bison parser, full semantic analyzer, and solver are intentionally outside Step 1/2.
