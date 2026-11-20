# Step 2 — Shared AST and symbol table (revised)

The Flex/Bison frontend creates `Program` and its expression nodes. The
symbol table builder registers declarations and evaluates parameters. The
semantic analyzer validates the AST, and the IR builder lowers affine
expressions to a solver model.

## File responsibilities

- `include/core/ast.hpp`: common expression and statement node types, source spans and helper constructors.
- `include/core/symbol_table.hpp`: declaration metadata, name lookup, and optional individual bounds in variable metadata.
- `tests/ast_symbol_demo.cpp`: verifies the interfaces independently of Flex/Bison.

## AST outline

```text
Program
  parameters: [ParameterDecl(name, Expr)]
  variables:  [VariableDecl(name, type)]
  objective:  optional ObjectiveDecl(sense, name, Expr)
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
    Relation relation;       // Strict relations are rejected by semantic analysis.
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

Both optionals are empty for variables built from source. Constraints are
normalized as separate IR rows. The CLI pipeline currently leaves these bound
fields empty. If a caller supplies explicit IR bounds for an integer variable,
the GLPK backend applies `ceil(lowerBound)` and `floor(upperBound)`.

`SymbolTable::insert()` rejects duplicate names. `find()` provides read-only
lookup and `findMutable()` provides mutable lookup. Declared identifiers share
one namespace.

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

All three `VariableSymbol` entries have empty lower and upper bounds. The
IR stores `minX`, `maxX`, and `capacity` as separate rows. GLPK enforces the
bounds on `x` through the first two rows.

## Build smoke test

From the repository root, after configuring and building as described in the
[README](../README.md):

```bash
ctest --test-dir build/cmake -R '^test_ast$' --output-on-failure
```

The smoke test creates an AST and exercises symbol-table lookup independently
of the parser and solver.
