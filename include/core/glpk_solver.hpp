

#pragma once

#include "core/linear_ir.hpp"

#include <iosfwd>
#include <string>
#include <unordered_map>

namespace optilang {

enum class SolveStatus {
    Optimal,
    Feasible,
    Infeasible,
    Unbounded,
    Unknown
};

struct SolveResult {
    SolveStatus status = SolveStatus::Unknown;

    bool usedIntegerOptimizer = false;

    // Meaningful only when a solution exists.
    double objectiveValue = 0.0;

    std::unordered_map<std::string, double> variableValues;

    // Explanation of solver limitations or failures.
    std::string message;
};

[[nodiscard]] SolveResult solveWithGlpk(
    const LinearIR& model
);

[[nodiscard]] const char* solveStatusName(
    SolveStatus status
);

void printSolveResult(
    const LinearIR& model,
    const SolveResult& result,
    std::ostream& out
);

} // namespace optilang
