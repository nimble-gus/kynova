/*
 * kynova.l — Analizador léxico de Kynova 1.0 "Rapture Edition".
 *
 * Agrupa el texto del archivo .kyn en tokens (reservadas, identificadores,
 * números, operadores y delimitadores) y descarta espacios y comentarios.
 * No valida significado: solo reconoce piezas.
 */

%{
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"

const char *kyn_filename = "<entrada>";
int lex_errors = 0;
int lex_silent = 0;

YYLTYPE prev_tok_loc;
int prev_tok_kind = 0;
static YYLTYPE cur_tok_loc;
static int cur_tok_kind = 0;

static int cur_line = 1;
static int cur_col = 1;
static YYLTYPE comment_start;

static void update_loc(void)
{
    yylloc.first_line = cur_line;
    yylloc.first_column = cur_col;
    for (int i = 0; i < yyleng; i++) {
        unsigned char c = (unsigned char)yytext[i];
        if (c == '\n') {
            cur_line++;
            cur_col = 1;
        } else if ((c & 0xC0) != 0x80) {
            /* Bytes de continuación UTF-8 no avanzan columna. */
            cur_col++;
        }
    }
    yylloc.last_line = cur_line;
    yylloc.last_column = cur_col > 1 ? cur_col - 1 : 1;
}

static void lex_error_at(const YYLTYPE *loc, const char *fmt, ...)
{
    lex_errors++;
    if (lex_silent)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%d:%d: error léxico: ", kyn_filename, loc->first_line, loc->first_column);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

static char *dup_text(void)
{
    char *s = strdup(yytext);
    if (!s) {
        fprintf(stderr, "kynova: memoria insuficiente\n");
        exit(2);
    }
    return s;
}

#define YY_USER_ACTION update_loc();

#define TOKEN(t)                     \
    do {                             \
        prev_tok_loc = cur_tok_loc;  \
        prev_tok_kind = cur_tok_kind;\
        cur_tok_loc = yylloc;        \
        cur_tok_kind = (t);          \
        return (t);                  \
    } while (0)
%}

%option noyywrap nounput noinput 8bit
%x COMMENT

DIGIT    [0-9]
LETTER   [a-zA-Z_]
ID       {LETTER}({LETTER}|{DIGIT})*

%%

[ \t\r\n]+              { /* espacios: se descartan */ }
"//".*                  { /* comentario de línea */ }

"/*"                    { comment_start = yylloc; BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>[^*]+          { }
<COMMENT>"*"            { }
<COMMENT><<EOF>>        {
                            lex_error_at(&comment_start, "comentario multilínea sin cerrar (falta '*/')");
                            BEGIN(INITIAL);
                            yylloc.first_line = yylloc.last_line = cur_line;
                            yylloc.first_column = yylloc.last_column = cur_col;
                            TOKEN(0);
                        }

 /* Palabras reservadas (distinguen mayúsculas: "Atlas" es un identificador). */
"splice"                { TOKEN(SPLICE); }
"adam"                  { TOKEN(ADAM); }
"eve"                   { TOKEN(EVE); }
"radio"                 { TOKEN(RADIO); }
"diary"                 { TOKEN(DIARY); }
"atlas"                 { TOKEN(ATLAS); }
"ryan"                  { TOKEN(RYAN); }
"vita"                  { TOKEN(VITA); }

 /* Operadores relacionales de dos caracteres. */
"<="                    { TOKEN(LE); }
">="                    { TOKEN(GE); }
"=="                    { TOKEN(EQ); }
"!="                    { TOKEN(NE); }

 /* Operadores y delimitadores de un carácter: el token es el propio carácter. */
[-+*/<>=(){};]          { TOKEN(yytext[0]); }

 /* Números: el real se reconoce antes que el entero. */
{DIGIT}+"."{DIGIT}+     { yylval.str = dup_text(); TOKEN(REAL_LIT); }

{DIGIT}+","{DIGIT}+     {
                            lex_error_at(&yylloc, "número real mal formado '%s': los reales usan punto decimal, no coma", yytext);
                            yylval.str = dup_text();
                            *strchr(yylval.str, ',') = '.';
                            TOKEN(REAL_LIT);
                        }

{DIGIT}+"."             {
                            lex_error_at(&yylloc, "número real mal formado '%s': falta la parte fraccionaria", yytext);
                            yylval.str = dup_text();
                            TOKEN(REAL_LIT);
                        }

{DIGIT}+{ID}            {
                            lex_error_at(&yylloc, "identificador inválido '%s': no puede iniciar con un dígito", yytext);
                            yylval.str = dup_text();
                            TOKEN(ID);
                        }

{DIGIT}+                { yylval.str = dup_text(); TOKEN(INT_LIT); }

{ID}                    { yylval.str = dup_text(); TOKEN(ID); }

[\xC0-\xFF][\x80-\xBF]* { lex_error_at(&yylloc, "carácter no reconocido '%s'", yytext); }
.                       { lex_error_at(&yylloc, "carácter no reconocido '%s'", yytext); }

<<EOF>>                 {
                            yylloc.first_line = yylloc.last_line = cur_line;
                            yylloc.first_column = yylloc.last_column = cur_col;
                            TOKEN(0);
                        }

%%

void lexer_reset(void)
{
    cur_line = 1;
    cur_col = 1;
    prev_tok_kind = cur_tok_kind = 0;
    memset(&prev_tok_loc, 0, sizeof prev_tok_loc);
    memset(&cur_tok_loc, 0, sizeof cur_tok_loc);
    BEGIN(INITIAL);
}
