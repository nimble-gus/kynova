#ifndef KYNOVA_SYMTAB_H
#define KYNOVA_SYMTAB_H

#include <stdio.h>

#include "ast.h"

/* Cada variable registrada en el "Gene Bank". */
typedef struct Symbol {
    char    *name;
    DataType type;
    int      scope_id;     /* 0 = global; cada bloque { } abre un alcance nuevo */
    int      scope_level;  /* profundidad de anidamiento */
    int      line;
    int      col;
    int      uses;
    int      active;       /* 0 cuando su alcance ya se cerró */
} Symbol;

typedef struct {
    Symbol **syms;         /* todas las variables, en orden de declaración */
    int      count;
    int      cap;
    int     *scopes;       /* pila de alcances abiertos (ids) */
    int      depth;
    int      scopes_cap;
    int      next_scope_id;
} SymbolTable;

void    symtab_init(SymbolTable *st);
void    symtab_free(SymbolTable *st);
void    symtab_enter_scope(SymbolTable *st);
void    symtab_exit_scope(SymbolTable *st);

/* Busca en el alcance actual y en los que lo contienen (el más interno primero). */
Symbol *symtab_lookup(SymbolTable *st, const char *name);
/* Busca solo en el alcance actual. */
Symbol *symtab_lookup_current(SymbolTable *st, const char *name);
Symbol *symtab_insert(SymbolTable *st, const char *name, DataType type, int line, int col);

void    symtab_print(FILE *out, const SymbolTable *st);

#endif
