
%option noyywrap yylineno noinput nounput

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.hpp"
%}

DIGIT       [0-9]
NUMBER_RE   {DIGIT}+(\.{DIGIT}+)?([eE][+-]?{DIGIT}+)?
ID          [a-zA-Z_][a-zA-Z0-9_]*

%%

"param"       { return PARAM; }
"var"         { return VAR; }
"int"         { return INT; }
"real"        { return REAL; }

"maximize"    { return MAXIMIZE; }
"minimize"    { return MINIMIZE; }

"subject"     { return SUBJECT; }
"to"          { return TO; }

"solve"       { return SOLVE; }
"print"       { return PRINT; }

"<="          { return LESS_EQUAL; }
">="          { return GREATER_EQUAL; }
"<"           { return LESS; }
">"           { return GREATER; }

"="           { return EQUAL; }
"+"           { return PLUS; }
"-"           { return MINUS; }
"*"           { return MULTIPLY; }

":"           { return COLON; }
";"           { return SEMICOLON; }
","           { return COMMA; }
"("           { return LPAREN; }
")"           { return RPAREN; }

{NUMBER_RE} {
    yylval.number = atof(yytext);
    return NUMBER;
}

{ID} {
    yylval.identifier = strdup(yytext);
    return IDENTIFIER;
}

"#"[^\n]*      { /* Ignore comments */ }
[ \t\r\n]+     { /* Ignore whitespace */ }

. {
    fprintf(stderr,
        "Lexical error at line %d: '%s'\n",
        yylineno, yytext);
    return INVALID;
}

%%
