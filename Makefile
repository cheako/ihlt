all:
	$(MAKE) -C src

clean:
	$(MAKE) -C example clean
	$(MAKE) -C src clean

test: clean
	CFLAGS=-g LDFLAGS=-g $(MAKE) -C example
	CFLAGS=-g LDFLAGS=-g $(MAKE)
	rm -rf ~/.ihlt
	mkdir ~/.ihlt
	cp example/openpgp-server.txt ~/.ihlt/certfile.txt
	cp example/openpgp-secret.txt ~/.ihlt/keyfile.txt
	prove -f

.PHONY: all clean test
