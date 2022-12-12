# Copyright 2022, pEp Foundation
# This file is part of libpEpCxx11
# This file may be used under the terms of the GNU General Public License version 3
# see LICENSE

include ../Makefile.conf

EXE_MAIN_CLI:=cli_main
EXE_MAIN_TUI:=tui_main

OBJ_MAIN_CLI:=$(EXE_MAIN_CLI).o
OBJ_MAIN_TUI:=$(EXE_MAIN_TUI).o

SOURCE:=$(wildcard *.c)
HEADERS:=$(wildcard *.h)

OBJECTS:=$(subst .c,.o,$(SOURCE))
OBJECTS:=$(filter-out $(OBJ_MAIN_CLI),$(OBJECTS))
OBJECTS:=$(filter-out $(OBJ_MAIN_TUI),$(OBJECTS))

DEPENDS:=$(subst .c,.d,$(SOURCE))
CFLAGS+= -MMD -MP

ifneq ($(MAKECMDGOALS),clean)
    -include $(DEPENDS)
endif

.PHONY: all install uninstall clean
.DEFAULT_GOAL := all

all: $(EXE_MAIN_CLI) $(EXE_MAIN_TUI)

$(EXE_MAIN_CLI): $(OBJECTS) $(OBJ_MAIN_CLI)

$(EXE_MAIN_TUI): $(OBJECTS) $(OBJ_MAIN_TUI)

clean:
	rm -vf \
		$(DEPENDS)
		$(OBJECTS) \
		$(OBJ_MAIN_TUI) \
		$(OBJ_MAIN_CLI) \
		$(EXE_MAIN_TUI) \
		$(EXE_MAIN_CLI) \
	rm -f *.d.*
