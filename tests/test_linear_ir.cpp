#include "core/linear_ir.hpp"
#include "core/symbol_table_builder.hpp"

#include <cassert>
#include <iostream>
#include <variant>

using namespace optilang;

int main() {
    Program program;
    program.parameters.push_back({"capacity", number(20), {}});
    program.variables.push_back({"x", VariableType::Real, {}});
    program.variables.push_back({"y", VariableType::Integer, {}});
    program.objective = ObjectiveDecl{
        ObjectiveSense::Maximize, "profit",
        binary(BinaryOp::Add,
            binary(BinaryOp::Add,
                binary(BinaryOp::Multiply, number(3), name("x")),
                binary(BinaryOp::Multiply, number(5), name("y"))),
            number(2)), {}};
    program.constraints.push_back({"c1",
        binary(BinaryOp::Add,
            binary(BinaryOp::Add,
                binary(BinaryOp::Multiply, number(2), name("x")), name("y")),
            number(4)), Relation::LessEqual, name("capacity"), {}});
    program.constraints.push_back({"c2", name("x"), Relation::GreaterEqual,
                                   number(0), {}});
    program.constraints.push_back({"c3",
        binary(BinaryOp::Add,
            binary(BinaryOp::Multiply, number(2), name("x")), number(3)),
        Relation::LessEqual,
        binary(BinaryOp::Add, name("y"), number(8)), {}});
    program.solve = SourceSpan{};
    program.print = PrintStmt{{"x", "y", "profit"}, {}};

    SymbolTable symbols = buildSymbolTable(program);
    SemanticAnalyzer analyzer(symbols);
    LinearIR ir = buildLinearIR(program, symbols, analyzer);
    assert(ir.variables.size() == 2);
    assert(ir.variables[0].name == "x");
    assert(ir.variables[0].type == VariableType::Real);
    assert(!ir.variables[0].lowerBound && !ir.variables[0].upperBound);
    assert(ir.variables[1].type == VariableType::Integer);
    assert(ir.solveRequested);
    assert(ir.printNames && ir.printNames->size() == 3);
    assert(ir.objective);
    assert(ir.objective->sense == ObjectiveSense::Maximize);
    assert(ir.objective->coefficients.at("x") == 3);
    assert(ir.objective->coefficients.at("y") == 5);
    assert(ir.objective->constant == 2);
    assert(ir.constraints.size() == 3);
    const auto& c1 = ir.constraints[0];
    assert(c1.coefficients.at("x") == 2);
    assert(c1.coefficients.at("y") == 1);
    assert(c1.rhs == 16);
    assert(c1.relation == Relation::LessEqual);
    const auto& c2 = ir.constraints[1];
    assert(c2.coefficients.at("x") == 1 && c2.rhs == 0);
    assert(c2.relation == Relation::GreaterEqual);
    const auto& c3 = ir.constraints[2];
    assert(c3.coefficients.at("x") == 2);
    assert(c3.coefficients.at("y") == -1);
    assert(c3.rhs == 5);

    Program declarationsOnly;
    declarationsOnly.variables.push_back({"z", VariableType::Real, {}});
    declarationsOnly.constraints.push_back({"", name("z"),
        Relation::LessEqual, number(10), {}});
    SymbolTable symbols2 = buildSymbolTable(declarationsOnly);
    SemanticAnalyzer analyzer2(symbols2);
    LinearIR ir2 = buildLinearIR(declarationsOnly, symbols2, analyzer2);
    assert(!ir2.objective && !ir2.solveRequested);
    assert(ir2.constraints[0].name.empty());

    std::cout << "PASS: Linear IR normalization, metadata, objective-free models\n";
}

