/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2015,2017  Michael Mestnik <cheako+github_com@mikemestnik.net>
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

/**
 * @file server.c
 * @brief using select() for I/O multiplexing
 * @author Mike Mestnik
 *
 * Example from:
 * - [http://www.tenouk.com/Module41.html]
 * - [http://linux.die.net/man/3/getaddrinfo]
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "server.h"
#include "accounting.h"

/** Send/receive raw dataa */
int sock_send(int fd, char *src, size_t size) {
	int offset = 0;

	if (!src)
		return -1;

	while (offset != size) {
		// write isn't guaranteed to send the entire string at once,
		// so we have to sent it in a loop like this
		int sent = send(fd, ((char *) src) + offset, size - offset, 0);
		if (sent == -1) {
			if (errno != EAGAIN)
				//shutdown(fd, SHUT_RDWR);
				return sent;
			continue;
		} else if (sent == 0) {
			// when this returns zero, it generally means
			// we got disconnected
			return sent + offset;
		}

		offset += sent;
	}

	return offset;
}

struct ProccessInputHandler {
	void (*func)(struct ConnectionNode *, struct ProccessInputHandler *, char *,
			size_t);
	void (*free)(struct ConnectionNode *, struct ProccessInputHandler *);
	struct ConnectionNodeHandler *prev;
	char * buf;
	size_t nbytes;
};

static void ProccessInputFree(struct ConnectionNode *i,
		struct ProccessInputHandler *h) {
	if (h->prev != NULL)
		h->prev->free(i, h->prev);
	if (h->buf != NULL)
		free(h->buf);
	free(h);
}

static void LineLocator(struct ConnectionNode *conn,
		struct ProccessInputHandler *h) {
	/* look for the end of the line */
	char *str1, *saveptr1, *ntoken, *token;
	bool endswell = false;
	switch (h->buf[h->nbytes - 1]) {
	case '\r':
	case '\n':
		endswell = true;
		break;
	};

	for (str1 = h->buf;; str1 = NULL) {
		ntoken = strtok_r(str1, "\r\n", &saveptr1);
		/* Reverse this so we know the next result. */
		if (str1 == NULL) {
			if (ntoken != NULL || endswell) {
				char *str2, *saveptr2, *subtoken;
				char **argv = NULL;
				while (argv == NULL)
					argv = malloc(0);
				int argc = 0;

				for (str2 = token;; str2 = NULL) {
					subtoken = strtok_r(str2, " \t", &saveptr2);
					if (subtoken == NULL)
						break;
					void *i = NULL;
					while (i == NULL)
						i = realloc(argv, sizeof(void*) * (argc + 1));
					argv = i;
					argv[argc++] = subtoken;
				}
				CommandFunc f;
				f = get_command_function(argv[0]);
				if (!f) {
					write(conn->fd,
							"502 Bad command or it is not implemented here.\r\n",
							48);
				} else
					f(conn, argc, argv);
				free(argv);
				if (ntoken == NULL) {
					free(h->buf);
					h->buf = NULL;
					h->nbytes = 0;
					break;
				}
			} else {
				h->nbytes = strlen(token);
				str1 = strdup(token);
				free(h->buf);
				h->buf = str1;
				break;
			}
		}
		token = ntoken;
	}
}

static void ProccessInput(struct ConnectionNode *conn,
		struct ProccessInputHandler *h, char *buf, size_t nbytes) {
	/* we got some data from a client */
	h->nbytes += nbytes;
	printf("socket recv from %s on socket %d index %d\n", conn->host, conn->fd,
			conn->index);
	if (strnlen(buf, nbytes) != nbytes) {
		/* this would be a crash */
		printf("socket to %s send non-string\n", conn->host);
		char e[42];
		sprintf(e, "551 Invalid string %4d bytes discarded\r\n",
				(unsigned int) h->nbytes);
		write(conn->fd, &e, 41);
		if (h->buf)
			free(h->buf);
		h->buf = NULL;
		h->nbytes = 0;
	} else {
		if (h->buf != NULL) {
			char *t = NULL;
			while (t == NULL)
				t = realloc(h->buf, h->nbytes + 1);
			strncat(t, buf, h->nbytes);
			h->buf = t;
		} else
			h->buf = strdup(buf);
		LineLocator(conn, h);
	}
}

