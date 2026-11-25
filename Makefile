# [REQ:RNF6] Makefile como build principal (Windows/w64devkit, Linux e macOS)
# Makefile do PROJETO PRECURSOR (build principal)
#
#   make              compila o jogo em bin/
#   make run          compila (se precisar) e abre o jogo
#   make test         roda os testes sem janela (lógica, regras e fluxo)
#   make smoke        abre o jogo no piloto automático e joga a partida inteira
#   make readme-check confere as referências de código citadas no README
#   make clean        apaga obj/ e bin/
#
# Funciona no Windows (w64devkit do instalador da raylib), no Linux e no macOS.
# Se a raylib estiver em outro lugar, passe o caminho: make RAYLIB_PATH=C:/minha/raylib

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -DRAYMATH_STATIC_INLINE -Isrc -MMD -MP

ifeq ($(OS),Windows_NT)
    PLATAFORMA = WINDOWS
    EXE = .exe
    RAYLIB_PATH ?= C:/raylib/raylib
    RAYLIB_INC_DIR ?= $(RAYLIB_PATH)/src
    RAYLIB_LIB_DIR ?= $(RAYLIB_PATH)/src
    RAYLIB_CFLAGS = -isystem $(RAYLIB_INC_DIR)
    RAYLIB_LIBS = -L$(RAYLIB_LIB_DIR) -lraylib -lopengl32 -lgdi32 -lwinmm
else
    EXE =
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATAFORMA = MACOS
        LIBS_SISTEMA = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    else
        PLATAFORMA = LINUX
        LIBS_SISTEMA = -lGL -lm -lpthread -ldl -lrt -lX11
    endif
    ifdef RAYLIB_PATH
        RAYLIB_INC_DIR ?= $(RAYLIB_PATH)/src
        RAYLIB_LIB_DIR ?= $(RAYLIB_PATH)/src
        RAYLIB_CFLAGS = -isystem $(RAYLIB_INC_DIR)
        RAYLIB_LIBS = -L$(RAYLIB_LIB_DIR) -lraylib $(LIBS_SISTEMA)
    else
        # pkg-config devolve -I...; trocamos por -isystem para os avisos da raylib não se misturarem aos nossos
        RAYLIB_CFLAGS = $(patsubst -I%,-isystem %,$(shell pkg-config --cflags raylib))
        RAYLIB_LIBS = $(shell pkg-config --libs raylib) -lm
    endif
endif

FONTES = $(wildcard src/*.c)
OBJETOS = $(patsubst src/%.c,obj/%.o,$(FONTES))
JOGO = bin/precursor$(EXE)
# teste visual do menu (abre janela; fica fora do make test): make test-menu
TEST_MENU = bin/test_menu$(EXE)

TESTES = bin/test_logica$(EXE) bin/test_regras$(EXE) bin/test_fluxo$(EXE)

.PHONY: all run test smoke readme-check clean test-menu

all: $(JOGO)

$(JOGO): $(OBJETOS) | bin
	$(CC) $(OBJETOS) -o $@ $(RAYLIB_LIBS)

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

obj/test_%.o: tests/test_%.c | obj
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -Itests -c $< -o $@

# test_logica e test_fluxo só usam os módulos puros: não precisam da raylib
bin/test_logica$(EXE): obj/test_logica.o obj/logica.o obj/regras.o obj/regras_texto.o | bin
	$(CC) $^ -o $@ -lm

bin/test_fluxo$(EXE): obj/test_fluxo.o obj/logica.o obj/regras.o obj/regras_texto.o | bin
	$(CC) $^ -o $@ -lm

# test_regras também testa as coleções, que usam TraceLog e CheckCollisionBoxes (sem abrir janela)
bin/test_regras$(EXE): obj/test_regras.o obj/regras.o obj/regras_texto.o obj/regras_ranking.o obj/regras_sanidade.o \
                        obj/regras_som.o obj/logica.o obj/colecoes.o obj/materiais.o obj/materiais_parede.o \
                        obj/esqueleto.o obj/regras_animacao.o obj/regras_animacao_cenas.o obj/regras_porta.o obj/regras_cena.o | bin
	$(CC) $^ -o $@ $(RAYLIB_LIBS)

$(TEST_MENU): obj/test_menu.o obj/menu_inicial.o | bin
	$(CC) $^ -o $@ $(RAYLIB_LIBS)

test-menu: $(TEST_MENU)
	./$(TEST_MENU)

run: $(JOGO)
	./$(JOGO)

test: $(TESTES)
	./bin/test_logica$(EXE)
	./bin/test_regras$(EXE)
	./bin/test_fluxo$(EXE)

smoke: $(JOGO)
	./$(JOGO) --smoke

readme-check:
	sh tools/check_readme_refs.sh

bin obj:
	mkdir -p $@

clean:
	rm -rf obj bin

-include $(wildcard obj/*.d)
