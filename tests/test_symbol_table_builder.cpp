
#include "core/symbol_table_builder.hpp"
#include "core/semantic_analyzer.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <variant>

using namespace optilang;

void expectError(const Program& program, const std::string& fragment) {
    try {
        (void)buildSymbolTable(program);
    } catch (const SemanticError& e) {
        assert(std::string(e.what()).find(fragment) != std::string::npos);
        return;
    }
    assert(false && "Expected a SemanticError");
}

int main() {
    {
        Program p;
        // Forward reference b -> a: supported by the builder's DFS.
        p.parameters.push_back({"b", binary(BinaryOp::Add,
            binary(BinaryOp::Multiply, number(2), name("a")), number(1)), {}});
        p.parameters.push_back({"a", number(3), {}});
        p.variables.push_back({"x", VariableType::Real, {}});
        p.variables.push_back({"n", VariableType::Integer, {}});
        p.objective = ObjectiveDecl{ObjectiveSense::Maximize, "profit",
            binary(BinaryOp::Multiply, name("b"), name("x")), {}};
        p.constraints.push_back({"", name("x"), Relation::GreaterEqual, number(0), {}});
        p.constraints.push_back({"c1", name("x"), Relation::LessEqual, number(10), {}});

        SymbolTable symbols = buildSymbolTable(p);
        assert(std::get<ParameterSymbol>(symbols.find("a")->data).value == 3.0);
        assert(std::get<ParameterSymbol>(symbols.find("b")->data).value == 7.0);
        const auto& x = std::get<VariableSymbol>(symbols.find("x")->data);
        assert(x.type == VariableType::Real && !x.lowerBound && !x.upperBound);
        assert(std::holds_alternative<ObjectiveSymbol>(symbols.find("profit")->data));
        assert(std::holds_alternative<ConstraintSymbol>(symbols.find("c1")->data));
        assert(!symbols.contains(""));
        std::cout << "PASS: declarations, values, forward references, unnamed constraints\n";
    }
    {
        Program p;
        p.variables.push_back({"x", VariableType::Real, {}});
        p.variables.push_back({"x", VariableType::Integer, {}});
        expectError(p, "duplicate declaration");
        std::cout << "PASS: duplicate rejection\n";
    }
    {
        Program p;
        p.parameters.push_back({"a", name("b"), {}});
        p.parameters.push_back({"b", name("a"), {}});
        expectError(p, "cyclic definition");
        std::cout << "PASS: cyclic parameter rejection\n";
    }
    {
        Program p;
        p.parameters.push_back({"a", binary(BinaryOp::Subtract,name("x"),name("x")), {}});
        p.variables.push_back({"x", VariableType::Real, {}});
        expectError(p, "non-parameter");
        std::cout << "PASS: decision-variable dependency rejection\n";
    }
    {
        Program p;
        p.parameters.push_back({"a", name("unknown"), {}});
        expectError(p, "undeclared identifier");
        std::cout << "PASS: undeclared parameter reference rejection\n";
    }
    {
        Program p;
        p.parameters.push_back({"a", number(INFINITY), {}});
        expectError(p, "non-finite");
        std::cout << "PASS: non-finite numeric rejection\n";
    }
    {
        Program p;
        p.variables.push_back({"x", VariableType::Real, {}});
        SymbolTable symbols = buildSymbolTable(p);
        assert(symbols.contains("x"));
        std::cout << "PASS: objective-free declaration program\n";
    }
}

