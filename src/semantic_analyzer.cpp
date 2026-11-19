#include "core/semantic_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>

namespace optilang {
namespace {

[[noreturn]] void fail(SourceSpan span, const std::string& message) { // Error at line ...
    throw SemanticError("semantic error at " +
                        std::to_string(span.begin.line) + ":" +
                        std::to_string(span.begin.column) + ": " + message);
}

double finite(double value, SourceSpan span) { // handles finite / infinite / numerical validity cases 
    if (!std::isfinite(value)) {
        fail(span, "non-finite number or arithmetic overflow");
    }
    return value;


    // finite(INFINITY, span) -->  Throws SemanticError
}

LinearExpr scale(LinearExpr expression, double factor, SourceSpan span) {   // Scale by a factor
    expression.constant = finite(expression.constant * factor, span);
    for (auto it = expression.coefficients.begin();
         it != expression.coefficients.end();) {
        const double value = finite(it->second * factor, span);
        if (value == 0.0) {
            it = expression.coefficients.erase(it);
        } else {
            it->second = value;
            ++it;
        }
    }
    return expression;
}


// Mutates destination: destination += factor * source.
void addScaled(
    LinearExpr& destination,
    const LinearExpr& source,
    double factor,
    SourceSpan span
) {
    // Step 1: Multiply source by factor
    LinearExpr scaled = scale(source, factor, span);

    // Step 2: Add constants
    destination.constant = finite(
        destination.constant + scaled.constant, span
    );

    // Step 3: Add coefficients
    for (const auto& [name, coefficient] : scaled.coefficients) {    // std::unordered_map<std::string, double> coefficients;

        double& value = destination.coefficients[name];

        value = finite(value + coefficient, span);

        if (value == 0.0) {
            destination.coefficients.erase(name);  // If variable is absent we assign 0.0 to it;
        }
    }
}

bool approximatelyEqual(double a, double b) {   // Compares the floating points numbers, returns true is they are both approximately equal;
    return std::abs(a - b) <=
           1e-10 * std::max({1.0, std::abs(a), std::abs(b)});
}

}  // namespace



SemanticAnalyzer::SemanticAnalyzer(const SymbolTable& symbols)  // Constructor
    : symbols_(symbols) {}


void SemanticAnalyzer::analyze(const Program& program) const {
    checkDeclarations(program);
    checkParameters(program);
    checkObjective(program);
    checkConstraints(program);
    checkSolve(program);
    checkPrint(program);
}


void SemanticAnalyzer::checkVariable(const std::string& name,
                                     SourceSpan span) const {
    const Symbol* symbol = symbols_.find(name);
    if (!symbol) {  // symbole == nullptr, i.e. Variable does not exist;
        fail(span, "undeclared identifier '" + name + "'");
    }
    if (!std::holds_alternative<VariableSymbol>(symbol->data)) {  // symbol is not a Variable Symbole, it's either Param Symbole/ Constraint Symbol or ...;
        fail(span, "'" + name + "' is not a decision variable");
    }
}

}


