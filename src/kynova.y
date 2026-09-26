/*
 * kynova.y — Analizador sintáctico de Kynova 1.0 "Rapture Edition".
 *
 * Verifica que la secuencia de tokens respete la gramática EBNF del
 * contrato y, si es válida, construye el AST.
 *
 * La precedencia se codifica en la propia gramática (expresion -> termino
 * -> factor), igual que en la EBNF: * y / ligan más fuerte que + y -, y
 * la recursión por la izquierda da asociatividad de izquierda a derecha.
 * Los relacionales solo aparecen en "condicion", por eso no se encadenan.
 */

%code requires {
#include "ast.h"
}

%code provides {
extern AstNode *parse_result;
extern int syntax_errors;
}

%code {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(void);
void yyerror(const char *msg);

extern const char *kyn_filename;
extern YYLTYPE prev_tok_loc;
extern int prev_tok_kind;

AstNode *parse_result = NULL;
int syntax_errors = 0;
}

%define parse.error custom
%locations

%union {
    char    *str;
    AstNode *node;
    DataType type;
    OpKind   op;
}

%token SPLICE "splice"
%token ADAM   "adam"
%token EVE    "eve"
%token RADIO  "radio"
%token DIARY  "diary"
%token ATLAS  "atlas"
%token RYAN   "ryan"
%token VITA   "vita"
%token LE "<=" GE ">=" EQ "==" NE "!="
%token <str> ID INT_LIT REAL_LIT

%type <node> lista_instrucciones instruccion declaracion asignacion
%type <node> entrada salida condicional ciclo bloque
%type <node> condicion expresion termino factor
%type <type> tipo
%type <op>   relop

%destructor { free($$); } <str>
%destructor { ast_free($$); } <node>

%start programa

%%

programa
    : lista_instrucciones                 { parse_result = ast_program($1); }
    ;

/* EBNF: lista_instrucciones ::= instruccion lista_instrucciones | ε
   Se escribe con recursión por la izquierda (equivalente y preferida en Bison). */
lista_instrucciones
    : %empty                              { $$ = ast_block(@$.first_line, @$.first_column); }
    | lista_instrucciones instruccion     { $$ = $1; if ($2) ast_block_append($$, $2); }
    ;

instruccion
    : declaracion
    | asignacion
    | entrada
    | salida
    | condicional
    | ciclo
    /* Recuperación de errores: descartar hasta ';' o hasta un bloque completo
       para poder reportar más de un error por archivo. */
    | error ';'                           { $$ = NULL; yyerrok; }
    | error bloque                        { $$ = NULL; ast_free($2); yyerrok; }
    ;

declaracion
    : SPLICE tipo ID ';'                  { $$ = ast_decl($2, $3, NULL, @3.first_line, @3.first_column); }
    | SPLICE tipo ID '=' expresion ';'    { $$ = ast_decl($2, $3, $5, @3.first_line, @3.first_column); }
    ;

asignacion
    : ID '=' expresion ';'                { $$ = ast_assign($1, $3, @1.first_line, @1.first_column); }
    ;

entrada
    : RADIO '(' ID ')' ';'                { $$ = ast_read($3, @3.first_line, @3.first_column); }
    ;

salida
    : DIARY '(' expresion ')' ';'         { $$ = ast_write($3, @1.first_line, @1.first_column); }
    ;

condicional
    : ATLAS '(' condicion ')' bloque RYAN bloque
                                          { $$ = ast_if($3, $5, $7, @1.first_line, @1.first_column); }
    ;

ciclo
    : VITA '(' condicion ')' bloque       { $$ = ast_while($3, $5, @1.first_line, @1.first_column); }
    ;

bloque
    : '{' lista_instrucciones '}'         { $$ = $2; $$->line = @1.first_line; $$->col = @1.first_column; }
    ;

tipo
    : ADAM                                { $$ = TYPE_ADAM; }
    | EVE                                 { $$ = TYPE_EVE; }
    ;

condicion
    : expresion relop expresion           { $$ = ast_relop($2, $1, $3, @2.first_line, @2.first_column); }
    | error                               { $$ = NULL; }
    ;

relop
    : '<'                                 { $$ = OP_LT; }
    | '>'                                 { $$ = OP_GT; }
    | LE                                  { $$ = OP_LE; }
    | GE                                  { $$ = OP_GE; }
    | EQ                                  { $$ = OP_EQ; }
    | NE                                  { $$ = OP_NE; }
    ;

