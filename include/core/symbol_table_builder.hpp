#pragma once

#include "core/ast.hpp"
#include "core/symbol_table.hpp"

namespace optilang {

[[nodiscard]] SymbolTable buildSymbolTable(
    const Program& program
);

}  // namespace optilang
