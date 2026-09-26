#include "symtab.h"

#include <stdlib.h>
#include <string.h>

static void *xrealloc(void *p, size_t size)
{
    p = realloc(p, size);
    if (!p) {
        fprintf(stderr, "kynova: memoria insuficiente\n");
        exit(2);
    }
    return p;
}

void symtab_init(SymbolTable *st)
{
    memset(st, 0, sizeof *st);
    symtab_enter_scope(st);
}

void symtab_free(SymbolTable *st)
{
    for (int i = 0; i < st->count; i++) {
        free(st->syms[i]->name);
        free(st->syms[i]);
    }
    free(st->syms);
    free(st->scopes);
    memset(st, 0, sizeof *st);
}

static int current_scope(const SymbolTable *st)
{
    return st->scopes[st->depth - 1];
}

void symtab_enter_scope(SymbolTable *st)
{
    if (st->depth == st->scopes_cap) {
        st->scopes_cap = st->scopes_cap ? st->scopes_cap * 2 : 8;
        st->scopes = xrealloc(st->scopes, (size_t)st->scopes_cap * sizeof *st->scopes);
    }
    st->scopes[st->depth++] = st->next_scope_id++;
}

void symtab_exit_scope(SymbolTable *st)
{
    int scope = current_scope(st);
    for (int i = 0; i < st->count; i++)
        if (st->syms[i]->scope_id == scope)
            st->syms[i]->active = 0;
    st->depth--;
}

Symbol *symtab_lookup(SymbolTable *st, const char *name)
{
    for (int i = st->count - 1; i >= 0; i--)
        if (st->syms[i]->active && strcmp(st->syms[i]->name, name) == 0)
            return st->syms[i];
    return NULL;
}

Symbol *symtab_lookup_current(SymbolTable *st, const char *name)
{
    int scope = current_scope(st);
    for (int i = st->count - 1; i >= 0; i--)
        if (st->syms[i]->scope_id == scope && strcmp(st->syms[i]->name, name) == 0)
            return st->syms[i];
    return NULL;
}

Symbol *symtab_insert(SymbolTable *st, const char *name, DataType type, int line, int col)
{
    if (st->count == st->cap) {
        st->cap = st->cap ? st->cap * 2 : 16;
        st->syms = xrealloc(st->syms, (size_t)st->cap * sizeof *st->syms);
    }
    Symbol *s = xrealloc(NULL, sizeof *s);
    s->name = xrealloc(NULL, strlen(name) + 1);
    strcpy(s->name, name);
    s->type = type;
    s->scope_id = current_scope(st);
    s->scope_level = st->depth - 1;
    s->line = line;
    s->col = col;
    s->uses = 0;
    s->active = 1;
    st->syms[st->count++] = s;
    return s;
}

void symtab_print(FILE *out, const SymbolTable *st)
{
    if (st->count == 0) {
        fprintf(out, "(sin variables declaradas)\n");
        return;
    }
    fprintf(out, "%-3s  %-16s %-5s %-7s %-20s %-8s %s\n",
            "#", "Nombre", "Tipo", "C11", "Alcance", "Línea", "Usos");
    fprintf(out, "---  ---------------- ----- ------- -------------------- ------- ----\n");
    for (int i = 0; i < st->count; i++) {
        const Symbol *s = st->syms[i];
        char scope[32];
        if (s->scope_level == 0)
            snprintf(scope, sizeof scope, "global");
        else
            snprintf(scope, sizeof scope, "bloque #%d (nivel %d)", s->scope_id, s->scope_level);
        fprintf(out, "%-3d  %-16s %-5s %-7s %-20s %-7d %d\n",
                i + 1, s->name, type_name(s->type), type_c11_name(s->type),
                scope, s->line, s->uses);
    }
}
