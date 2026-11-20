
%{
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/ast.hpp"

using namespace optilang;

// AST produced by the parser
std::unique_ptr<Program> parsedProgram;

int yylex();
void yyerror(const char* message);
extern int yylineno;
%}

%code requires {
    #include "core/ast.hpp"
    #include <vector>
    #include <string>
}

/* Better syntax error messages */
%define parse.error detailed

/* Semantic values */
%union {
    double number;
    char* identifier;

    optilang::Expr* expr;
    optilang::ParameterDecl* param;
    optilang::VariableDecl* var;
    optilang::ObjectiveDecl* objective;
    optilang::ConstraintDecl* constraint;
    optilang::PrintStmt* printstmt;

    std::vector<optilang::ParameterDecl>* params;
    std::vector<optilang::VariableDecl>* vars;
    std::vector<optilang::ConstraintDecl>* constraints;
    std::vector<std::string>* names;

    int kind;
}

/* Keywords */
%token PARAM VAR INT REAL
%token MAXIMIZE MINIMIZE
%token SUBJECT TO SOLVE PRINT

/* Comparisons */
%token LESS GREATER
%token LESS_EQUAL GREATER_EQUAL
%token EQUAL

/* Arithmetic */
%token PLUS MINUS MULTIPLY

/* Punctuation */
%token COLON SEMICOLON COMMA
%token LPAREN RPAREN

/* Lexical error */
%token INVALID

/* Tokens carrying values */
%token <number> NUMBER
%token <identifier> IDENTIFIER

/* Grammar value types */
%type <params> parameter_list
%type <param> parameter_decl

%type <vars> variable_list
%type <var> variable_decl
%type <kind> variable_type

%type <objective> objective optional_objective
%type <constraints> constraint_list optional_constraint_section

%type <constraint> constraint
%type <kind> relation

%type <expr> expression term unary primary

%type <kind> optional_solve
%type <printstmt> optional_print
%type <names> identifier_list

/* Cleanup on syntax errors */
%destructor { free($$); } <identifier>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <param> <var>
%destructor { delete $$; } <objective> <constraint>
%destructor { delete $$; } <params> <vars>
%destructor { delete $$; } <constraints>
%destructor { delete $$; } <printstmt> <names>

%start program

%%

/* ========================================
   1. COMPLETE PROGRAM
   ======================================== */

program:
    parameter_list
    variable_list
    optional_objective
    optional_constraint_section
    optional_solve
    optional_print
    {
        auto p = std::make_unique<Program>();

        p->parameters = std::move(*$1);
        p->variables = std::move(*$2);

        if ($3)
            p->objective.emplace(std::move(*$3));

        p->constraints = std::move(*$4);

        if ($5)
            p->solve = SourceSpan{};

        if ($6)
            p->print.emplace(std::move(*$6));

        delete $1;
        delete $2;
        delete $3;
        delete $4;
        delete $6;

        parsedProgram = std::move(p);
    }
;

/* ========================================
   2. PARAMETERS
   ======================================== */

parameter_list:
    %empty
    {
        $$ = new std::vector<ParameterDecl>();
    }
    | parameter_list parameter_decl
    {
        $$ = $1;
        $$->push_back(std::move(*$2));
        delete $2;
    }
;

parameter_decl:
    PARAM IDENTIFIER EQUAL expression SEMICOLON
    {
        $$ = new ParameterDecl{
            std::string($2),
            ExprPtr($4),
            {}
        };

        free($2);
    }
;

/* ========================================
   3. VARIABLES
   ======================================== */

variable_list:
    variable_decl
    {
        $$ = new std::vector<VariableDecl>();
        $$->push_back(std::move(*$1));
        delete $1;
    }
    | variable_list variable_decl
    {
        $$ = $1;
        $$->push_back(std::move(*$2));
        delete $2;
    }
;

variable_decl:
    VAR IDENTIFIER COLON variable_type SEMICOLON
    {
        $$ = new VariableDecl{
            std::string($2),
            static_cast<VariableType>($4),
            {}
        };

        free($2);
    }
;

variable_type:
    INT
    {
        $$ = static_cast<int>(VariableType::Integer);
    }
    | REAL
    {
        $$ = static_cast<int>(VariableType::Real);
    }
;

/* ========================================
   4. OBJECTIVE FUNCTION
   ======================================== */

optional_objective:
    %empty
    {
        $$ = nullptr;
    }
    | objective
    {
        $$ = $1;
    }
