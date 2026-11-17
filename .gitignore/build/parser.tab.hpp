/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_HPP_INCLUDED
# define YY_YY_PARSER_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 23 "src/parser.y"

    #include "core/ast.hpp"
    #include <vector>
    #include <string>

#line 55 "parser.tab.hpp"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    PARAM = 258,                   /* PARAM  */
    VAR = 259,                     /* VAR  */
    INT = 260,                     /* INT  */
    REAL = 261,                    /* REAL  */
    MAXIMIZE = 262,                /* MAXIMIZE  */
    MINIMIZE = 263,                /* MINIMIZE  */
    SUBJECT = 264,                 /* SUBJECT  */
    TO = 265,                      /* TO  */
    SOLVE = 266,                   /* SOLVE  */
    PRINT = 267,                   /* PRINT  */
    LESS = 268,                    /* LESS  */
    GREATER = 269,                 /* GREATER  */
    LESS_EQUAL = 270,              /* LESS_EQUAL  */
    GREATER_EQUAL = 271,           /* GREATER_EQUAL  */
    EQUAL = 272,                   /* EQUAL  */
    PLUS = 273,                    /* PLUS  */
    MINUS = 274,                   /* MINUS  */
    MULTIPLY = 275,                /* MULTIPLY  */
    COLON = 276,                   /* COLON  */
    SEMICOLON = 277,               /* SEMICOLON  */
    COMMA = 278,                   /* COMMA  */
    LPAREN = 279,                  /* LPAREN  */
    RPAREN = 280,                  /* RPAREN  */
    INVALID = 281,                 /* INVALID  */
    NUMBER = 282,                  /* NUMBER  */
    IDENTIFIER = 283               /* IDENTIFIER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 33 "src/parser.y"

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

#line 119 "parser.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_HPP_INCLUDED  */
