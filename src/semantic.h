#ifndef KYNOVA_SEMANTIC_H
#define KYNOVA_SEMANTIC_H

#include "ast.h"
#include "symtab.h"

/*
 * Recorre el AST, llena la tabla de símbolos y valida:
 *   - variables usadas sin declarar (o fuera de su alcance),
 *   - declaraciones duplicadas en el mismo alcance,
 *   - compatibilidad de tipos en asignaciones (eve no cabe en adam).
 * Anota en cada expresión su tipo inferido. Devuelve el número de errores.
 */
int semantic_check(AstNode *program, SymbolTable *st, const char *filename);

#endif
