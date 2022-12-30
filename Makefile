.PHONY: all test src clean

all: test src

src:
	$(MAKE) -C src

test:
	$(MAKE) -C test

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean