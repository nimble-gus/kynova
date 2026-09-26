#ifndef KYNOVA_AST_H
#define KYNOVA_AST_H

#include <stdio.h>

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_ADAM,   /* entero */
    TYPE_EVE     /* real   */
} DataType;

typedef enum {
    NODE_PROGRAM,
    NODE_BLOCK,
    NODE_DECL,     /* splice tipo id [= expr];      */
    NODE_ASSIGN,   /* id = expr;                    */
    NODE_READ,     /* radio(id);                    */
    NODE_WRITE,    /* diary(expr);                  */
    NODE_IF,       /* atlas (cond) {...} ryan {...} */
    NODE_WHILE,    /* vita (cond) {...}             */
    NODE_BINOP,    /* + - * /                       */
    NODE_RELOP,    /* < > <= >= == !=               */
    NODE_INT,
    NODE_REAL,
    NODE_ID
} NodeKind;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE
} OpKind;

typedef struct AstNode AstNode;

struct AstNode {
    NodeKind kind;
    int line;
    int col;
    /* DECL: tipo declarado. Expresiones: tipo inferido por el análisis semántico. */
    DataType type;
    OpKind op;
    /* Nombre de la variable (ID, DECL, ASSIGN, READ) o lexema del literal. */
    char *text;
    long long ival;
    double dval;
    /* IF: cond, entonces, ryan | WHILE: cond, cuerpo | BINOP/RELOP: izq, der
       DECL: valor inicial (opcional) | ASSIGN/WRITE: expresión */
    AstNode *child[3];
    /* PROGRAM y BLOCK: lista de instrucciones. */
    AstNode **items;
    int count;
    int cap;
};

/* Los constructores toman posesión de los char* recibidos. */
AstNode *ast_block(int line, int col);
void     ast_block_append(AstNode *block, AstNode *stmt);
AstNode *ast_program(AstNode *block);
AstNode *ast_decl(DataType type, char *name, AstNode *init, int line, int col);
AstNode *ast_assign(char *name, AstNode *expr, int line, int col);
AstNode *ast_read(char *name, int line, int col);
AstNode *ast_write(AstNode *expr, int line, int col);
AstNode *ast_if(AstNode *cond, AstNode *then_block, AstNode *else_block, int line, int col);
AstNode *ast_while(AstNode *cond, AstNode *body, int line, int col);
AstNode *ast_binop(OpKind op, AstNode *left, AstNode *right, int line, int col);
AstNode *ast_relop(OpKind op, AstNode *left, AstNode *right, int line, int col);
AstNode *ast_int(char *lexeme, int line, int col);
AstNode *ast_real(char *lexeme, int line, int col);
AstNode *ast_id(char *name, int line, int col);

void ast_free(AstNode *node);
void ast_print(FILE *out, const AstNode *root);
/* Exporta el árbol en formato Graphviz DOT (convertir con: dot -Tpng). */
void ast_print_dot(FILE *out, const AstNode *root);

const char *type_name(DataType type);
const char *type_c11_name(DataType type);
const char *op_symbol(OpKind op);

#endif
