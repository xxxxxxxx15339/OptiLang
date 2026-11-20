

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

// Final AST produced by Bison.
std::unique_ptr<Program> parsedProgram;

// Flex scanner and Bison error callback.
int yylex();
void yyerror(const char* message);

// Convert a Bison location to OptiLang SourceSpan.
#define OPTI_SPAN(loc) optilang::SourceSpan{              \
    {(loc).first_line, (loc).first_column},              \
    {(loc).last_line, (loc).last_column}                  \
}
%}

/*
 * Types required in the generated parser.tab.hpp.
 * Flex includes this header to access tokens and YYSTYPE.
 */
%code requires {
    #include "core/ast.hpp"
    #include <vector>
    #include <string>
}

/* Detailed parser errors and source locations. */
%define parse.error detailed
%locations

/* ==========================================
   SEMANTIC VALUES
   ========================================== */

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

/* ==========================================
   TOKENS
   ========================================== */

/* Declarations */
%token PARAM VAR INT REAL

/* Objective */
%token MAXIMIZE MINIMIZE

/* Model sections */
%token SUBJECT TO SOLVE PRINT

/* Comparison operators */
%token LESS GREATER
%token LESS_EQUAL GREATER_EQUAL
%token EQUAL

/* Arithmetic operators */
%token PLUS MINUS MULTIPLY

/* Punctuation */
%token COLON SEMICOLON COMMA
%token LPAREN RPAREN

/* Invalid lexical token */
%token INVALID

/* Tokens with associated values */
%token <number> NUMBER
%token <identifier> IDENTIFIER

/* ==========================================
   NONTERMINAL TYPES
   ========================================== */

%type <params> parameter_list
%type <param> parameter_decl

%type <vars> variable_list
%type <var> variable_decl
%type <kind> variable_type

%type <objective> objective optional_objective

%type <constraints> constraint_list
%type <constraints> optional_constraint_section
%type <constraint> constraint
%type <kind> relation

%type <kind> optional_solve
%type <printstmt> optional_print
%type <names> identifier_list

%type <expr> expression term unary primary

/* ==========================================
   MEMORY CLEANUP ON PARSING ERRORS
   ========================================== */

%destructor { free($$); } <identifier>

%destructor { delete $$; } <expr>
%destructor { delete $$; } <param> <var>
%destructor { delete $$; } <objective> <constraint>
%destructor { delete $$; } <params> <vars>
%destructor { delete $$; } <constraints>
%destructor { delete $$; } <printstmt> <names>

%start program

%%

/* ==========================================
   1. COMPLETE PROGRAM
   ========================================== */

/*
 * Parameters: optional
 * Variables: at least one
 * Objective: optional unless solve is present
 * Constraints: optional
 * Solve: optional
 * Print: optional; semantic analyzer requires solve
 */

program:
    parameter_list
    variable_list
    optional_objective
    optional_constraint_section
    optional_solve
    optional_print
    {
        auto p = std::make_unique<Program>();

        // 1. Parameters
        p->parameters = std::move(*$1);

        // 2. Variables
        p->variables = std::move(*$2);

        // 3. Objective (optional)
        if ($3) {
            p->objective.emplace(std::move(*$3));
        }

        // 4. Constraints (possibly empty)
        p->constraints = std::move(*$4);

        // 5. Solve statement (optional)
        if ($5) {
            p->solve = OPTI_SPAN(@5);
        }

        // 6. Print statement (optional)
        if ($6) {
            p->print.emplace(std::move(*$6));
        }

        // Clean up temporary grammar objects.
        delete $1;
        delete $2;
        delete $3;
        delete $4;
        delete $6;

        // Transfer ownership of the AST.
        parsedProgram = std::move(p);
    }
;

/* ==========================================
   2. PARAMETERS
   ========================================== */

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
            OPTI_SPAN(@$)
        };

        free($2);
    }
;

/* ==========================================
   3. VARIABLES
   ========================================== */

/*
 * At least one variable declaration is required.
 * Bounds are not specified in variable declarations.
 */

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
            OPTI_SPAN(@$)
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

/* ==========================================
   4. OBJECTIVE FUNCTION
   ========================================== */

/*
 * Objective is optional during parsing.
 * SemanticAnalyzer::checkSolve() requires an
 * objective when solve; is present.
 */

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
            OPTI_SPAN(@$)
        };

        free($2);
    }
    | MINIMIZE IDENTIFIER COLON expression SEMICOLON
    {
        $$ = new ObjectiveDecl{
            ObjectiveSense::Minimize,
            std::string($2),
            ExprPtr($4),
            OPTI_SPAN(@$)
        };

        free($2);
    }
