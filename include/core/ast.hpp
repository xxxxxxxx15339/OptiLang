#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace optilang {

struct SourcePos {
    int line = 1;
    int column = 1;
};

struct SourceSpan {
    SourcePos begin{};
    SourcePos end{};
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class UnaryOp { Plus, Minus };
enum class BinaryOp { Add, Subtract, Multiply };

struct NumberExpr {
    double value;
};

struct NameExpr {
    std::string name;
};

struct UnaryExpr {
    UnaryOp op;
    ExprPtr operand;
};

struct BinaryExpr {
    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

struct Expr {
    using Node = std::variant<NumberExpr, NameExpr, UnaryExpr, BinaryExpr>;
    Node node;
    SourceSpan span{};

    explicit Expr(Node node, SourceSpan span = {})
        : node(std::move(node)), span(span) {}
};

// Helper constructors simplify grammar actions and independent unit tests.
inline ExprPtr number(double value, SourceSpan span = {}) {
    return std::make_unique<Expr>(Expr::Node{NumberExpr{value}}, span);
}

inline ExprPtr name(std::string identifier, SourceSpan span = {}) {
    return std::make_unique<Expr>(
        Expr::Node{NameExpr{std::move(identifier)}}, span);
}

inline ExprPtr unary(UnaryOp op, ExprPtr operand, SourceSpan span = {}) {
    return std::make_unique<Expr>(
        Expr::Node{UnaryExpr{op, std::move(operand)}}, span);
}

inline ExprPtr binary(BinaryOp op, ExprPtr left, ExprPtr right,
                      SourceSpan span = {}) {
    return std::make_unique<Expr>(
        Expr::Node{BinaryExpr{op, std::move(left), std::move(right)}}, span);
}

enum class VariableType { Integer, Real };
enum class ObjectiveSense { Maximize, Minimize };
enum class Relation { LessEqual, GreaterEqual, Equal, Greater, Less };

struct ParameterDecl {
    std::string name;
    ExprPtr value;  // Must be compile-time constant after semantic analysis.
    SourceSpan span{};
};

struct VariableDecl {
    std::string name;
    VariableType type;
    // No bound in a declaration: all restrictions belong in subject to.
    SourceSpan span{};
};

struct ObjectiveDecl {
    ObjectiveSense sense;
    std::string name;
    ExprPtr expression;
    SourceSpan span{};
};

struct ConstraintDecl {
    std::string name;
    ExprPtr left;
    Relation relation;
    ExprPtr right;
    SourceSpan span{};
};

struct PrintStmt {
    std::vector<std::string> names;
    SourceSpan span{};
};

struct Program {
    std::vector<ParameterDecl> parameters;
    std::vector<VariableDecl> variables;
    std::optional<ObjectiveDecl> objective;  // Exactly one required by grammar.
    std::vector<ConstraintDecl> constraints;
    std::optional<SourceSpan> solve;          // Presence represents "solve;".
    std::optional<PrintStmt> print;
};

}  // namespace optilang
