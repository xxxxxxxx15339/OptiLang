# Step 1 — OptiLang++ language syntax (revised)

This is the agreed **rich** OptiLang++ design. The only changes relative to the earlier design are:

- Variables are declared **without lower or upper bounds**: `var x: real;` or `var n: int;`.
- All restrictions are written inside `subject to:`.
- Variables have **no finite individual bounds by default**. They are free real variables (`real`) or unrestricted integers (`int`) unless constrained.
- The `in R` form is **removed**: `var z: real;` is sufficient for a real variable without explicit bounds.

The language retains compile-time parameters, typed variables, named objectives and constraints, arithmetic, `solve;`, and `print`.

## Complete example

```text
# Compile-time parameters
param profitA = 30;
param profitB = 50;
param capacity = 100;
param setup = 4;

# Names and types only; no individual bounds declared here
var x: int;
var y: real;
var z: real;

maximize totalProfit:
    profitA*x + profitB*y + 2*x - x;

subject to:
    machineLimit: 2*x + 3*y + setup <= capacity;
    minX: x >= 0;
    maxX: x <= 40;
    minY: y >= 0;
    # z has no individual bounds and is a free real variable

solve;
print x, y, z, totalProfit;
```

## EBNF grammar

Here, `{ ... }` means zero or more repetitions, `[ ... ]` is optional, and literal terminals are quoted. `EOF` marks the end of the file.

```ebnf
program =       // The square brackets mean the declaration is optional.
    { parameter_decl },
    variable_decl,
    { variable_decl },
    [ objective ],
    [ constraint_section ],
    [ solve_stmt ],
    [ print_stmt ],
    EOF ;
parameter_decl     = "param", IDENTIFIER, "=", expression, ";" ;
variable_decl      = "var", IDENTIFIER, ":", type, ";" ;
type               = "int" | "real" ;
objective          = ("maximize" | "minimize"), IDENTIFIER, ":", expression, ";" ;
constraint_section = "subject", "to", ":", constraint, { constraint } ;
constraint         = [ IDENTIFIER, ":" ], expression, relation, expression, ";" ;
relation           = "<=" | ">=" | "=" ;
solve_stmt         = "solve", ";" ;
print_stmt         = "print", IDENTIFIER, { ",", IDENTIFIER }, ";" ;
expression         = term, { ("+" | "-"), term } ;
term               = unary, { "*", unary } ;
unary              = ("+" | "-"), unary | primary ;
primary            = NUMBER | IDENTIFIER | "(", expression, ")" ;
```

### Lexical rules

- `IDENTIFIER`: `[A-Za-z_][A-Za-z0-9_]*`, except reserved keywords.
- `NUMBER`: ([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][+-]?[0-9]+)?
- Comments start with `#` and run to the end of the line.
- Whitespace is ignored except for error locations.
- Operators: `+`, `-`, `*`, `<=`, `>=`, `=`, `:`, `;`, `,`, `(`, `)`.
- Reserved keywords: `param`, `var`, `int`, `real`, `maximize`, `minimize`, `subject`, `to`, `solve`, `print`.

## OptiLang v1 — Final semantic contract

1. At least one decision variable must be declared.
2. Parameters are optional. Their values are evaluated at compile time; forward references are allowed, but cyclic dependencies and decision-variable dependencies are forbidden.
3. Variables declared `real` have domain \(\mathbb{R}\). Variables declared `int` have domain \(\mathbb{Z}\). No implicit nonnegativity or finite bounds are imposed.
4. An objective function is optional unless `solve;` is requested.
5. The `subject to:` section is optional. When present, it must contain at least one constraint.
6. Valid constraint relations are `<=`, `>=`, and `=`. The frontend may recognize `<` and `>` to provide explicit semantic errors, but these relations are not valid OptiLang v1 constraints.
7. Objective and constraint expressions must be affine in decision variables.
8. All declared identifiers share one global namespace and must be unique. Constraints may be unnamed.
9. The `print` statement requires a preceding `solve;` and may reference declared decision variables or the objective.
10. Statements must appear in the order: parameters, variables, objective, constraints, solve, print.
11. Lexical, syntax, and semantic errors must be reported with source locations.
12. Feasibility, unboundedness, and optimality are solver results, not syntax errors.

## Valid domain examples

| Input | Interpretation |
|---|---|
| `var z: real;` and no constraints on `z` | `z` is a free real variable |
| `var n: int;` and no constraints on `n` | `n` is an unrestricted integer variable |
| `var x: real;` plus `x >= 0;` | Lower-bounded at 0 |
| `var y: real;` plus `y <= 10;` | Upper-bounded at 10 |
| `var t: real;` plus `t >= -5; t <= 7;` | Bounded between -5 and 7 |
| `var a: real; var b: real;` plus `a+b <= 10;` | Joint restriction; neither gets an independent bound from this alone |

## Explicitly not part of v1

`z in R;`, variable declaration bounds (`var x: int >= 0;`), arrays, summations, functions, and division.
