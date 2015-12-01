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
 *   http://www.gnutls.org/manual/html_node/Echo-server-with-OpenPGP-authentication.html
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

#include <gnutls/openpgp.h>

/* master read file descriptor list */
static fd_set master_r;
/* master write file descriptor list */
static fd_set master_w;

void _gnutls_record_get_direction(struct ConnectionNode *i) {
	if(gnutls_record_get_direction(i->session) == 1)
		FD_SET(i->fd, &master_w);
	else
		FD_CLR(i->fd, &master_w);
}

int _gnutls_handshake(struct ConnectionNode *i) {
	int ret = gnutls_handshake(i->session);
	if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN)
		_gnutls_record_get_direction(i);
	else if (ret < 0) {
		/* close it... */
		close(i->fd);
		gnutls_deinit(i->session);
		/* remove from master sets */
		FD_CLR(i->fd, &master_r);
		FD_CLR(i->fd, &master_w);
		/* step back and remove this connection */
		fprintf(stderr, "*** Handshake has failed: %s\n",
				gnutls_strerror(ret));
		RemoveConnection(i);
	} else
		printf("- Handshake was completed\n");

	return ret;
}

ssize_t _gnutls_record_recv(struct ConnectionNode *i, void *data,
		size_t data_size) {
	ssize_t ret = gnutls_record_recv(i->session, data, data_size);
	if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN)
		_gnutls_record_get_direction(i);
	return ret;
}

ssize_t _gnutls_record_send(struct ConnectionNode *i, const void *data,
		size_t data_size) {
	ssize_t ret = gnutls_record_send(i->session, data, data_size);
	if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN)
		_gnutls_record_get_direction(i);
	return ret;
}

int gnutls_ready(struct ConnectionNode *i) {
	switch (i->gnutls_state) {
	case _GNUTLS_READY:
		return 0;
	case _GNUTLS_HANDSHAKE:
		return _gnutls_handshake(i);
	case _GNUTLS_RECV:
		return _gnutls_record_recv(i, NULL, 0);
	case _GNUTLS_SEND:
		return _gnutls_record_send(i, NULL, 0);
	}
}

// Send/receive raw data
int sock_send(struct ConnectionNode *i, char *src, size_t size) {
	if (!src)
		return -1;

	return _gnutls_record_send(i, src, size);
}

struct ProccessInputHandler {
	void (*func)(struct ConnectionNode *, char *, size_t);
	void (*free)(struct ConnectionNodeHandler *);
	struct ConnectionNodeHandler *prev;
	char * buf;
	size_t nbytes;
};

void ProccessInputFree(struct ConnectionNodeHandler *h) {
	struct ProccessInputHandler *rm = (struct ProccessInputHandler *) h;
	if (rm->buf != NULL )
		free(rm->buf);
	free(rm);
}

