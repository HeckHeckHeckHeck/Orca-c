include ../Makefile.conf

SRC:=$(wildcard *.c*)
SRC_EXE:=$(filter test_%, $(SRC))
EXE:=$(basename $(SRC_EXE))

$(info src exe: $(SRC_EXE))
$(info lib: $(LIB))
$(info exe: $(EXE))

LDFLAGS+=-L../src
CFLAGS+=-I../src
CXXFLAGS+=-I../src

.PHONY: lib all clean ../src/$(LIB)
.DEFAULT_GOAL := all

all: $(EXE)

../src/$(LIB):
	$(MAKE) -C ../src $(LIB)

$(EXE): ../src/$(LIB)

clean:
	rm -rf \
		$(OBJS) \
		$(EXE) \
		$(DEPS) \
		*.dSYM \
		*.h.gch