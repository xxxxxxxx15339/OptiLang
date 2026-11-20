
#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>

#include "core/ast.hpp"
#include "core/symbol_table.hpp"
#include "core/symbol_table_builder.hpp"
#include "core/semantic_analyzer.hpp"
#include "core/linear_ir.hpp"

using namespace optilang;

// Defined in parser.y
extern FILE* yyin;
extern std::unique_ptr<Program> parsedProgram;

int yyparse();

int main(int argc, char** argv) {

    // -----------------------------------------
    // STEP 1: Validate command-line arguments
    // -----------------------------------------
    if (argc != 2) {
        std::cerr << "Usage: optilang file.opt\n";
        return 1;
    }

    // -----------------------------------------
    // STEP 2: Open source file
    // -----------------------------------------
    yyin = std::fopen(argv[1], "r");

    if (!yyin) {
        std::cerr << "Error: Cannot open file: "
                  << argv[1] << '\n';
        return 1;
    }

    try {

        // -----------------------------------------
        // STEP 3: Lexical + Syntax Analysis
        // Person 1: Flex / Bison
        // -----------------------------------------
        int parseResult = yyparse();

        std::fclose(yyin);
        yyin = nullptr;

        if (parseResult != 0 || !parsedProgram) {
            std::cerr << "Parsing failed!\n";
            return 1;
        }

        std::cout << "[1/4] Parsing successful!\n";

        // -----------------------------------------
        // STEP 4: Build Symbol Table
        // Person 2: Fix 4
        // -----------------------------------------
        SymbolTable symbols =
            buildSymbolTable(*parsedProgram);

        std::cout << "[2/4] Symbol Table built!\n";

        // -----------------------------------------
        // STEP 5: Semantic Analysis
        // Person 2
        // -----------------------------------------
        SemanticAnalyzer analyzer(symbols);

        analyzer.analyze(*parsedProgram);

        std::cout << "[3/4] Semantic analysis successful!\n";

        // -----------------------------------------
        // STEP 6: Generate Intermediate Representation
        // Person 2: Fix 5
        // -----------------------------------------
        LinearIR model = buildLinearIR(
            *parsedProgram,
            symbols,
            analyzer
        );

        std::cout << "[4/4] Linear IR generated!\n";

        // -----------------------------------------
        // STEP 7: Display compilation summary
        // -----------------------------------------
        std::cout << "\n===== OptiLang Compilation =====\n";

        std::cout << "Variables: "
                  << model.variables.size() << '\n';

        std::cout << "Constraints: "
                  << model.constraints.size() << '\n';

        std::cout << "Objective: "
                  << (model.objective ? "Declared" : "None")
                  << '\n';

        std::cout << "Solve requested: "
                  << (model.solveRequested ? "Yes" : "No")
                  << '\n';

        // -----------------------------------------
        // STEP 8: Solver execution
        // Person 2: Future GLPK Backend
        // -----------------------------------------
        if (model.solveRequested) {
            std::cout
                << "\nModel ready for optimization.\n"
                << "GLPK backend not yet implemented.\n";
        } else {
            std::cout
                << "\nCompilation completed.\n"
                << "No solve statement requested.\n";
        }

        return 0;

    } catch (const SemanticError& e) {

        // Semantic or IR validation errors
        std::cerr << "\nSemantic error: "
                  << e.what() << '\n';

    } catch (const std::exception& e) {

        // Other C++ errors
        std::cerr << "\nCompiler error: "
                  << e.what() << '\n';
    }

    // Close source file if an exception occurred during parsing.
    if (yyin) {
        std::fclose(yyin);
        yyin = nullptr;
    }

    return 1;
}