void LineLocator(struct ConnectionNode *conn) {
	/* look for the end of the line */
	struct ProccessInputHandler *h =
			(struct ProccessInputHandler *) conn->handler;
	char *str1, *saveptr1, *ntoken, *token;
	bool endswell = false;
	switch (h->buf[h->nbytes - 1]) {
	case '\r':
	case '\n':
		endswell = true;
		break;
	};

	for (str1 = h->buf;; str1 = NULL ) {
		ntoken = strtok_r(str1, "\r\n", &saveptr1);
		/* Reverse this so we know the next result. */
		if (str1 == NULL ) {
			if (ntoken != NULL || endswell) {
				char *str2, *saveptr2, *subtoken;
				char **argv = NULL;
				while (argv == NULL )
					argv = malloc(0);
				int argc = 0;

				for (str2 = token;; str2 = NULL ) {
					subtoken = strtok_r(str2, " \t", &saveptr2);
					if (subtoken == NULL )
						break;
					void *i = NULL;
					while (i == NULL )
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
				if (ntoken == NULL ) {
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

void ProccessInput(struct ConnectionNode *conn, char *buf, size_t nbytes) {
	/* we got some data from a client */
	struct ProccessInputHandler *h =
			(struct ProccessInputHandler *) conn->handler;
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
		if (h->buf != NULL ) {
			char *t = NULL;
			while (t == NULL )
				t = realloc(h->buf, h->nbytes + 1);
			strncat(t, buf, h->nbytes);
			h->buf = t;
		} else
			h->buf = strdup(buf);
		LineLocator(conn);
	}
}

void OpenConnection(int listener, int *fdmax) { /* we got a new one... */
	/* handle new connections */
	struct ConnectionNode *TempNode = GetNewConnection();
#ifdef GNUTLS_NONBLOCK
	gnutls_init(&TempNode->session, GNUTLS_SERVER || GNUTLS_NONBLOCK);
#else
	gnutls_init(&TempNode->session, GNUTLS_SERVER);
#endif
	gnutls_priority_set_direct(TempNode->session, "NORMAL:+CTYPE-OPENPGP",
			NULL );
	gnutls_certificate_server_set_request(TempNode->session,
			GNUTLS_CERT_REQUEST);

	TempNode->addr_len = sizeof(TempNode->addr);
	if ((TempNode->fd = accept(listener, (struct sockaddr *) &TempNode->addr,
			&TempNode->addr_len)) == -1) {
		perror("Warning accepting one new connection");
		free(TempNode);
	} else {
		if (TempNode != NULL && TempNode->fd > *fdmax) /* keep track of the maximum */
			if (TempNode->fd > FD_SETSIZE) {
				close(TempNode->fd);
				gnutls_deinit(TempNode->session);
				free(TempNode);
				TempNode = NULL;
			} else
				*fdmax = TempNode->fd;

		if (TempNode != NULL ) {
			TempNode->getnameinfo = getnameinfo(
					(struct sockaddr *) &TempNode->addr, TempNode->addr_len,
					TempNode->host, NI_MAXHOST, NULL, 0, 0);

			struct ProccessInputHandler *handler = NULL;
			while (handler == NULL )
				handler = malloc(sizeof(struct ProccessInputHandler));
			bzero(handler, sizeof(struct ProccessInputHandler));

			handler->func = &ProccessInput;
			handler->free = &ProccessInputFree;

			TempNode->handler = (struct ConnectionNodeHandler *) handler;

			InsertConnectionBefore(&connections_head, TempNode);
			printf("New connection from %s on socket %d index %d\n",
					TempNode->host, TempNode->fd, TempNode->index);

			/* add to master set */
			FD_SET(TempNode->fd, &master_r);
		}

#if GNUTLS_VERSION_NUMBER >= 0x030109
		gnutls_transport_set_int(TempNode->session, TempNode->fd);
#else
#error need at least 3.1.9 gnutls: GNUTLS_VERSION_NUMBER
#endif
		_gnutls_handshake(TempNode);
	}
}

void EnterListener(struct ListenerOptions *opts) {
	/* temp file descriptor lists for select() */
	fd_set read_fds, write_fds;
	/* maximum file descriptor number */
	int fdmax;
	/* listening socket descriptor */
	int listener;
	/* for setsockopt() SO_REUSEADDR, below */
	int yes = 1;
	int j;
	struct addrinfo *result, *rp;

	/* clear the master and temp sets */
	FD_ZERO(&master_r);
	FD_ZERO(&master_w);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

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
		exit(EXIT_FAILURE);
	}

	/* add the listener to the master set */
	FD_SET(listener, &master_r);
	/* keep track of the biggest file descriptor */
	fdmax = listener; /* so far, it's this one */

	/* loop */
	for (;;) {
		read_fds = master_r;
		write_fds = master_w;

		if (select(fdmax + 1, &read_fds, &write_fds, NULL, NULL ) == -1) {
			perror("Error waiting for input");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(listener, &read_fds))
			OpenConnection(listener, &fdmax);

		/* run through the existing connections looking for data to be read */
		struct ConnectionNode *i = connections_head;
		if (connections_head != NULL )
			do {
				if (FD_ISSET(i->fd, &read_fds)) { /* we got one... */
					/* handle data from a client */
					printf("New data from %s on socket %d index %d\n", i->host,
							i->fd, i->index);
					/* buffer for client data */
					char buf[1024];
					int nbytes;
					nbytes = gnutls_record_recv(i->session, buf, 1023);

					if (nbytes == 0) {
						/* connection closed */
						printf("socket to %s hung up on socket %d index %d\n",
								i->host, i->fd, i->index);
						/* close it... */
						close(i->fd);
						gnutls_deinit(i->session);
						/* remove from master sets */
						FD_CLR(i->fd, &master_r);
						FD_CLR(i->fd, &master_w);
						/* step back and remove this connection */
						i = RemoveConnection(i);
						if (i == NULL )
							break;
					} else if (nbytes < 0
							&& gnutls_error_is_fatal(nbytes) == 0) {
						fprintf(stderr, "*** Warning: %s\n",
								gnutls_strerror(nbytes));
					} else if (nbytes < 0) {
						printf(
								"socket to %s received corrupted data(%d) on socket %d index %d, closing the connection.\n",
								i->host, nbytes, i->fd, i->index);
						/* close it... */
						close(i->fd);
						gnutls_deinit(i->session);
						/* remove from master sets */
						FD_CLR(i->fd, &master_r);
						FD_CLR(i->fd, &master_w);
						/* step back and remove this connection */
						i = RemoveConnection(i);
						if (i == NULL )
							break;
					} else if (nbytes > 0) {
						/* Ensure this is an ansi string */
						buf[nbytes] = '\0';
						i->handler->func(i, buf, nbytes);
					}
				}
				i = i->next;
			} while (i != connections_head);
	}
}
