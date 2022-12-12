# Copyright 2022, pEp Foundation
# This file is part of libpEpCxx11
# This file may be used under the terms of the GNU General Public License version 3
# see LICENSE

include ../Makefile.conf

SOURCE=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJECTS=$(subst .c,.o,$(SOURCE))
DEPENDS=$(subst .c,.d,$(SOURCE))
CFLAGS+= -MMD -MP

ifneq ($(MAKECMDGOALS),clean)
    -include $(DEPENDS)
endif

.PHONY: all install uninstall clean
.DEFAULT_GOAL := all

all: $(TARGET)

$(TARGET): $(OBJECTS)

clean:
	rm -vf $(TARGET) $(OBJECTS) $(DEPENDS)
	rm -f *.d.*
