#include "core/glpk_solver.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace optilang;

bool near(double a, double b) { return std::abs(a - b) < 1e-7; }

int main() {
    // LP with unrestricted real variables, bounded by constraints.
    {
        LinearIR model;
        model.solveRequested = true;
        model.variables = {{"x", VariableType::Real, {}, {}},
                           {"y", VariableType::Real, {}, {}}};
        model.objective = IRObjective{"profit", ObjectiveSense::Maximize,
                                       {{"x",3.0},{"y",5.0}}, 0.0};
        model.constraints = {{"capacity", {{"x",1},{"y",1}}, Relation::LessEqual, 100},
                             {"x_nonneg", {{"x",1}}, Relation::GreaterEqual, 0},
                             {"y_nonneg", {{"y",1}}, Relation::GreaterEqual, 0}};
        auto result = solveWithGlpk(model);
        assert(result.status == SolveStatus::Optimal);
        assert(near(result.objectiveValue, 500));
        assert(near(result.variableValues.at("x"), 0));
        assert(near(result.variableValues.at("y"), 100));
        std::cout << "PASS: bounded LP\n";
    }
    // Unrestricted integer domain with a fractional upper bound from a row.
    {
        LinearIR model;
        model.solveRequested = true;
        model.variables = {{"n", VariableType::Integer, {}, {}}};
        model.objective = IRObjective{"obj", ObjectiveSense::Maximize,
                                       {{"n",1}}, 0};
        model.constraints = {{"limit", {{"n",1}}, Relation::LessEqual, 2.7},
                             {"min", {{"n",1}}, Relation::GreaterEqual, 0}};
        auto result = solveWithGlpk(model);
        assert(result.status == SolveStatus::Optimal);
        assert(result.usedIntegerOptimizer);
        assert(near(result.variableValues.at("n"), 2));
        std::cout << "PASS: integer optimization\n";
    }
    // Implicit nonnegativity would make this infeasible. x is free by default.
    {
        LinearIR model;
        model.solveRequested = true;
        model.variables = {{"x", VariableType::Real, {}, {}}};
        model.objective = IRObjective{"obj", ObjectiveSense::Maximize,
                                       {{"x",-1}}, 0};
        model.constraints = {{"min", {{"x",1}}, Relation::GreaterEqual, -5},
                             {"max", {{"x",1}}, Relation::LessEqual, -2}};
        auto result = solveWithGlpk(model);
        assert(result.status == SolveStatus::Optimal);
        assert(near(result.variableValues.at("x"), -5));
        std::cout << "PASS: negative real variable\n";
    }
    // Contradictory rows -> infeasible LP.
    {
        LinearIR model;
        model.solveRequested = true;
        model.variables = {{"x", VariableType::Real, {}, {}}};
        model.objective = IRObjective{"obj", ObjectiveSense::Maximize,
                                       {{"x",1}}, 0};
        model.constraints = {{"low", {{"x",1}}, Relation::GreaterEqual, 2},
                             {"high", {{"x",1}}, Relation::LessEqual, 1}};
        auto result = solveWithGlpk(model);
        assert(result.status == SolveStatus::Infeasible);
        std::cout << "PASS: infeasible LP\n";
    }
    // No rows: maximize an unrestricted real variable -> unbounded.
    {
        LinearIR model;
        model.solveRequested = true;
        model.variables = {{"x", VariableType::Real, {}, {}}};
        model.objective = IRObjective{"obj", ObjectiveSense::Maximize,
                                       {{"x",1}}, 0};
        auto result = solveWithGlpk(model);
        assert(result.status == SolveStatus::Unbounded);
        std::cout << "PASS: unbounded LP with zero rows\n";
    }
    std::cout << "All GLPK solver tests passed.\n";
}

