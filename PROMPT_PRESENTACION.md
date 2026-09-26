# Prompt para generar la presentación de la Entrega Parcial (Semana 11)

**Cómo usarlo:** copiar todo lo que está debajo de la línea "INICIO DEL PROMPT"
y pegarlo en Claude (claude.ai). Adjuntar también la imagen `ast.png` (está en la
raíz del proyecto) para que la incluya en la diapositiva del AST. Si se quiere
reutilizar el estilo exacto del deck anterior, adjuntar además el PowerPoint de
la primera presentación.

---

## INICIO DEL PROMPT

Eres un diseñador de presentaciones técnicas y experto en compiladores. Necesito
que generes una presentación en **PowerPoint (.pptx)** para la **entrega parcial
de la Semana 11** del proyecto del curso de Compiladores. Todo el contenido debe
estar en **español**. Abajo tienes el contexto completo, los datos reales
obtenidos al ejecutar el compilador y la estructura que quiero. No inventes
resultados: usa exactamente las salidas que te doy.

### 1. Contexto del proyecto

- **Lenguaje:** Kynova 1.0 (`.kyn`), "Rapture Edition". Es un lenguaje educativo
  imperativo con palabras reservadas inspiradas en BioShock. El tema es solo
  léxico: la gramática y la semántica son las de un lenguaje imperativo estándar.
- **Equipo (3 integrantes):**
  - Jonathan Javier Soberanis Castillo
  - Javier Augusto Estrada Gordillo
  - Gustavo Adolfo Ortiz Gutiérrez
- **Vocabulario:**

| Kynova   | Equivalente | Significado                    |
|----------|-------------|--------------------------------|
| `splice` | declare     | declarar variable              |
| `adam`   | int         | tipo entero                    |
| `eve`    | real        | tipo real                      |
| `radio`  | read        | entrada                        |
| `diary`  | show        | salida                         |
| `atlas`  | if          | condicional                    |
| `ryan`   | else        | rama alternativa (obligatoria) |
| `vita`   | while       | ciclo                          |

- **Apodos internos:** la tabla de símbolos se llama **"Gene Bank"** y la IR
  (fase futura) se llama **"ADAM crudo"**.

### 2. Qué pedía la entrega (requisitos del curso)

> **7. Entrega parcial — Semana 11.** Para esta fecha debe existir un front-end
> funcional:
> `Archivo del lenguaje → Flex → Tokens → Bison → AST inicial → Tabla de símbolos`
>
> - Archivo Flex (.l) funcional.
> - Archivo Bison (.y) funcional.
> - Integración Flex + Bison.
> - AST inicial para las construcciones principales.
> - Tabla de símbolos básica.
> - Prueba de un programa válido y uno inválido.
> - README con instrucciones de compilación y ejecución.

**Se cumplieron los 7 puntos.** Mapeo a archivos:

| Requisito                  | Archivo(s)                                       |
|----------------------------|--------------------------------------------------|
| Flex (.l)                  | `src/kynova.l`                                   |
| Bison (.y)                 | `src/kynova.y`                                   |
| Integración Flex + Bison   | `Makefile`, `src/lexer.h`, `src/main.c`          |
| AST inicial                | `src/ast.h`, `src/ast.c`                         |
| Tabla de símbolos          | `src/symtab.h/.c`, `src/semantic.c`              |
| Pruebas válidas/inválidas  | `tests/valid/` (3), `tests/invalid/` (11), `make test` |
| README                     | `README.md`                                      |

Datos generales: implementado en **C11**, con **Flex 2.6.4** y **GNU Bison 3.8.2**,
en unas 1,470 líneas de código propio. Compila **sin advertencias** y la gramática
tiene **0 conflictos** shift/reduce y reduce/reduce. **14 de 14 pruebas pasan** y
la herramienta `leaks` de macOS reporta **0 fugas de memoria**.

### 3. Detalle técnico por fase (para el contenido de las diapositivas)

**Flex — léxico (`src/kynova.l`)**
- Reconoce las 8 reservadas (distinguen mayúsculas: `Atlas` es identificador),
  identificadores `[a-zA-Z_][a-zA-Z0-9_]*`, reales `[0-9]+\.[0-9]+` (se reconocen
  antes que los enteros), enteros, operadores `+ - * /`, relacionales
  `< > <= >= == !=`, asignación `=` y delimitadores `( ) { } ;`.
- Descarta espacios y comentarios `//` y `/* */`.
- Cada token guarda **línea y columna** para los mensajes de error.
- Errores léxicos con mensaje específico: identificador que empieza con dígito
  (`2age`), real con coma (`80,5`), real sin parte fraccionaria (`3.`), comentario
  sin cerrar y carácter no reconocido.

Salida real de `./kynova --tokens` (primeras líneas del programa demostrativo):

```
Lín:Col   Token          Lexema
2:1       SPLICE         splice
2:8       ADAM           adam
2:13      ID             age
2:16      PUNTO_COMA     ;
3:1       SPLICE         splice
3:8       ADAM           adam
3:13      ID             laps
3:18      ASIGNACION     =
3:20      ENTERO         1
3:21      PUNTO_COMA     ;
...
Total: 66 tokens
```