;

/* ==========================================
   5. CONSTRAINT SECTION
   ========================================== */

/*
 * A constraint section may be absent.
 *
 * When present, it must contain at least
 * one constraint.
 */

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

/*
 * Both named and unnamed constraints are supported.
 */

constraint:
    IDENTIFIER COLON expression relation expression SEMICOLON
    {
        $$ = new ConstraintDecl{
            std::string($1),
            ExprPtr($3),
            static_cast<Relation>($4),
            ExprPtr($5),
            OPTI_SPAN(@$)
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
            OPTI_SPAN(@$)
        };
    }
;

/*
 * Strict inequalities are recognized so that the
 * Semantic Analyzer can give a clear rejection.
 *
 * Valid OptiLang v1 relations:
 * <=, >=, =
 */

relation:
    LESS_EQUAL
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
    | LESS
    {
        $$ = static_cast<int>(Relation::Less);
    }
    | GREATER
    {
        $$ = static_cast<int>(Relation::Greater);
    }
;

/* ==========================================
   6. SOLVE STATEMENT
   ========================================== */

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

/* ==========================================
   7. PRINT STATEMENT
   ========================================== */

/*
 * print is syntactically optional.
 * The Semantic Analyzer requires solve; before print.
 */

optional_print:
    %empty
    {
        $$ = nullptr;
    }
    | PRINT identifier_list SEMICOLON
    {
        $$ = new PrintStmt{
            std::move(*$2),
            OPTI_SPAN(@$)
        };

        delete $2;
    }
;

identifier_list:
    IDENTIFIER
    {
        $$ = new std::vector<std::string>();

        $$->push_back(std::string($1));

        free($1);
    }
    | identifier_list COMMA IDENTIFIER
    {
        $$ = $1;

        $$->push_back(std::string($3));

        free($3);
    }
;

/* ==========================================
   8. ARITHMETIC EXPRESSIONS
   ========================================== */

/*
 * Precedence is defined by the grammar:
 *
 * expression -> addition/subtraction
 * term       -> multiplication
 * unary      -> unary plus/minus
 * primary    -> literals/names/parentheses
 */

/* ------------------------------------------
   8.1 ADDITION AND SUBTRACTION
   ------------------------------------------ */

expression:
    expression PLUS term
    {
        $$ = binary(
            BinaryOp::Add,
            ExprPtr($1),
            ExprPtr($3),
            OPTI_SPAN(@$)
        ).release();
    }
    | expression MINUS term
    {
        $$ = binary(
            BinaryOp::Subtract,
            ExprPtr($1),
            ExprPtr($3),
            OPTI_SPAN(@$)
        ).release();
    }
    | term
    {
        $$ = $1;
    }
;

/* ------------------------------------------
   8.2 MULTIPLICATION
   ------------------------------------------ */

term:
    term MULTIPLY unary
    {
        $$ = binary(
            BinaryOp::Multiply,
            ExprPtr($1),
            ExprPtr($3),
            OPTI_SPAN(@$)
        ).release();
    }
    | unary
    {
        $$ = $1;
    }
;

/* ------------------------------------------
   8.3 UNARY PLUS AND MINUS
   ------------------------------------------ */

unary:
    PLUS unary
    {
        $$ = optilang::unary(
            UnaryOp::Plus,
            ExprPtr($2),
            OPTI_SPAN(@$)
        ).release();
    }
    | MINUS unary
    {
        $$ = optilang::unary(
            UnaryOp::Minus,
            ExprPtr($2),
            OPTI_SPAN(@$)
        ).release();
    }
    | primary
    {
        $$ = $1;
    }
;

/* ------------------------------------------
   8.4 PRIMARY EXPRESSIONS
   ------------------------------------------ */

primary:
    NUMBER
    {
        $$ = number(
            $1,
            OPTI_SPAN(@1)
        ).release();
    }
    | IDENTIFIER
    {
        $$ = name(
            std::string($1),
            OPTI_SPAN(@1)
        ).release();

        free($1);
    }
    | LPAREN expression RPAREN
    {
        $$ = $2;

        // Include parentheses in the expression span.
        $$->span = OPTI_SPAN(@$);
    }
;

%%

/* ==========================================
   9. SYNTAX ERROR HANDLING
   ========================================== */

void yyerror(const char* message)
{
    std::cerr
        << "Syntax error at "
        << yylloc.first_line
        << ":"
        << yylloc.first_column
        << ": "
        << message
        << '\n';
}
