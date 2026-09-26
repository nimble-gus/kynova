#include "ast.h"

#include <stdlib.h>
#include <string.h>

static void *xcalloc(size_t n, size_t size)
{
    void *p = calloc(n, size);
    if (!p) {
        fprintf(stderr, "kynova: memoria insuficiente\n");
        exit(2);
    }
    return p;
}

static AstNode *new_node(NodeKind kind, int line, int col)
{
    AstNode *n = xcalloc(1, sizeof *n);
    n->kind = kind;
    n->line = line;
    n->col = col;
    return n;
}

AstNode *ast_block(int line, int col)
{
    return new_node(NODE_BLOCK, line, col);
}

void ast_block_append(AstNode *block, AstNode *stmt)
{
    if (block->count == block->cap) {
        int cap = block->cap ? block->cap * 2 : 8;
        AstNode **items = realloc(block->items, (size_t)cap * sizeof *items);
        if (!items) {
            fprintf(stderr, "kynova: memoria insuficiente\n");
            exit(2);
        }
        block->items = items;
        block->cap = cap;
    }
    block->items[block->count++] = stmt;
}

AstNode *ast_program(AstNode *block)
{
    block->kind = NODE_PROGRAM;
    return block;
}

AstNode *ast_decl(DataType type, char *name, AstNode *init, int line, int col)
{
    AstNode *n = new_node(NODE_DECL, line, col);
    n->type = type;
    n->text = name;
    n->child[0] = init;
    return n;
}

AstNode *ast_assign(char *name, AstNode *expr, int line, int col)
{
    AstNode *n = new_node(NODE_ASSIGN, line, col);
    n->text = name;
    n->child[0] = expr;
    return n;
}

AstNode *ast_read(char *name, int line, int col)
{
    AstNode *n = new_node(NODE_READ, line, col);
    n->text = name;
    return n;
}

AstNode *ast_write(AstNode *expr, int line, int col)
{
    AstNode *n = new_node(NODE_WRITE, line, col);
    n->child[0] = expr;
    return n;
}

AstNode *ast_if(AstNode *cond, AstNode *then_block, AstNode *else_block, int line, int col)
{
    AstNode *n = new_node(NODE_IF, line, col);
    n->child[0] = cond;
    n->child[1] = then_block;
    n->child[2] = else_block;
    return n;
}

AstNode *ast_while(AstNode *cond, AstNode *body, int line, int col)
{
    AstNode *n = new_node(NODE_WHILE, line, col);
    n->child[0] = cond;
    n->child[1] = body;
    return n;
}

AstNode *ast_binop(OpKind op, AstNode *left, AstNode *right, int line, int col)
{
    AstNode *n = new_node(NODE_BINOP, line, col);
    n->op = op;
    n->child[0] = left;
    n->child[1] = right;
    return n;
}

AstNode *ast_relop(OpKind op, AstNode *left, AstNode *right, int line, int col)
{
    AstNode *n = ast_binop(op, left, right, line, col);
    n->kind = NODE_RELOP;
    return n;
}

AstNode *ast_int(char *lexeme, int line, int col)
{
    AstNode *n = new_node(NODE_INT, line, col);
    n->text = lexeme;
    n->ival = strtoll(lexeme, NULL, 10);
    n->dval = (double)n->ival;
    return n;
}

AstNode *ast_real(char *lexeme, int line, int col)
{
    AstNode *n = new_node(NODE_REAL, line, col);
    n->text = lexeme;
    n->dval = strtod(lexeme, NULL);
    return n;
}

AstNode *ast_id(char *name, int line, int col)
{
    AstNode *n = new_node(NODE_ID, line, col);
    n->text = name;
    return n;
}

void ast_free(AstNode *node)
{
    if (!node)
        return;
    for (int i = 0; i < 3; i++)
        ast_free(node->child[i]);
    for (int i = 0; i < node->count; i++)
        ast_free(node->items[i]);
    free(node->items);
    free(node->text);
    free(node);
}

const char *type_name(DataType type)
{
    switch (type) {
    case TYPE_ADAM: return "adam";
    case TYPE_EVE:  return "eve";
    default:        return "?";
    }
}

const char *type_c11_name(DataType type)
{
    switch (type) {
    case TYPE_ADAM: return "int";
    case TYPE_EVE:  return "double";
    default:        return "?";
    }
}

const char *op_symbol(OpKind op)
{
    switch (op) {
    case OP_ADD: return "+";
    case OP_SUB: return "-";
    case OP_MUL: return "*";
    case OP_DIV: return "/";
    case OP_LT:  return "<";
    case OP_GT:  return ">";
    case OP_LE:  return "<=";
    case OP_GE:  return ">=";
    case OP_EQ:  return "==";
    case OP_NE:  return "!=";
    }
    return "?";
}

