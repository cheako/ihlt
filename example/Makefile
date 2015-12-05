CFLAGS += $(shell pkg-config --cflags gnutls)
LDLIBS += $(shell pkg-config --libs gnutls)

all: openpgp-auth
	ltrace -f ./openpgp-auth

openpgp-auth: utils.o

utils.o: utils.h

clean:
	rm -rf *.o openpgp-auth

.PHONY: all clean
