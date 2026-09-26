/*
 * main.c — Driver del front-end de Kynova.
 *
 *   archivo .kyn -> Flex (tokens) -> Bison (AST) -> tabla de símbolos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "semantic.h"
#include "symtab.h"

enum { SHOW_TOKENS = 1, SHOW_AST = 2, SHOW_SYMBOLS = 4 };

static void usage(FILE *out, const char *prog)
{
    fprintf(out,
            "Uso: %s [opciones] archivo.kyn\n"
            "\n"
            "Opciones:\n"
            "  -t, --tokens    muestra el flujo de tokens (salida de Flex)\n"
            "  -a, --ast       muestra el AST (salida de Bison)\n"
            "  -s, --symbols   muestra la tabla de símbolos (Gene Bank)\n"
            "  -d, --dot ARCH  exporta el AST en formato Graphviz a ARCH\n"
            "                  (imagen: dot -Tpng ARCH -o ast.png)\n"
            "  -q, --quiet     solo muestra errores y el resultado final\n"
            "  -h, --help      muestra esta ayuda\n"
            "\n"
            "Sin opciones se muestran el AST y la tabla de símbolos.\n"
            "Código de salida: 0 = programa válido, 1 = errores, 2 = error de uso.\n",
            prog);
}

static const char *token_name(int tok)
{
    switch (tok) {
    case SPLICE:   return "SPLICE";
    case ADAM:     return "ADAM";
    case EVE:      return "EVE";
    case RADIO:    return "RADIO";
    case DIARY:    return "DIARY";
    case ATLAS:    return "ATLAS";
    case RYAN:     return "RYAN";
    case VITA:     return "VITA";
    case ID:       return "ID";
    case INT_LIT:  return "ENTERO";
    case REAL_LIT: return "REAL";
    case LE:       return "MENOR_IGUAL";
    case GE:       return "MAYOR_IGUAL";
    case EQ:       return "IGUAL";
    case NE:       return "DIFERENTE";
    case '<':      return "MENOR";
    case '>':      return "MAYOR";
    case '+':      return "MAS";
    case '-':      return "MENOS";
    case '*':      return "POR";
    case '/':      return "ENTRE";
    case '=':      return "ASIGNACION";
    case '(':      return "PAR_IZQ";
    case ')':      return "PAR_DER";
    case '{':      return "LLAVE_IZQ";
    case '}':      return "LLAVE_DER";
    case ';':      return "PUNTO_COMA";
    default:       return "DESCONOCIDO";
    }
}

static void dump_tokens(void)
{
    printf("=== Tokens (Flex) ===\n");
    printf("%-10s %-14s %s\n", "Lín:Col", "Token", "Lexema");
    int tok, count = 0;
    while ((tok = yylex()) != 0) {
        char pos[24];
        snprintf(pos, sizeof pos, "%d:%d", yylloc.first_line, yylloc.first_column);
        printf("%-9s %-14s %s\n", pos, token_name(tok), yytext);
        if (tok == ID || tok == INT_LIT || tok == REAL_LIT)
            free(yylval.str);
        count++;
    }
    printf("Total: %d tokens\n\n", count);
}

int main(int argc, char **argv)
{
    int show = 0, quiet = 0;
    const char *path = NULL;
    const char *dot_path = NULL;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-d") || !strcmp(a, "--dot")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "kynova: '%s' requiere el nombre del archivo .dot\n", a);
                return 2;
            }
            dot_path = argv[++i];
        }
        else if (!strcmp(a, "-t") || !strcmp(a, "--tokens"))  show |= SHOW_TOKENS;
        else if (!strcmp(a, "-a") || !strcmp(a, "--ast"))     show |= SHOW_AST;
        else if (!strcmp(a, "-s") || !strcmp(a, "--symbols")) show |= SHOW_SYMBOLS;
        else if (!strcmp(a, "-q") || !strcmp(a, "--quiet"))   quiet = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout, argv[0]);
            return 0;
        } else if (a[0] == '-') {
            fprintf(stderr, "kynova: opción desconocida '%s'\n\n", a);
            usage(stderr, argv[0]);
            return 2;
        } else if (!path) {
            path = a;
        } else {
            fprintf(stderr, "kynova: solo se admite un archivo de entrada\n");
            return 2;
        }
    }
    if (!path) {
        usage(stderr, argv[0]);
        return 2;
    }
    if (quiet)
        show = 0;
    else if (show == 0 && !dot_path)
        show = SHOW_AST | SHOW_SYMBOLS;

    const char *ext = strrchr(path, '.');
    if (!ext || strcmp(ext, ".kyn") != 0)
        fprintf(stderr, "kynova: advertencia: '%s' no tiene extensión .kyn\n", path);

    yyin = fopen(path, "r");
    if (!yyin) {
        perror(path);
        return 2;
    }
    kyn_filename = path;

    if (show & SHOW_TOKENS) {
        dump_tokens();
        /* Segunda lectura para el parser: los errores léxicos ya se mostraron. */
        rewind(yyin);
        yyrestart(yyin);
        lexer_reset();
        lex_errors = 0;
        lex_silent = 1;
    }

    yyparse();
    fclose(yyin);
    yylex_destroy();

    if (lex_errors || syntax_errors) {
        fprintf(stderr, "\nResultado: programa INVÁLIDO (%d error(es) léxico(s), %d sintáctico(s))\n",
                lex_errors, syntax_errors);
        ast_free(parse_result);
        return 1;
    }

    SymbolTable st;
    symtab_init(&st);
    int sem_errors = semantic_check(parse_result, &st, kyn_filename);

    if (show & SHOW_AST) {
        printf("=== AST (Bison, anotado con tipos) ===\n");
        ast_print(stdout, parse_result);
        printf("\n");
    }
    if (show & SHOW_SYMBOLS) {
        printf("=== Tabla de símbolos (Gene Bank) ===\n");
        symtab_print(stdout, &st);
        printf("\n");
    }
    if (dot_path) {
        FILE *dot = fopen(dot_path, "w");
        if (!dot) {
            perror(dot_path);
        } else {
            ast_print_dot(dot, parse_result);
            fclose(dot);
            printf("AST exportado a %s\n", dot_path);
        }
    }

    symtab_free(&st);
    ast_free(parse_result);

    if (sem_errors) {
        fprintf(stderr, "Resultado: programa INVÁLIDO (%d error(es) semántico(s))\n", sem_errors);
        return 1;
    }
    printf("Resultado: programa VÁLIDO\n");
    return 0;
}