/* ------------------------------------------------------------------ */
/* Impresión del árbol                                                 */
/* ------------------------------------------------------------------ */

static void print_expr_type(FILE *out, const AstNode *n)
{
    if (n->type != TYPE_UNKNOWN)
        fprintf(out, " : %s", type_name(n->type));
}

static void print_description(FILE *out, const AstNode *n)
{
    switch (n->kind) {
    case NODE_PROGRAM:
        fprintf(out, "Programa (%d %s)", n->count, n->count == 1 ? "instrucción" : "instrucciones");
        break;
    case NODE_BLOCK:
        fprintf(out, "Bloque (%d %s)", n->count, n->count == 1 ? "instrucción" : "instrucciones");
        break;
    case NODE_DECL:
        fprintf(out, "Declaración splice %s %s  [línea %d]", type_name(n->type), n->text, n->line);
        break;
    case NODE_ASSIGN:
        fprintf(out, "Asignación %s =  [línea %d]", n->text, n->line);
        break;
    case NODE_READ:
        fprintf(out, "Entrada radio(%s)  [línea %d]", n->text, n->line);
        break;
    case NODE_WRITE:
        fprintf(out, "Salida diary  [línea %d]", n->line);
        break;
    case NODE_IF:
        fprintf(out, "Condicional atlas/ryan  [línea %d]", n->line);
        break;
    case NODE_WHILE:
        fprintf(out, "Ciclo vita  [línea %d]", n->line);
        break;
    case NODE_BINOP:
        fprintf(out, "Operación '%s'", op_symbol(n->op));
        print_expr_type(out, n);
        break;
    case NODE_RELOP:
        fprintf(out, "Comparación '%s'", op_symbol(n->op));
        break;
    case NODE_INT:
        fprintf(out, "Entero %s", n->text);
        print_expr_type(out, n);
        break;
    case NODE_REAL:
        fprintf(out, "Real %s", n->text);
        print_expr_type(out, n);
        break;
    case NODE_ID:
        fprintf(out, "Identificador %s", n->text);
        print_expr_type(out, n);
        break;
    }
}

typedef struct {
    const AstNode *node;
    const char *label;
} Child;

static int collect_children(const AstNode *n, Child *out, int max)
{
    int k = 0;
    switch (n->kind) {
    case NODE_PROGRAM:
    case NODE_BLOCK:
        for (int i = 0; i < n->count && k < max; i++)
            out[k++] = (Child){ n->items[i], "" };
        break;
    case NODE_IF:
        out[k++] = (Child){ n->child[0], "cond: " };
        out[k++] = (Child){ n->child[1], "atlas: " };
        out[k++] = (Child){ n->child[2], "ryan: " };
        break;
    case NODE_WHILE:
        out[k++] = (Child){ n->child[0], "cond: " };
        out[k++] = (Child){ n->child[1], "cuerpo: " };
        break;
    case NODE_DECL:
        if (n->child[0])
            out[k++] = (Child){ n->child[0], "valor inicial: " };
        break;
    case NODE_ASSIGN:
    case NODE_WRITE:
        out[k++] = (Child){ n->child[0], "" };
        break;
    case NODE_BINOP:
    case NODE_RELOP:
        out[k++] = (Child){ n->child[0], "" };
        out[k++] = (Child){ n->child[1], "" };
        break;
    default:
        break;
    }
    return k;
}

static void print_node(FILE *out, const AstNode *n, const char *prefix,
                       int is_last, int is_root, const char *label)
{
    if (!n)
        return;

    fprintf(out, "%s%s%s", prefix, is_root ? "" : (is_last ? "└── " : "├── "), label);
    print_description(out, n);
    fputc('\n', out);

    size_t len = strlen(prefix) + 8;
    char *child_prefix = xcalloc(len, 1);
    snprintf(child_prefix, len, "%s%s", prefix, is_root ? "" : (is_last ? "    " : "│   "));

    int max = (n->kind == NODE_PROGRAM || n->kind == NODE_BLOCK) ? n->count : 3;
    Child *children = xcalloc((size_t)(max > 0 ? max : 1), sizeof *children);
    int count = collect_children(n, children, max);
    for (int i = 0; i < count; i++)
        print_node(out, children[i].node, child_prefix, i == count - 1, 0, children[i].label);

    free(children);
    free(child_prefix);
}

void ast_print(FILE *out, const AstNode *root)
{
    print_node(out, root, "", 1, 1, "");
}

/* ------------------------------------------------------------------ */
/* Exportación a Graphviz (paleta Art Decó: navy, crema y dorado)      */
/* ------------------------------------------------------------------ */