;

objective:
    MAXIMIZE IDENTIFIER COLON expression SEMICOLON
    {
        $$ = new ObjectiveDecl{
            ObjectiveSense::Maximize,
            std::string($2),
            ExprPtr($4),
            {}
        };

        free($2);
    }
    | MINIMIZE IDENTIFIER COLON expression SEMICOLON
    {
        $$ = new ObjectiveDecl{
            ObjectiveSense::Minimize,
            std::string($2),
            ExprPtr($4),
            {}
        };

        free($2);
    }
;

/* ========================================
   5. CONSTRAINTS
   ======================================== */

optional_constraint_section:
    %empty
    {
        $$ = new std::vector<ConstraintDecl>();
    }
    | SUBJECT TO COLON constraint_list
    {
        $$ = $4;
    }
;

constraint_list:
    constraint
    {
        $$ = new std::vector<ConstraintDecl>();
        $$->push_back(std::move(*$1));
        delete $1;
    }
    | constraint_list constraint
    {
        $$ = $1;
        $$->push_back(std::move(*$2));
        delete $2;
    }
;

constraint:
    IDENTIFIER COLON expression relation
    expression SEMICOLON
    {
        $$ = new ConstraintDecl{
            std::string($1),
            ExprPtr($3),
            static_cast<Relation>($4),
            ExprPtr($5),
            {}
        };

        free($1);
    }
    | expression relation expression SEMICOLON
    {
        $$ = new ConstraintDecl{
            "",
            ExprPtr($1),
            static_cast<Relation>($2),
            ExprPtr($3),
            {}
        };
    }
;

relation:
    LESS
    {
        $$ = static_cast<int>(Relation::Less);
    }
    | GREATER
    {
        $$ = static_cast<int>(Relation::Greater);
    }
    | LESS_EQUAL
    {
        $$ = static_cast<int>(Relation::LessEqual);
    }
    | GREATER_EQUAL
    {
        $$ = static_cast<int>(Relation::GreaterEqual);
    }
    | EQUAL
    {
        $$ = static_cast<int>(Relation::Equal);
    }
;

/* ========================================
   6. SOLVE
   ======================================== */

optional_solve:
    %empty
    {
        $$ = 0;
    }
    | SOLVE SEMICOLON
    {
        $$ = 1;
    }
;

/* ========================================
   7. PRINT
   ======================================== */

optional_print:
    %empty
    {
        $$ = nullptr;
    }
    | PRINT identifier_list SEMICOLON
    {
        $$ = new PrintStmt{
            std::move(*$2),
            {}
        };

        delete $2;
    }
;

identifier_list:
    IDENTIFIER
    {
        $$ = new std::vector<std::string>();
        $$->push_back($1);
        free($1);
    }
    | identifier_list COMMA IDENTIFIER
    {
        $$ = $1;
        $$->push_back($3);
        free($3);
    }
;

/* ========================================
   8. ARITHMETIC EXPRESSIONS
   ======================================== */

expression:
    expression PLUS term
    {
        $$ = binary(
            BinaryOp::Add,
            ExprPtr($1),
            ExprPtr($3)
        ).release();
    }
    | expression MINUS term
    {
        $$ = binary(
            BinaryOp::Subtract,
            ExprPtr($1),
            ExprPtr($3)
        ).release();
    }
    | term
    {
        $$ = $1;
    }
;

term:
    term MULTIPLY unary
    {
        $$ = binary(
            BinaryOp::Multiply,
            ExprPtr($1),
            ExprPtr($3)
        ).release();
    }
    | unary
    {
        $$ = $1;
    }
;

unary:
    PLUS unary
    {
        $$ = optilang::unary(
            UnaryOp::Plus,
            ExprPtr($2)
        ).release();
    }
    | MINUS unary
    {
        $$ = optilang::unary(
            UnaryOp::Minus,
            ExprPtr($2)
        ).release();
    }
    | primary
    {
        $$ = $1;
    }
;

primary:
    NUMBER
    {
        $$ = number($1).release();
    }
    | IDENTIFIER
    {
        $$ = name($1).release();
        free($1);
    }
    | LPAREN expression RPAREN
    {
        $$ = $2;
    }
;

%%

/* ========================================
   9. SYNTAX ERROR HANDLING
   ======================================== */

void yyerror(const char* message)
{
    std::cerr
        << "Syntax error at line "
        << yylineno
        << ": "
        << message
        << std::endl;
}
