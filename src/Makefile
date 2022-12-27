include ../Makefile.conf

SRC:=$(wildcard *.c*)
SRC_EXE:=$(filter main_%, $(SRC))
SRC_LIB=$(filter-out main_%, $(SRC))
OBJS:=$(addsuffix .o, $(basename $(SRC)))
OBJS_LIB:=$(addsuffix .o, $(basename $(SRC_LIB)))
EXE:=$(basename $(SRC_EXE))
DEPS:=$(addsuffix .d, $(basename $(SRC)))

.PHONY: all install uninstall clean
.DEFAULT_GOAL:= all

ifneq ($(MAKECMDGOALS),clean)
    -include $(DEPS)
endif

all: $(EXE)

$(LIB): $(OBJS)
	$(AR) -rc $@ $(OBJS_LIB)

$(EXE) : $(LIB)

clean:
	rm -rf \
		$(OBJS) \
		$(EXE) \
		$(LIB) \
		$(DEPS) \
		*.dSYM \
		*.h.gch