#define DOT_NAVY  "#0B1A2E"
#define DOT_NAVY2 "#1C3353"
#define DOT_CREAM "#F3E9D2"
#define DOT_GOLD  "#C9A227"

static void dot_label(const AstNode *n, char *buf, size_t size)
{
    char type[16] = "";
    if (n->type != TYPE_UNKNOWN && n->kind != NODE_DECL)
        snprintf(type, sizeof type, "\\n%s", type_name(n->type));

    switch (n->kind) {
    case NODE_PROGRAM: snprintf(buf, size, "Programa"); break;
    case NODE_BLOCK:   snprintf(buf, size, "Bloque { }"); break;
    case NODE_DECL:    snprintf(buf, size, "splice %s %s", type_name(n->type), n->text); break;
    case NODE_ASSIGN:  snprintf(buf, size, "%s =", n->text); break;
    case NODE_READ:    snprintf(buf, size, "radio(%s)", n->text); break;
    case NODE_WRITE:   snprintf(buf, size, "diary"); break;
    case NODE_IF:      snprintf(buf, size, "atlas / ryan"); break;
    case NODE_WHILE:   snprintf(buf, size, "vita"); break;
    case NODE_BINOP:
    case NODE_RELOP:   snprintf(buf, size, "%s%s", op_symbol(n->op), type); break;
    case NODE_INT:
    case NODE_REAL:
    case NODE_ID:      snprintf(buf, size, "%s%s", n->text, type); break;
    }
}

static const char *dot_style(const AstNode *n)
{
    switch (n->kind) {
    case NODE_PROGRAM:
    case NODE_BLOCK:
        return "shape=box, style=\"rounded,filled\", fillcolor=\"" DOT_NAVY2 "\", "
               "color=\"" DOT_GOLD "\", fontcolor=\"" DOT_CREAM "\"";
    case NODE_DECL:
    case NODE_ASSIGN:
    case NODE_READ:
    case NODE_WRITE:
    case NODE_IF:
    case NODE_WHILE:
        return "shape=box, style=\"rounded,filled\", fillcolor=\"" DOT_GOLD "\", "
               "color=\"" DOT_GOLD "\", fontcolor=\"" DOT_NAVY "\"";
    case NODE_BINOP:
    case NODE_RELOP:
        return "shape=circle, style=filled, fillcolor=\"" DOT_CREAM "\", "
               "color=\"" DOT_GOLD "\", fontcolor=\"" DOT_NAVY "\"";
    default:
        return "shape=ellipse, style=filled, fillcolor=\"" DOT_CREAM "\", "
               "color=\"" DOT_CREAM "\", fontcolor=\"" DOT_NAVY "\"";
    }
}

static int dot_node(FILE *out, const AstNode *n, int *next_id)
{
    int id = (*next_id)++;
    char label[160];
    dot_label(n, label, sizeof label);
    fprintf(out, "  n%d [label=\"%s\", %s];\n", id, label, dot_style(n));

    int max = (n->kind == NODE_PROGRAM || n->kind == NODE_BLOCK) ? n->count : 3;
    Child *children = xcalloc((size_t)(max > 0 ? max : 1), sizeof *children);
    int count = collect_children(n, children, max);
    for (int i = 0; i < count; i++) {
        if (!children[i].node)
            continue;
        int child = dot_node(out, children[i].node, next_id);
        /* Las etiquetas de collect_children terminan en ": ". */
        int len = (int)strlen(children[i].label);
        if (len > 2)
            fprintf(out, "  n%d -> n%d [label=\"%.*s\"];\n", id, child, len - 2, children[i].label);
        else
            fprintf(out, "  n%d -> n%d;\n", id, child);
    }
    free(children);
    return id;
}

void ast_print_dot(FILE *out, const AstNode *root)
{
    int next_id = 0;
    fprintf(out,
            "digraph AST {\n"
            "  graph [bgcolor=\"" DOT_NAVY "\", pad=0.4, nodesep=0.35, ranksep=0.5, "
            "label=\"Kynova 1.0 — AST\", labelloc=t, fontcolor=\"" DOT_GOLD "\", "
            "fontname=\"Helvetica-Bold\", fontsize=22];\n"
            "  node  [fontname=\"Helvetica\", fontsize=13, penwidth=1.5];\n"
            "  edge  [color=\"" DOT_GOLD "\", fontcolor=\"" DOT_CREAM "\", "
            "fontname=\"Helvetica\", fontsize=11, arrowsize=0.7];\n");
    if (root)
        dot_node(out, root, &next_id);
    fprintf(out, "}\n");
}
