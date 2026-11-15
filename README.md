# OptiLang++ — Revised Steps 1 and 2 only

This package updates **Step 1 (the language syntax)** and **Step 2 (shared AST and symbol table)** according to the latest agreed requirements. It deliberately **does not** change the separate full compiler implementation.

Changes:
- `var x: int;` / `var y: real;` (no bound in variable declarations)
- All bounds and mathematical restrictions are written as constraints inside `subject to:`
- Unconstrained `real` variables start free over R; unconstrained `int` variables over Z
- No `z in R;` syntax
- All rich syntax is retained: compile-time parameters, types, named objectives, named constraints, `solve`, `print`
- Feature 1 (simplification) and Feature 2 (static error checks) remain in the planned semantic pipeline

Read `STEP1_LANGUAGE_SPEC.md`, then `STEP2_AST_SYMBOL_TABLE.md`. The headers under `include/` are the revised shared code for Step 2, and the C++ smoke test is in `tests/`.

Run the test:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tests/ast_symbol_demo.cpp -o ast_demo
./ast_demo
```
