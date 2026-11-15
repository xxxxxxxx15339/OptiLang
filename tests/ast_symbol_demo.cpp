#include "core/ast.hpp"
#include "core/symbol_table.hpp"

#include <cassert>
#include <iostream>
#include <variant>

int main() {
    using namespace optilang;

    Program program;
    program.parameters.push_back(ParameterDecl{"profitA", number(30), {}});
    // Variable declarations do not carry bounds.
    program.variables.push_back(VariableDecl{"x", VariableType::Integer, {}});
    program.variables.push_back(VariableDecl{"y", VariableType::Real, {}});
    program.variables.push_back(VariableDecl{"z", VariableType::Real, {}});
    program.objective = ObjectiveDecl{
        ObjectiveSense::Maximize, "totalProfit",
        binary(BinaryOp::Add,
               binary(BinaryOp::Multiply, name("profitA"), name("x")),
               binary(BinaryOp::Multiply, number(5), name("y"))),
        {}};
    // Bounds are expressed as ordinary constraints in the AST.
    program.constraints.push_back(ConstraintDecl{
        "minX", name("x"), Relation::GreaterEqual, number(0), {}});
    program.constraints.push_back(ConstraintDecl{
        "maxX", name("x"), Relation::LessEqual, number(40), {}});
    program.constraints.push_back(ConstraintDecl{
        "capacity", binary(BinaryOp::Add, name("x"), name("y")),
        Relation::LessEqual, number(100), {}});
    program.solve = SourceSpan{};
    program.print = PrintStmt{{"x", "y", "z", "totalProfit"}, {}};

    assert(program.variables.size() == 3);
    assert(program.constraints.size() == 3);
    assert(std::holds_alternative<BinaryExpr>(program.objective->expression->node));

    SymbolTable table;
    assert(table.insert("profitA", Symbol{ParameterSymbol{30}, {}}));
    assert(table.insert("x", Symbol{VariableSymbol{VariableType::Integer, std::nullopt, std::nullopt}, {}}));
    assert(table.insert("y", Symbol{VariableSymbol{VariableType::Real, std::nullopt, std::nullopt}, {}}));
    assert(table.insert("z", Symbol{VariableSymbol{VariableType::Real, std::nullopt, std::nullopt}, {}}));
    assert(table.insert("totalProfit", Symbol{ObjectiveSymbol{}, {}}));
    assert(table.insert("minX", Symbol{ConstraintSymbol{}, {}}));
    assert(table.insert("maxX", Symbol{ConstraintSymbol{}, {}}));
    assert(table.insert("capacity", Symbol{ConstraintSymbol{}, {}}));
    assert(!table.insert("x", Symbol{VariableSymbol{VariableType::Integer, {}, {}}, {}}));
    assert(!table.contains("unknown"));

    // A declaration alone does not add a finite lower or upper bound.
    const Symbol* zSymbol = table.find("z");
    assert(zSymbol != nullptr);
    const auto& z = std::get<VariableSymbol>(zSymbol->data);
    assert(!z.lowerBound.has_value());
    assert(!z.upperBound.has_value());

    // Later, semantic lowering extracts bounds from the subject-to constraints.
    Symbol* xSymbol = table.findMutable("x");
    assert(xSymbol != nullptr);
    auto& x = std::get<VariableSymbol>(xSymbol->data);
    x.lowerBound = 0;
    x.upperBound = 40;
    assert(x.lowerBound.value() == 0);
    assert(x.upperBound.value() == 40);

    // y remains free of individual bounds despite appearing in x+y <= 100.
    const auto& y = std::get<VariableSymbol>(table.find("y")->data);
    assert(!y.lowerBound.has_value() && !y.upperBound.has_value());

    std::cout << "Updated Step 1/2 AST and symbol table smoke test passed.\n";
}
