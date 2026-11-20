
%option noyywrap noinput nounput

%{
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "parser.tab.hpp"

// Current position in the input file.
static int currentLine = 1;
static int currentColumn = 1;

// Called before every matched rule, including whitespace.
static void trackLocation(const char* text, int length) {
    yylloc.first_line = currentLine;
    yylloc.first_column = currentColumn;

    for (int i = 0; i < length; ++i) {
        if (text[i] == '\n') {
            ++currentLine;
            currentColumn = 1;
        } else {
            ++currentColumn;
        }
    }

    yylloc.last_line = currentLine;
    yylloc.last_column = currentColumn - 1;
}

#define YY_USER_ACTION trackLocation(yytext, yyleng);
%}

DIGIT       [0-9]
NUMBER_RE   ({DIGIT}+(\.{DIGIT}*)?|\.[0-9]+)([eE][+-]?{DIGIT}+)?
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
    yylval.number = std::strtod(yytext, nullptr);
    return NUMBER;
}

{ID} {
    yylval.identifier = strdup(yytext);
    return IDENTIFIER;
}

"#"[^\n]* {
    // Ignore comments.
}

[ \t\r\n]+ {
    // Ignore whitespace.
}

<<EOF>> {
    yylloc.first_line = currentLine;
    yylloc.first_column = currentColumn;
    yylloc.last_line = currentLine;
    yylloc.last_column = currentColumn;
    return 0;
}

. {
    std::fprintf(
        stderr,
        "Lexical error at %d:%d: '%s'\n",
        yylloc.first_line,
        yylloc.first_column,
        yytext
    );
    return INVALID;
}

%%