expresion
    : termino
    | expresion '+' termino               { $$ = ast_binop(OP_ADD, $1, $3, @2.first_line, @2.first_column); }
    | expresion '-' termino               { $$ = ast_binop(OP_SUB, $1, $3, @2.first_line, @2.first_column); }
    ;

termino
    : factor
    | termino '*' factor                  { $$ = ast_binop(OP_MUL, $1, $3, @2.first_line, @2.first_column); }
    | termino '/' factor                  { $$ = ast_binop(OP_DIV, $1, $3, @2.first_line, @2.first_column); }
    ;

factor
    : INT_LIT                             { $$ = ast_int($1, @1.first_line, @1.first_column); }
    | REAL_LIT                            { $$ = ast_real($1, @1.first_line, @1.first_column); }
    | ID                                  { $$ = ast_id($1, @1.first_line, @1.first_column); }
    | '(' expresion ')'                   { $$ = $2; }
    ;

%%

static const char *symbol_display(yysymbol_kind_t sym)
{
    switch (sym) {
    case YYSYMBOL_YYEOF:    return "fin de archivo";
    case YYSYMBOL_ID:       return "identificador";
    case YYSYMBOL_INT_LIT:  return "número entero";
    case YYSYMBOL_REAL_LIT: return "número real";
    case YYSYMBOL_SPLICE:   return "'splice'";
    case YYSYMBOL_ADAM:     return "'adam'";
    case YYSYMBOL_EVE:      return "'eve'";
    case YYSYMBOL_RADIO:    return "'radio'";
    case YYSYMBOL_DIARY:    return "'diary'";
    case YYSYMBOL_ATLAS:    return "'atlas'";
    case YYSYMBOL_RYAN:     return "'ryan'";
    case YYSYMBOL_VITA:     return "'vita'";
    case YYSYMBOL_LE:       return "'<='";
    case YYSYMBOL_GE:       return "'>='";
    case YYSYMBOL_EQ:       return "'=='";
    case YYSYMBOL_NE:       return "'!='";
    default:                return yysymbol_name(sym);
    }
}

static int expects(const yysymbol_kind_t *expected, int n, yysymbol_kind_t sym)
{
    for (int i = 0; i < n; i++)
        if (expected[i] == sym)
            return 1;
    return 0;
}

static int expects_char(const yysymbol_kind_t *expected, int n, const char *name)
{
    for (int i = 0; i < n; i++)
        if (strcmp(yysymbol_name(expected[i]), name) == 0)
            return 1;
    return 0;
}

static int yyreport_syntax_error(const yypcontext_t *ctx)
{
    const YYLTYPE *loc = yypcontext_location(ctx);
    yysymbol_kind_t lookahead = yypcontext_token(ctx);
    yysymbol_kind_t expected[YYNTOKENS];
    int n = yypcontext_expected_tokens(ctx, expected, YYNTOKENS);

    syntax_errors++;
    fprintf(stderr, "%s:%d:%d: error sintáctico", kyn_filename, loc->first_line, loc->first_column);
    if (lookahead != YYSYMBOL_YYEMPTY)
        fprintf(stderr, ": %s inesperado", symbol_display(lookahead));
    if (n > 0 && n <= 6) {
        fprintf(stderr, ", se esperaba ");
        for (int i = 0; i < n; i++) {
            if (i > 0)
                fprintf(stderr, i == n - 1 ? " o " : ", ");
            fprintf(stderr, "%s", symbol_display(expected[i]));
        }
    }
    fputc('\n', stderr);

    if (expects(expected, n, YYSYMBOL_RYAN)) {
        fprintf(stderr, "  nota: en Kynova 1.0 todo 'atlas' debe llevar su bloque 'ryan'.\n");
    } else if (expects_char(expected, n, "';'") && prev_tok_loc.last_line > 0) {
        fprintf(stderr, "  nota: ¿falta ';' al final de la instrucción de la línea %d?\n",
                prev_tok_loc.last_line);
    } else if (expects_char(expected, n, "'('") && (prev_tok_kind == ATLAS || prev_tok_kind == VITA)) {
        fprintf(stderr, "  nota: la condición de '%s' debe ir entre paréntesis.\n",
                prev_tok_kind == ATLAS ? "atlas" : "vita");
    }
    return 0;
}

void yyerror(const char *msg)
{
    syntax_errors++;
    fprintf(stderr, "%s: error sintáctico: %s\n", kyn_filename, msg);
}