static void OpenConnection(int listener, int *fdmax, fd_set *master) { /* we got a new one... */
	/* handle new connections */
	struct ConnectionNode *TempNode = GetNewConnection();
	TempNode->addr_len = sizeof(TempNode->addr);
	if ((TempNode->fd = accept(listener, (struct sockaddr *) &TempNode->addr,
			&TempNode->addr_len)) == -1) {
		perror("Warning accepting one new connection");
		free(TempNode);
	} else {
		if (TempNode->fd > *fdmax) /* keep track of the maximum */
			if (TempNode->fd > FD_SETSIZE) {
				close(TempNode->fd);
				free(TempNode);
				TempNode = NULL;
			} else
				*fdmax = TempNode->fd;

		if (TempNode != NULL) {
			TempNode->getnameinfo = getnameinfo(
					(struct sockaddr *) &TempNode->addr, TempNode->addr_len,
					TempNode->host, NI_MAXHOST, NULL, 0, 0);

			struct ProccessInputHandler *handler = NULL;
			while (handler == NULL)
				handler = malloc(sizeof(struct ProccessInputHandler));
			handler->func = &ProccessInput;
			handler->free = &ProccessInputFree;
			handler->prev = NULL;
			handler->nbytes = 0;
			handler->buf = NULL;

			TempNode->handler = (struct ConnectionNodeHandler *) handler;

			InsertConnectionBefore(&connections_head, TempNode);
			printf("New connection from %s on socket %d index %d\n",
					TempNode->host, TempNode->fd, TempNode->index);

			total_connections++;
			new_connections_faucet.tokens = token_faucet_get(&new_connections_faucet) + 300000;  /* 5min */

			/* add to master set */
			FD_SET(TempNode->fd, master);
		}
	}
}

void EnterListener(struct ListenerOptions *opts) {
	/* master file descriptor list */
	fd_set master;
	/* temp file descriptor list for select() */
	fd_set read_fds;
	/* maximum file descriptor number */
	int fdmax;
	/* listening socket descriptor */
	int listener;
	/* for setsockopt() SO_REUSEADDR, below */
	int yes = 1;
	int j;
	struct addrinfo *result, *rp;

	/* clear the master and temp sets */
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&opts->hints, 0, sizeof(struct addrinfo));
	opts->hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	opts->hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	opts->hints.ai_flags = AI_PASSIVE;

	j = getaddrinfo(opts->nodename, opts->servname, &opts->hints, &result);
	if (j != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(j));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	 Try each address until we successfully bind(2).
	 If socket(2) (or bind(2)) fails, we (close the socket
	 and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
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

	if (rp == NULL) { /* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result); /* No longer needed */

	/* listen */
	if (listen(listener, SOMAXCONN) == -1) {
		perror("Error opening listener");
		exit(EXIT_FAILURE);
	}

	/* add the listener to the master set */
	FD_SET(listener, &master);
	/* keep track of the biggest file descriptor */
	fdmax = listener; /* so far, it's this one */

	/* loop */
	for (;;) {
		/* copy it */
		read_fds = master;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("Error waiting for input");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(listener, &read_fds))
			OpenConnection(listener, &fdmax, &master);

		/* run through the existing connections looking for data to be read */
		struct ConnectionNode *i = connections_head;
		if (connections_head != NULL)
			do {
				if (FD_ISSET(i->fd, &read_fds)) { /* we got one... */
					/* handle data from a client */
					printf("New data from %s on socket %d index %d\n", i->host,
							i->fd, i->index);
					/* buffer for client data */
					char buf[1024];
					int nbytes;
					if ((nbytes = recv(i->fd, buf, sizeof(buf) - 1, 0)) <= 0) {
						/* got error or connection closed by client */
						if (nbytes == 0)
							/* connection closed */
							printf(
									"socket to %s hung up on socket %d index %d\n",
									i->host, i->fd, i->index);
						else
							perror("Negative recv");
						/* close it... */
						close(i->fd);
						total_connections--;
						/* remove from master set */
						FD_CLR(i->fd, &master);
						/* step back and remove this connection */
						i = RemoveConnection(i);
						if (i == NULL)
							break;
					} else {
						/* Ensure this is an ansi string */
						buf[nbytes] = '\0';
						i->handler->func(i, i->handler, buf, nbytes);
					}
				}
				i = i->next;
			} while (i != connections_head);
	}
}
