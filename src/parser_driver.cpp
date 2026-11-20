
#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>

#include "core/ast.hpp"
#include "core/symbol_table.hpp"
#include "core/symbol_table_builder.hpp"
#include "core/semantic_analyzer.hpp"
#include "core/linear_ir.hpp"
#include "core/glpk_solver.hpp"

using namespace optilang;

// Defined in parser.y
extern FILE* yyin;
extern std::unique_ptr<Program> parsedProgram;

int yyparse();

int main(int argc, char** argv) {

    // =========================================
    // STEP 1: Command-line arguments
    // =========================================
    if (argc != 2) {
        std::cerr << "Usage: optilang file.opt\n";
        return 1;
    }

    // =========================================
    // STEP 2: Open the .opt source file
    // =========================================
    yyin = std::fopen(argv[1], "r");

    if (!yyin) {
        std::cerr << "Error: Cannot open file: "
                  << argv[1] << '\n';
        return 1;
    }

    try {

        // =========================================
        // STEP 3: Lexical and Syntax Analysis
        // Person 1: Flex + Bison
        // =========================================
        int parseResult = yyparse();

        std::fclose(yyin);
        yyin = nullptr;

        if (parseResult != 0 || !parsedProgram) {
            std::cerr << "Parsing failed!\n";
            return 1;
        }

        std::cout << "[1/5] Parsing successful!\n";

        // =========================================
        // STEP 4: Build Symbol Table
        // Person 2
        // =========================================
        SymbolTable symbols =
            buildSymbolTable(*parsedProgram);

        std::cout << "[2/5] Symbol Table built!\n";

        // =========================================
        // STEP 5: Semantic Analysis
        // Person 2
        // =========================================
        SemanticAnalyzer analyzer(symbols);

        analyzer.analyze(*parsedProgram);

        std::cout
            << "[3/5] Semantic analysis successful!\n";

        // =========================================
        // STEP 6: Intermediate Representation
        // Person 2
        // =========================================
        LinearIR model = buildLinearIR(
            *parsedProgram,
            symbols,
            analyzer
        );

        std::cout << "[4/5] Linear IR generated!\n";

        // =========================================
        // STEP 7: Compilation summary
        // =========================================
        std::cout
            << "\n===== OptiLang Compilation =====\n";

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

        // =========================================
        // STEP 8: GLPK Solver
        // Person 2
        // =========================================
        if (model.solveRequested) {

            std::cout
                << "\n[5/5] Running GLPK optimizer...\n";

            // Convert IR into a GLPK problem,
            // solve it and retrieve the solution.
            SolveResult solution =
                solveWithGlpk(model);

            // Display objective value and
            // selected decision variables.
            printSolveResult(
                model,
                solution,
                std::cout
            );

            // Return 2 when optimization does
            // not produce a proven optimum.
            if (solution.status != SolveStatus::Optimal) {
                return 2;
            }

            std::cout
                << "\nOptimization completed successfully!\n";

        } else {

            // Valid declaration-only programs
            // do not require an objective or solver.
            std::cout
                << "\nCompilation completed.\n"
                << "No solve statement requested.\n";
        }

        return 0;

    } catch (const SemanticError& e) {

        // Invalid declarations, parameter
        // dependencies or nonlinear expressions.
        std::cerr
            << "\nSemantic error: "
            << e.what() << '\n';

    } catch (const std::exception& e) {

        // GLPK or other runtime errors.
        std::cerr
            << "\nCompiler error: "
            << e.what() << '\n';
    }

    // Close the source file if an exception
    // occurred before it was closed.
    if (yyin) {
        std::fclose(yyin);
        yyin = nullptr;
    }

    return 1;
}