void SemanticAnalyzer::checkDeclarations(const Program& program) const { // CHecks that all the declarations declared in the AST are consistent with the symbole table 
    std::unordered_set<std::string> declared;

    // The check makes us impose the same validation logic for different types of symbols from the symbole_table;
    auto check = [&](const std::string& name, SourceSpan span,
                     auto expectedTag) {  // expectedTag captures the type of the Epression
        using Expected = decltype(expectedTag);  // Determines the a C++ Expression type in Compile time

        // This is handles the empty name
        if (name.empty()) fail(span, "declaration has an empty name");

        // This if handles the duplicate declarations
        if (!declared.insert(name).second) {
            // std::unordered_set::insert() returns a pair: std::pair<iterator, bool>
            // true: The identifier was not already present and was inserted;
            // false: The identifier already existed;
            fail(span, "duplicate declaration of '" + name + "'");
        }

        // This if checks wether the symbol exists
        const Symbol* symbol = symbols_.find(name);
        if (!symbol) {
            fail(span, "missing symbol-table entry for '" + name + "'");
        }

        // This if checks the type of the symbole: VariableSymbol / ParameterSymbol / ConstraintSymbol / ObjectiveSymbol
        if (!std::holds_alternative<Expected>(symbol->data)) { // using Expected = decltype(expectedTag)  vs  (symbole-->data);
            fail(span, "wrong symbol-table kind for '" + name + "'");
        }
    };


    for (const auto& p : program.parameters)
        check(p.name, p.span, ParameterSymbol{});

    // The {} in ParameterSymbol{} constructs a temporary value used only to tell the generic lambda which type to expect.

    for (const auto& v : program.variables) {
        check(v.name, v.span, VariableSymbol{});
        // We check the variable type: INT / REAL?
        const auto& symbol = std::get<VariableSymbol>(symbols_.find(v.name)->data);
        if (symbol.type != v.type) {
            fail(v.span, "variable type mismatch for '" + v.name + "'");
        }
    }

    // Objective is optional until a solve statement is requested.
    if (program.objective) {
        check(program.objective->name, program.objective->span,
              ObjectiveSymbol{});
    }

    for (const auto& c : program.constraints) {
      if (!c.name.empty())
        check(c.name, c.span, ConstraintSymbol{});
    }
}

void SemanticAnalyzer::checkParameters(const Program& program) const {

    // Valid:
        // param a = 3;
        // param b = 2*a + 1;
        // param c = b + 5;
    // Invalid: 
        // param a = b + 1;
        // param b = a + 2;
    // Problem ==> Cyclic dependency

    std::unordered_map<std::string, const ParameterDecl*> parameters;
    for (const auto& declaration : program.parameters)
        parameters.emplace(declaration.name, &declaration);

    // Detect cyclic parameter definitions and prohibit decision variables
    // in parameter initializers (even e.g. x - x).
    std::unordered_map<std::string, int> state;  // 0=new, 1=active, 2=done
    std::function<void(const std::string&)> visit;
    visit = [&](const std::string& name) {
        if (state[name] == 2) return;
        const ParameterDecl& parameter = *parameters.at(name);
        if (state[name] == 1)
            fail(parameter.span, "cyclic definition of parameter '" + name + "'");
        state[name] = 1;
        if (!parameter.value)
            fail(parameter.span, "missing value for parameter '" + name + "'");

        std::function<void(const Expr&)> checkReferences;
        checkReferences = [&](const Expr& expr) {
            std::visit([&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, NameExpr>) {
                    const Symbol* symbol = symbols_.find(node.name);
                    if (!symbol) fail(expr.span, "undeclared identifier '" + node.name + "'");
                    if (!std::holds_alternative<ParameterSymbol>(symbol->data))
                        fail(expr.span, "parameter '" + name +
                             "' cannot depend on non-parameter '" + node.name + "'");
                    if (parameters.count(node.name)) visit(node.name);
                } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                    if (!node.operand) fail(expr.span, "missing unary operand");
                    checkReferences(*node.operand);
                } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                    if (!node.left || !node.right)
                        fail(expr.span, "missing binary operand");
                    checkReferences(*node.left);
                    checkReferences(*node.right);
                }
            }, expr.node);
        };
        checkReferences(*parameter.value);
        state[name] = 2;
    };

    for (const auto& p : program.parameters) visit(p.name);

    // Check that the symbol table's precomputed parameter values agree with
    // the constant expressions in the AST.
    for (const auto& p : program.parameters) {
        const LinearExpr evaluated = linearize(*p.value);
        if (!evaluated.coefficients.empty())
            fail(p.span, "parameter '" + p.name + "' is not constant");
        const double stored = std::get<ParameterSymbol>(symbols_.find(p.name)->data).value;
        finite(stored, p.span);
        if (!approximatelyEqual(stored, evaluated.constant)) {
            fail(p.span, "precomputed value of parameter '" + p.name +
                 "' does not match its declaration");
        }
    }
}

