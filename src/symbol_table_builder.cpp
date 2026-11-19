
#include "core/symbol_table_builder.hpp"
#include "core/semantic_analyzer.hpp"  // SemanticError

#include <cmath>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace optilang {
namespace {

[[noreturn]] void fail(SourceSpan span, const std::string& message) {
    throw SemanticError("semantic error at " +
        std::to_string(span.begin.line) + ":" +
        std::to_string(span.begin.column) + ": " + message);
}

void registerSymbol(SymbolTable& symbols, const std::string& identifier,
                    SymbolData data, SourceSpan span) {
    if (identifier.empty()) {
        fail(span, "declaration has an empty name");
    }
    if (!symbols.insert(identifier, Symbol{std::move(data), span})) {
        fail(span, "duplicate declaration of '" + identifier + "'");
    }
}

double requireFinite(double value, SourceSpan span) {
    if (!std::isfinite(value)) {
        fail(span, "non-finite number or arithmetic overflow");
    }
    return value;
}

}  // namespace

SymbolTable buildSymbolTable(const Program& program) {
    SymbolTable symbols;
    std::unordered_map<std::string, const ParameterDecl*> parameters;

    // Register ALL declarations first. Parameter values are initially placeholders.
    for (const auto& p : program.parameters) {
        registerSymbol(symbols, p.name, ParameterSymbol{0.0}, p.span);
        parameters.emplace(p.name, &p);
    }
    for (const auto& v : program.variables) {
        registerSymbol(symbols, v.name,
                       VariableSymbol{v.type, std::nullopt, std::nullopt}, v.span);
    }
    if (program.objective) {
        registerSymbol(symbols, program.objective->name,
                       ObjectiveSymbol{}, program.objective->span);
    }
    for (const auto& c : program.constraints) {
        if (!c.name.empty()) {
            registerSymbol(symbols, c.name, ConstraintSymbol{}, c.span);
        }
    }

    // Evaluate parameter definitions recursively. This permits references to
    // other parameters declared later, and catches cyclic definitions.
    std::unordered_map<std::string, int> state; // 0=unseen, 1=visiting, 2=done
    std::function<double(const std::string&)> evaluateParameter;
    std::function<double(const Expr&)> evaluateExpression;

    evaluateExpression = [&](const Expr& expr) -> double {
        const double result = std::visit([&](const auto& node) -> double {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, NumberExpr>) {
                return node.value;
            } else if constexpr (std::is_same_v<T, NameExpr>) {
                auto it = parameters.find(node.name);
                if (it == parameters.end()) {
                    if (symbols.contains(node.name)) {
                        fail(expr.span, "parameter expression cannot depend on non-parameter '" +
                                        node.name + "'");
                    }
                    fail(expr.span, "undeclared identifier '" + node.name + "'");
                }
                return evaluateParameter(node.name);
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                if (!node.operand) fail(expr.span, "missing unary operand");
                const double value = evaluateExpression(*node.operand);
                return node.op == UnaryOp::Minus ? -value : value;
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                if (!node.left || !node.right)
                    fail(expr.span, "missing binary operand");
                const double lhs = evaluateExpression(*node.left);
                const double rhs = evaluateExpression(*node.right);
                switch (node.op) {
                    case BinaryOp::Add:      return lhs + rhs;
                    case BinaryOp::Subtract: return lhs - rhs;
                    case BinaryOp::Multiply: return lhs * rhs;
                }
                fail(expr.span, "unknown binary operator");
            }
        }, expr.node);
        return requireFinite(result, expr.span);
    };

    evaluateParameter = [&](const std::string& identifier) -> double {
        const auto& declaration = *parameters.at(identifier);
        if (state[identifier] == 1)
            fail(declaration.span, "cyclic definition of parameter '" + identifier + "'");
        if (state[identifier] == 2)
            return std::get<ParameterSymbol>(symbols.find(identifier)->data).value;
        if (!declaration.value)
            fail(declaration.span, "missing value for parameter '" + identifier + "'");

        state[identifier] = 1;
        const double value = evaluateExpression(*declaration.value);
        Symbol* symbol = symbols.findMutable(identifier);
        std::get<ParameterSymbol>(symbol->data).value = value;
        state[identifier] = 2;
        return value;
    };

    for (const auto& p : program.parameters) {
        (void)evaluateParameter(p.name);
    }
    return symbols;
}

}  // namespace optilang

