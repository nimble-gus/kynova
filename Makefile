# Makefile — Front-end de Kynova 1.0 "Rapture Edition"
#
#   make          compila el ejecutable ./kynova
#   make test     corre las pruebas válidas e inválidas
#   make demo     ejecuta el programa demostrativo mostrando tokens, AST y símbolos
#   make ast-png  genera ast.png con el AST (requiere Graphviz; KYN=archivo.kyn)
#   make clean    borra los archivos generados

# En macOS el bison del sistema (2.3) es muy viejo; se prefiere el de Homebrew.
BISON ?= $(firstword $(wildcard /opt/homebrew/opt/bison/bin/bison /usr/local/opt/bison/bin/bison) bison)
FLEX  ?= $(firstword $(wildcard /opt/homebrew/opt/flex/bin/flex /usr/local/opt/flex/bin/flex) flex)
CC    ?= cc

BUILD   := build
TARGET  := kynova
CFLAGS  ?= -std=c11 -Wall -Wextra -g
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -Isrc -I$(BUILD)
# El código generado por Flex/Bison no está pensado para -Wextra.
GEN_CFLAGS := $(CFLAGS) -Wno-unused-function -Wno-sign-compare -Wno-unused-parameter

SRCS := src/main.c src/ast.c src/symtab.c src/semantic.c
OBJS := $(SRCS:src/%.c=$(BUILD)/%.o) $(BUILD)/kynova.tab.o $(BUILD)/lex.yy.o

KYN ?= tests/valid/01_demostrativo.kyn

.PHONY: all test demo ast-png clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD):
	mkdir -p $@

$(BUILD)/kynova.tab.c $(BUILD)/kynova.tab.h: src/kynova.y | $(BUILD)
	$(BISON) -Wall -d -o $(BUILD)/kynova.tab.c $<

$(BUILD)/lex.yy.c: src/kynova.l $(BUILD)/kynova.tab.h | $(BUILD)
	$(FLEX) -o $@ $<

$(BUILD)/kynova.tab.o: $(BUILD)/kynova.tab.c
	$(CC) $(CPPFLAGS) $(GEN_CFLAGS) -c -o $@ $<

$(BUILD)/lex.yy.o: $(BUILD)/lex.yy.c
	$(CC) $(CPPFLAGS) $(GEN_CFLAGS) -c -o $@ $<

$(BUILD)/%.o: src/%.c $(BUILD)/kynova.tab.h $(wildcard src/*.h) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	@sh tests/run_tests.sh ./$(TARGET)

demo: $(TARGET)
	./$(TARGET) --tokens --ast --symbols tests/valid/01_demostrativo.kyn

ast-png: $(TARGET)
	./$(TARGET) -q --dot $(BUILD)/ast.dot $(KYN)
	dot -Tpng -Gdpi=200 $(BUILD)/ast.dot -o ast.png
	@echo "Imagen generada: ast.png"

clean:
	rm -rf $(BUILD) $(TARGET)
