
#include <iostream>
#include <cstdio>
#include <memory>

#include "core/ast.hpp"
#include "core/semantic_analyzer.hpp"
#include "core/symbol_table_builder.hpp"
#include "core/linear_ir.hpp"

using namespace optilang;

extern FILE* yyin;
extern std::unique_ptr<Program> parsedProgram;

int yyparse();

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cerr << "Usage: parser file.opt\n";
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if (!yyin) {
        std::cerr << "Cannot open file\n";
        return 1;
    }

    int result = yyparse();
    fclose(yyin);

    if (result == 0 && parsedProgram) {
        std::cout << "Parsing successful!\n";

        std::cout << "Parameters: "
                  << parsedProgram->parameters.size()
                  << "\n";

        std::cout << "Variables: "
                  << parsedProgram->variables.size()
                  << "\n";

        std::cout << "Constraints: "
                  << parsedProgram->constraints.size()
                  << "\n";

        return 0;
    }

    std::cerr << "Parsing failed!\n";
    return 1;
}
