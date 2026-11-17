#pragma once

#include "core/ast.hpp"
#include "core/symbol_table.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace optilang {

// An affine expression: constant + sum(coefficients[name] * name).
// Only decision variables appear in coefficients; parameters are constants.
struct LinearExpr { // represents the a_1x_1+a_2x_2+....+a_nx_n+b expression
    std::unordered_map<std::string, double> coefficients; // ai
    double constant = 0.0; // b
};

class SemanticError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const SymbolTable& symbols);

    // Validate declarations, parameter values, objective, constraints, and print.
    void analyze(const Program& program) const;

    // Convert an AST expression into an affine expression.
    // Throws SemanticError for unknown names and nonlinear products.
    [[nodiscard]] LinearExpr linearize(const Expr& expr) const;

private:
    const SymbolTable& symbols_;

    void checkVariable(const std::string& name, SourceSpan span) const;
    void checkDeclarations(const Program& program) const;
    void checkParameters(const Program& program) const;
    void checkObjective(const Program& program) const;
    void checkConstraints(const Program& program) const;
    void checkSolve(const Program& program) const;
    void checkPrint(const Program& program) const;
};

}  // namespace optilang
