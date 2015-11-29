CFLAGS += -D_FILE_OFFSET_BITS=64 -DLARGEFILE_SOURCE=1
CFLAGS += $(shell gpgme-config --cflags)
LDLIBS += $(shell gpgme-config --libs)

ihlt : ConnectionsDoublyLinkedList.o protocol.o server.o relay_commands.o

ConnectionsDoublyLinkedList.o : ConnectionsDoublyLinkedList.h

relay_commands.o : relay_commands.h

protocol.o : protocol.h

server.o : server.h

clean:
	rm -f *.o ihlt

.PHONY: clean
