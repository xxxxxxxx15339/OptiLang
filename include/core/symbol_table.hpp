#pragma once

#include "core/ast.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace optilang {

struct ParameterSymbol {
    double value;  // Already evaluated at compile time.
};

struct VariableSymbol {
    VariableType type;
    // Both absent on declaration. Optionally inferred from normalized constraints.
    std::optional<double> lowerBound;
    std::optional<double> upperBound;
};

struct ObjectiveSymbol {};
struct ConstraintSymbol {};

using SymbolData = std::variant<ParameterSymbol, VariableSymbol,
                                ObjectiveSymbol, ConstraintSymbol>;

struct Symbol {
    SymbolData data;
    SourceSpan declaredAt{};
};

class SymbolTable {
public:
    // One global namespace for OptiLang++ v1: no name may be declared twice.
    // Returns false on a duplicate; the semantic analyzer reports diagnostics.
    [[nodiscard]] bool insert(std::string identifier, Symbol symbol) {
        return entries_.emplace(std::move(identifier), std::move(symbol)).second;
    }

    [[nodiscard]] const Symbol* find(const std::string& identifier) const {
        const auto it = entries_.find(identifier);
        return it == entries_.end() ? nullptr : &it->second;
    }

    // Used later by semantic analysis to record inferred bounds.
    [[nodiscard]] Symbol* findMutable(const std::string& identifier) {
        const auto it = entries_.find(identifier);
        return it == entries_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] bool contains(const std::string& identifier) const {
        return find(identifier) != nullptr;
    }

private:
    std::unordered_map<std::string, Symbol> entries_;
};

}  // namespace optilang
