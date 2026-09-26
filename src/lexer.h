#ifndef KYNOVA_LEXER_H
#define KYNOVA_LEXER_H

#include <stdio.h>

#include "kynova.tab.h"

extern FILE *yyin;
extern char *yytext;

int  yylex(void);
void yyrestart(FILE *input);
int  yylex_destroy(void);

/* Reinicia posición y estado del lexer (para volver a leer el archivo). */
void lexer_reset(void);

/* Nombre del archivo fuente, usado en todos los mensajes de error. */
extern const char *kyn_filename;

extern int lex_errors;
/* Si es distinto de 0, los errores léxicos se cuentan pero no se imprimen. */
extern int lex_silent;

/* Token anterior al lookahead actual; lo usa el parser para dar pistas. */
extern YYLTYPE prev_tok_loc;
extern int prev_tok_kind;

#endif