LinearExpr SemanticAnalyzer::linearize(const Expr& expr) const {
    return std::visit([&](const auto& node) -> LinearExpr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, NumberExpr>) {
            return LinearExpr{{}, finite(node.value, expr.span)};
        } else if constexpr (std::is_same_v<T, NameExpr>) {
            const Symbol* symbol = symbols_.find(node.name);
            if (!symbol) fail(expr.span, "undeclared identifier '" + node.name + "'");
            if (const auto* param = std::get_if<ParameterSymbol>(&symbol->data)) {
                return LinearExpr{{}, finite(param->value, expr.span)};
            }
            checkVariable(node.name, expr.span);
            return LinearExpr{{{node.name, 1.0}}, 0.0};
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (!node.operand) fail(expr.span, "missing unary operand");
            LinearExpr operand = linearize(*node.operand);
            if (node.op == UnaryOp::Minus) return scale(std::move(operand), -1.0, expr.span);
            return operand;  // Unary +
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            if (!node.left || !node.right)
                fail(expr.span, "missing binary operand");
            LinearExpr left = linearize(*node.left);
            LinearExpr right = linearize(*node.right);

            switch (node.op) {
                case BinaryOp::Add:
                    addScaled(left, right, +1.0, expr.span);
                    return left;
                case BinaryOp::Subtract:
                    addScaled(left, right, -1.0, expr.span);
                    return left;
                case BinaryOp::Multiply:
                    if (!left.coefficients.empty() && !right.coefficients.empty())
                        fail(expr.span, "nonlinear multiplication: two expressions "
                                        "depend on decision variables");
                    if (left.coefficients.empty())
                        return scale(std::move(right), left.constant, expr.span);
                    return scale(std::move(left), right.constant, expr.span);
            }
            fail(expr.span, "unknown binary operator");
        }
    }, expr.node);
}

void SemanticAnalyzer::checkObjective(const Program& program) const {
    // Validate the objective if supplied; it is not mandatory for declarations.
    if (!program.objective) return;
    const ObjectiveDecl& objective = *program.objective;
    if (!objective.expression)
        fail(objective.span, "objective has no expression");
    (void)linearize(*objective.expression);  // Throws on nonlinear expressions.
}

void SemanticAnalyzer::checkConstraints(const Program& program) const {
    for (const ConstraintDecl& constraint : program.constraints) {
        if (!constraint.left || !constraint.right)
            fail(constraint.span, "constraint '" + constraint.name +
                                  "' has a missing side");
        if (constraint.relation == Relation::Less ||
            constraint.relation == Relation::Greater) {
            fail(constraint.span, "constraint '" + constraint.name +
                 "' uses < or >; LP/MILP requires <=, >=, or =");
        }
        // Both sides must be affine. Later, the IR builder can compute LHS-RHS.
        (void)linearize(*constraint.left);
        (void)linearize(*constraint.right);
    }
}

void SemanticAnalyzer::checkSolve(const Program& program) const {
    // A model may declare variables and constraints without being solved.
    // Once solve is requested, an objective declaration is required.
    if (program.solve && !program.objective) {
        fail(*program.solve, "cannot solve: no objective function declared");
    }
}

void SemanticAnalyzer::checkPrint(const Program& program) const {
    if (!program.print) return;
    for (const std::string& name : program.print->names) {
        const Symbol* symbol = symbols_.find(name);
        if (!symbol)
            fail(program.print->span, "cannot print undeclared identifier '" + name + "'");
        if (std::holds_alternative<ConstraintSymbol>(symbol->data))
            fail(program.print->span, "constraint '" + name +
                                        "' is not a printable numeric value");
    }
}

}  // namespace optilang
