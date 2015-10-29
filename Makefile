all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

test: clean
	CFLAGS=-g LDFLAGS=-g $(MAKE)
	prove -f

.PHONY: all clean test