**Bison — sintáctico (`src/kynova.y`)**
- La gramática sigue exactamente la EBNF del contrato.
- La **precedencia se codifica en la estructura** `expresion → termino → factor`
  (sin `%left`). La recursión por la izquierda da asociatividad de izquierda a
  derecha: `2 + 3 * 4 = 14`.
- **`ryan` es obligatorio**: la única regla de `atlas` exige `bloque RYAN bloque`.
- Los relacionales solo existen dentro de `condicion`, así que `a < b < c` es
  error sintáctico (no se encadenan).
- Los mensajes de error están en español y traen pistas, y hay recuperación de
  errores para reportar varios en una sola corrida.

Fragmento real de la gramática:

```
condicional
    : ATLAS '(' condicion ')' bloque RYAN bloque
                { $$ = ast_if($3, $5, $7, @1.first_line, @1.first_column); }
    ;

expresion
    : termino
    | expresion '+' termino   { $$ = ast_binop(OP_ADD, $1, $3, ...); }
    | expresion '-' termino   { $$ = ast_binop(OP_SUB, $1, $3, ...); }
    ;
```

**Integración Flex + Bison**
- Bison genera `kynova.tab.h` con los códigos de token, y Flex lo incluye.
- El parser llama a `yylex()`; el lexer devuelve el token, su valor en `yylval`
  (lexema de identificadores y números) y su posición en `yylloc`.
- Compilación con `make`:
  1. `bison -d src/kynova.y` genera el parser y `kynova.tab.h`.
  2. `flex src/kynova.l` genera el lexer.
  3. `cc -std=c11` enlaza todo en el ejecutable `./kynova`.

**AST (`src/ast.h`, `src/ast.c`)**
- 13 tipos de nodo: `PROGRAM`, `BLOCK`, `DECL` (splice), `ASSIGN`, `READ` (radio),
  `WRITE` (diary), `IF` (atlas/ryan), `WHILE` (vita), `BINOP`, `RELOP`, `INT`,
  `REAL` e `ID`.
- Cada nodo guarda línea y columna. El análisis semántico anota en cada expresión
  su tipo (`adam` o `eve`).
- Los paréntesis de agrupación no generan nodo, porque la forma del árbol ya
  representa la agrupación.
- Se puede exportar como imagen con Graphviz (`make ast-png`). **Usa la imagen
  adjunta `ast.png`** en la diapositiva del AST.

**Tabla de símbolos, "Gene Bank" (`src/symtab.c`, `src/semantic.c`)**
- Registra por variable: nombre, tipo, equivalente en C11, alcance, línea de
  declaración y número de usos.
- El programa es el alcance `global` y cada bloque `{ }` abre un alcance nuevo.
- Validaciones: variable no declarada o fuera de alcance, declaración duplicada
  en el mismo alcance, y asignar `eve` a `adam` (error por perder decimales;
  `adam` a `eve` sí se permite).
- Advertencias: variable nunca usada y división entre cero constante.

Salida real de `./kynova --symbols tests/valid/01_demostrativo.kyn`:

```
#    Nombre           Tipo  C11     Alcance              Línea   Usos
---  ---------------- ----- ------- -------------------- ------- ----
1    age              adam  int     global               2       2
2    laps             adam  int     global               3       3
3    average          eve   double  global               4       2

Resultado: programa VÁLIDO
```

### 4. Pruebas (datos reales)

Programa válido (demostrativo del contrato, `tests/valid/01_demostrativo.kyn`):

```
/* Programa demostrativo de Kynova — Rapture Edition */
splice adam age;
splice adam laps = 1;
splice eve average;

radio(age);
radio(average);

atlas (age >= 18) {
    diary(average + 5.0);
} ryan {
    diary(0);
}

// Repite tres veces
vita (laps <= 3) {
    diary(laps);
    laps = laps + 1;
}
```

Programas inválidos: hay 11 casos, y cada fase detecta los suyos. Estas son
salidas reales:

```
# Léxico
03_identificador_con_digito.kyn:2:13: error léxico: identificador inválido '2age': no puede iniciar con un dígito

# Sintáctico
04_falta_ryan.kyn:4:1: error sintáctico: fin de archivo inesperado, se esperaba 'ryan'
  nota: en Kynova 1.0 todo 'atlas' debe llevar su bloque 'ryan'.

# Semántico
07_declaracion_duplicada.kyn:3:12: error semántico: la variable 'age' ya fue declarada en este alcance (línea 2)
08_tipos_incompatibles.kyn:3:13: error semántico: tipos incompatibles: no se puede asignar un valor eve a 'age' (adam) sin perder la parte decimal
```

Lista completa de casos inválidos:

| Caso                          | Fase que lo detecta |
|-------------------------------|---------------------|
| Falta `;`                     | sintáctica          |
| Variable no declarada         | semántica           |
| `2age`                        | léxica              |
| `atlas` sin `ryan`            | sintáctica          |
| `vita` sin paréntesis         | sintáctica          |
| Real con coma `80,5`          | léxica              |
| Declaración duplicada         | semántica           |
| `eve` asignado a `adam`       | semántica           |
| Comentario sin cerrar         | léxica              |
| `a < b < c`                   | sintáctica          |
| Variable fuera de su alcance  | semántica           |

Resultado de `make test`: `Pruebas: 14 OK, 0 fallidas`.

Comandos para la demo en vivo:

```
make                                          # compilar
./kynova tests/valid/01_demostrativo.kyn      # AST + tabla de símbolos
./kynova --tokens tests/valid/01_demostrativo.kyn
./kynova tests/invalid/04_falta_ryan.kyn      # programa inválido
make test                                     # las 14 pruebas
```

### 5. Estilo visual (obligatorio)

Debe mantener la identidad del deck anterior del equipo:

- Estilo **"Art Decó sumergido"**, minimalista.
- Paleta: **navy oscuro `#0B1A2E`** (fondo), **navy medio `#1C3353`** (tarjetas),
  **crema `#F3E9D2`** (texto) y **dorado `#C9A227`** (acentos, líneas y títulos).
- Motivo recurrente de **círculos con íconos** (un círculo dorado con un ícono
  lineal adentro) en todas las diapositivas.
- Líneas finas doradas y ornamentos geométricos Art Decó discretos en esquinas o
  separadores.
- Tipografía: títulos en una sans geométrica o Art Decó (por ejemplo, Josefin
  Sans, Poiret One o Montserrat); cuerpo en sans legible. El código va en
  monoespaciada (JetBrains Mono, Fira Code o Consolas) sobre un panel navy medio,
  con resaltado en dorado para las reservadas de Kynova.
- Poco texto por diapositiva: ideas clave en viñetas cortas. Los detalles van en
  las notas del orador.

### 6. Estructura deseada (12 diapositivas)

1. **Portada**: "Kynova 1.0 — Rapture Edition" / "Entrega parcial · Semana 11 ·
   Front-end del compilador" / nombres del equipo.
2. **Objetivo de la entrega**: el pipeline de la captura y un checklist visual de
   los 7 requisitos, todos marcados como cumplidos.
3. **Arquitectura implementada**: pipeline `.kyn → Flex → Tokens → Bison → AST →
   Gene Bank`, con el archivo fuente bajo cada nodo. Mostrar "IR (ADAM crudo)" y
   "C11" en gris o atenuados como fases futuras.
4. **Flex, análisis léxico**: qué reconoce, un extracto de la tabla de tokens y
   los errores léxicos detectados.
5. **Bison, análisis sintáctico**: fragmento de la gramática, precedencia por
   estructura, `ryan` obligatorio y "0 conflictos".
6. **Integración Flex + Bison**: diagrama de cómo se comunican (`yylex`, `yylval`,
   `yylloc`, `kynova.tab.h`) y los 3 pasos del `make`.
7. **AST inicial**: la imagen `ast.png` grande y, al lado, los tipos de nodo.
8. **Tabla de símbolos "Gene Bank"**: la tabla real y las validaciones en íconos
   (no declarada, duplicada, tipos, alcance).
9. **Pruebas: válido vs. inválido**: comparación lado a lado (código válido y su
   "programa VÁLIDO" contra `atlas` sin `ryan` y su mensaje de error), con el
   contador "14/14 pruebas".
10. **Demo y README**: los comandos para compilar y ejecutar, y la estructura del
    repositorio.
11. **Siguientes pasos**: IR ("ADAM crudo") y generación de C11
    (`adam→int`, `eve→double`, `atlas/ryan→if/else`, `vita→while`). Remarcar que
    el back-end queda separado del análisis.
12. **Cierre**: agradecimiento y nota de uso responsable de IA: "Se utilizó IA
    como apoyo para organizar la especificación, comprender Flex/Bison y como
    asistente de programación. El equipo validó, ejecutó y puede explicar cada
    elemento del código."

### 7. Reparto de la exposición y notas del orador

Agrega **notas del orador** en cada diapositiva (de 60 a 120 palabras, en tono
natural y hablado) y reparte la exposición así:

- **Jonathan:** diapositivas 1 a 4 (introducción, objetivo, arquitectura y Flex).
- **Javier:** diapositivas 5 a 7 (Bison, integración y AST).
- **Gustavo:** diapositivas 8 a 12 (tabla de símbolos, pruebas, demo, siguientes
  pasos y cierre).

Al inicio de cada nota indica quién habla, por ejemplo "[Javier]". Las notas
deben explicar el *porqué* de las decisiones (por ejemplo, por qué la precedencia
va en la gramática, por qué `ryan` es obligatorio o por qué cada bloque abre un
alcance), porque el profesor puede hacer preguntas.

### 8. Entregables que espero de ti

1. El archivo `.pptx` descargable. Si no puedes generarlo directamente, entrega un
   script de Python con `python-pptx` que lo genere, listo para ejecutar.
2. Un resumen breve de cada diapositiva.
3. Una lista de 5 posibles preguntas del profesor con respuestas cortas.
