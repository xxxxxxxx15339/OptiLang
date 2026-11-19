
#pragma once

#include "core/ast.hpp"
#include "core/semantic_analyzer.hpp"
#include "core/symbol_table.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace optilang {

// A decision variable and its domain.
struct IRVariable {
    std::string name;
    VariableType type;
    std::optional<double> lowerBound;
    std::optional<double> upperBound;
};

// The objective: maximize or minimize c'x + constant.
struct IRObjective {
    std::string name;
    ObjectiveSense sense;

    std::unordered_map<std::string, double> coefficients;
    double constant = 0.0;
};

// A normalized constraint:
// sum(coefficients[name] * name) relation rhs
struct IRConstraint {
    std::string name;
    std::unordered_map<std::string, double> coefficients;

    Relation relation;
    double rhs = 0.0;

    SourceSpan span{};
};

// The complete intermediate optimization model.
struct LinearIR {
    std::vector<IRVariable> variables;
    std::optional<IRObjective> objective;
    std::vector<IRConstraint> constraints;

    bool solveRequested = false;

    std::optional<std::vector<std::string>> printNames;
};

// Convert a validated AST to normalized linear IR.
// The Symbol Table must already be populated.
[[nodiscard]] LinearIR buildLinearIR(
    const Program& program,
    const SymbolTable& symbols,
    const SemanticAnalyzer& analyzer
);

}  // namespace optilang

