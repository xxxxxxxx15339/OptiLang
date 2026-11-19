
#include "core/linear_ir.hpp"

#include <cmath>
#include <utility>

namespace optilang {
namespace {

// Report an error during IR construction.
[[noreturn]] void failIR(
    SourceSpan span,
    const std::string& message
) {
    throw SemanticError(
        "IR error at " +
        std::to_string(span.begin.line) + ":" +
        std::to_string(span.begin.column) + ": " +
        message
    );
}

// Ensure all coefficients and bounds remain finite.
double checked(double value, SourceSpan span) {
    if (!std::isfinite(value))
        failIR(span, "non-finite coefficient or bound");

    return value;
}

// Transform: Left relation Right
// into: sum(ai * xi) relation rhs
IRConstraint normalizeConstraint(
    const ConstraintDecl& constraint,
    const SemanticAnalyzer& analyzer
) {
    // Convert both AST expressions to affine expressions.
    LinearExpr left =
        analyzer.linearize(*constraint.left);

    LinearExpr right =
        analyzer.linearize(*constraint.right);

    IRConstraint row;

    row.name = constraint.name;
    row.relation = constraint.relation;
    row.span = constraint.span;

    // Start with the coefficients from the left side.
    row.coefficients = std::move(left.coefficients);

    // Subtract all variable coefficients from the right side.
    for (const auto& [name, coefficient] : right.coefficients) {
        double& value = row.coefficients[name];

        value = checked(
            value - coefficient,
            constraint.span
        );

        // Remove coefficients that cancel out.
        if (value == 0.0)
            row.coefficients.erase(name);
    }

    // Move constants to the right-hand side.
    row.rhs = checked(
        right.constant - left.constant,
        constraint.span
    );

    return row;
}

}  // namespace

LinearIR buildLinearIR(
    const Program& program,
    const SymbolTable& symbols,
    const SemanticAnalyzer& analyzer
) {
    // Check semantics before generating the IR.
    analyzer.analyze(program);

    LinearIR model;

    model.solveRequested = program.solve.has_value();

    // 1. Register variables in declaration order.
    for (const auto& declaration : program.variables) {

        const Symbol* entry =
            symbols.find(declaration.name);

        const auto* variable = entry
            ? std::get_if<VariableSymbol>(&entry->data)
            : nullptr;

        if (!variable || variable->type != declaration.type) {
            failIR(
                declaration.span,
                "invalid variable symbol '" +
                declaration.name + "'"
            );
        }

        model.variables.push_back({
            declaration.name,
            variable->type,
            variable->lowerBound,
            variable->upperBound
        });
    }

    // 2. Convert the objective, if one was declared.
    if (program.objective) {
        const auto& declaration = *program.objective;

        LinearExpr value =
            analyzer.linearize(*declaration.expression);

        model.objective = IRObjective{
            declaration.name,
            declaration.sense,
            std::move(value.coefficients),
            value.constant
        };
    }

    // 3. Normalize every constraint.
    for (const auto& declaration : program.constraints) {
        model.constraints.push_back(
            normalizeConstraint(declaration, analyzer)
        );
    }

    // 4. Preserve the identifiers requested by print.
    if (program.print) {
        model.printNames = program.print->names;
    }

    return model;
}

}  // namespace optilang

