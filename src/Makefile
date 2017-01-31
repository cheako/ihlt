LDLIBS=-lrt
ihlt : ConnectionsDoublyLinkedList.o protocol.o server.o relay_commands.o \
		channel_commands.o BaseLookup.o cdecode.o accounting.o \
		storage_commands.o

ConnectionsDoublyLinkedList.o : ConnectionsDoublyLinkedList.h

relay_commands.o : relay_commands.h

protocol.o : protocol.h

server.o : server.h

channel_commands.o : channel_commands.h

BaseLookup.o : BaseLookup.h

cdecode.o : cdecode.h

cencode.o : cencode.h

accounting.o : accounting.h

storage_commands.o : storage_commands.h

clean:
	rm -f *.o ihlt

.PHONY: clean
