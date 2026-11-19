#pragma once

#include "core/ast.hpp" 
#include "core/symbol_table.hpp"

namespace optilang {
  [[nodiscard]] SymboleTable buildSymbolTable(
      const Program& program
  );
}
