/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 5015  Michael Mestnik <cheako+github_com@mikemestnik.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Example from:
 *   http://www.tenouk.com/Module41.html
 *   http://linux.die.net/man/3/getaddrinfo
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* port we're listening on */
#define PORT "4458"

#include "ConnectionsDoublyLinkedList.h"

void LineLocator(struct ConnectionNode *conn) {
	/* look for the end of the line */
	struct ConnectionNode *j;
	for (j = conn->next; j != conn; j = j->next) {
		printf("socket send to %s on socket %d index %d\n", j->host, j->fd,
				j->index);
		if (send(j->fd, conn->buf, conn->nbytes, 0) == -1)
			perror("Negative send");
	}
	conn->buf = NULL;
	conn->nbytes = 0;
}

void ProccessInput(struct ConnectionNode *conn, char *buf, size_t nbytes) {
	/* we got some data from a client */
	printf("socket recv from %s on socket %d index %d\n", conn->host, conn->fd,
			conn->index);
	conn->nbytes += nbytes;
	conn->buf = buf;
	LineLocator(conn);
}

int main(int argc, char *argv[]) {
	/* master file descriptor list */
	fd_set master;
	/* temp file descriptor list for select() */
	fd_set read_fds;
	/* server address */
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	/* maximum file descriptor number */
	int fdmax;
	/* listening socket descriptor */
	int listener;
	/* for setsockopt() SO_REUSEADDR, below */
	int yes = 1;
	int j;

	/* clear the master and temp sets */
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;

	j = getaddrinfo(NULL, PORT, &hints, &result);
	if (j != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(j));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	 Try each address until we successfully bind(2).
	 If socket(2) (or bind(2)) fails, we (close the socket
	 and) try the next address. */

	for (rp = result; rp != NULL ; rp = rp->ai_next) {
		listener = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listener == -1)
			continue;

		/*"address already in use" error message */
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1)
			goto tryagain;

#ifdef SO_REUSEPORT
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int))
				== -1)
			goto tryagain;
#endif

		if (bind(listener, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* Success */

		tryagain: close(listener);
	}

	if (rp == NULL ) { /* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result); /* No longer needed */

	/* listen */
	if (listen(listener, SOMAXCONN) == -1) {
		perror("Error opening listener");
		exit(1);
	}

	/* add the listener to the master set */
	FD_SET(listener, &master);
	/* keep track of the biggest file descriptor */
	fdmax = listener; /* so far, it's this one */

	/* loop */
	for (;;) {
		/* copy it */
		read_fds = master;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
			perror("Error waiting for input");
			exit(1);
		}

		if (FD_ISSET(listener, &read_fds)) { /* we got a new one... */
			/* handle new connections */
			struct ConnectionNode *TempNode = GetNewConnection();
			TempNode->addr_len = sizeof(TempNode->addr);
			if ((TempNode->fd = accept(listener,
					(struct sockaddr *) &TempNode->addr, &TempNode->addr_len))
					== -1) {
				perror("Warning accepting one new connection");
				free(TempNode);
			} else {
				/* add to master set */
				if (TempNode->fd > fdmax) /* keep track of the maximum */
					if (TempNode->fd > FD_SETSIZE) {
						close(TempNode->fd);
						free(TempNode);
						TempNode = NULL;
					} else
						fdmax = TempNode->fd;
				if (TempNode != NULL ) {
					FD_SET(TempNode->fd, &master);

					TempNode->getnameinfo = getnameinfo(
							(struct sockaddr *) &TempNode->addr,
							TempNode->addr_len, TempNode->host, NI_MAXHOST,
							NULL, 0, 0);

					InsertConnectionBefore(&connections_head, TempNode);
					printf("%s: New connection from %s on socket %d index %d\n",
							argv[0], TempNode->host, TempNode->fd,
							TempNode->index);
				}
			}
		}

		/* run through the existing connections looking for data to be read */
		struct ConnectionNode *i = connections_head;
		if (connections_head != NULL )
			do {
				if (FD_ISSET(i->fd, &read_fds)) { /* we got one... */
					/* handle data from a client */
					printf("%s: New data from %s on socket %d index %d\n",
							argv[0], i->host, i->fd, i->index);
					/* buffer for client data */
					char buf[1024];
					int nbytes;
					if ((nbytes = recv(i->fd, buf, sizeof(buf) - 1, 0)) <= 0) {
						/* got error or connection closed by client */
						if (nbytes == 0)
							/* connection closed */
							printf(
									"%s: socket to %s hung up on socket %d index %d\n",
									argv[0], i->host, i->fd, i->index);
						else
							perror("Negative recv");
						/* close it... */
						close(i->fd);
						/* remove from master set */
						FD_CLR(i->fd, &master);
						/* step back and remove this connection */
						i = RemoveConnection(i);
						if (i == NULL )
							break;
					} else {
						/* Ensure this is an ansi string */
						buf[nbytes] = '\0';
						ProccessInput(i, buf, nbytes);
					}
				}
				i = i->next;
			} while (i != connections_head);
	}
	return 0;
}
