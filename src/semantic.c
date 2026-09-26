#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>

static int errors;
static const char *fname;
static SymbolTable *table;

static void report(const AstNode *n, const char *kind, const char *fmt, va_list ap)
{
    fprintf(stderr, "%s:%d:%d: %s: ", fname, n->line, n->col, kind);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

static void sem_error(const AstNode *n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    report(n, "error semántico", fmt, ap);
    va_end(ap);
    errors++;
}

static void sem_warning(const AstNode *n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    report(n, "advertencia", fmt, ap);
    va_end(ap);
}

static Symbol *resolve(const AstNode *n)
{
    Symbol *s = symtab_lookup(table, n->text);
    if (!s)
        sem_error(n, "la variable '%s' no ha sido declarada (o está fuera de su alcance)", n->text);
    return s;
}

static DataType check_expr(AstNode *e)
{
    switch (e->kind) {
    case NODE_INT:
        e->type = TYPE_ADAM;
        break;
    case NODE_REAL:
        e->type = TYPE_EVE;
        break;
    case NODE_ID: {
        Symbol *s = resolve(e);
        if (s) {
            s->uses++;
            e->type = s->type;
        }
        break;
    }
    case NODE_BINOP: {
        DataType l = check_expr(e->child[0]);
        DataType r = check_expr(e->child[1]);
        const AstNode *rhs = e->child[1];
        if (e->op == OP_DIV && (rhs->kind == NODE_INT || rhs->kind == NODE_REAL) && rhs->dval == 0.0)
            sem_warning(e, "división entre cero");
        if (l == TYPE_UNKNOWN || r == TYPE_UNKNOWN)
            e->type = TYPE_UNKNOWN;
        else
            e->type = (l == TYPE_EVE || r == TYPE_EVE) ? TYPE_EVE : TYPE_ADAM;
        break;
    }
    default:
        break;
    }
    return e->type;
}

static void check_condition(AstNode *c)
{
    check_expr(c->child[0]);
    check_expr(c->child[1]);
}

static void check_assignable(const AstNode *at, const char *name, DataType target, DataType value)
{
    if (target == TYPE_ADAM && value == TYPE_EVE)
        sem_error(at, "tipos incompatibles: no se puede asignar un valor eve a '%s' (adam) "
                      "sin perder la parte decimal", name);
}

static void check_block(AstNode *block);

static void check_stmt(AstNode *s)
{
    switch (s->kind) {
    case NODE_DECL: {
        /* El valor inicial se evalúa antes de registrar la variable:
           "splice adam x = x + 1;" usa una x aún no declarada. */
        DataType value = s->child[0] ? check_expr(s->child[0]) : TYPE_UNKNOWN;
        Symbol *prev = symtab_lookup_current(table, s->text);
        if (prev) {
            sem_error(s, "la variable '%s' ya fue declarada en este alcance (línea %d)",
                      s->text, prev->line);
        } else {
            symtab_insert(table, s->text, s->type, s->line, s->col);
        }
        if (s->child[0])
            check_assignable(s, s->text, s->type, value);
        break;
    }
    case NODE_ASSIGN: {
        Symbol *sym = resolve(s);
        DataType value = check_expr(s->child[0]);
        if (sym)
            check_assignable(s, s->text, sym->type, value);
        break;
    }
    case NODE_READ: {
        Symbol *sym = resolve(s);
        if (sym)
            sym->uses++;
        break;
    }
    case NODE_WRITE:
        check_expr(s->child[0]);
        break;
    case NODE_IF:
        check_condition(s->child[0]);
        check_block(s->child[1]);
        check_block(s->child[2]);
        break;
    case NODE_WHILE:
        check_condition(s->child[0]);
        check_block(s->child[1]);
        break;
    default:
        break;
    }
}

static void check_block(AstNode *block)
{
    symtab_enter_scope(table);
    for (int i = 0; i < block->count; i++)
        check_stmt(block->items[i]);
    symtab_exit_scope(table);
}

int semantic_check(AstNode *program, SymbolTable *st, const char *filename)
{
    errors = 0;
    fname = filename;
    table = st;

    for (int i = 0; i < program->count; i++)
        check_stmt(program->items[i]);

    for (int i = 0; i < st->count && errors == 0; i++) {
        const Symbol *s = st->syms[i];
        if (s->uses == 0) {
            AstNode at = { .line = s->line, .col = s->col };
            sem_warning(&at, "la variable '%s' se declaró pero nunca se usa", s->name);
        }
    }
    return errors;
}
