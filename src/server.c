/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2015  Michael Mestnik <cheako+github_com@mikemestnik.net>
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
#include <string.h>
/*! @brief port we're listening on */
#define PORT "4458"

/*! @brief program entry point */
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

		/* run through the existing connections looking for data to be read */
		int i;
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { /* we got one... */
				if (i == listener) {
					/* handle new connections */
					/* client address */
					struct sockaddr_storage clientaddr;
					socklen_t addrlen;
					/* newly accept()ed socket descriptor */
					int newfd;
					addrlen = sizeof(clientaddr);
					if ((newfd = accept(listener,
							(struct sockaddr *) &clientaddr, &addrlen)) == -1) {
						perror("Warning accepting one new connection");
					} else {
						/* add to master set */
						if (newfd > fdmax) /* keep track of the maximum */
							if (newfd > FD_SETSIZE) {
								close(newfd);
								continue;
							} else
								fdmax = newfd;

						FD_SET(newfd, &master);
						char host[NI_MAXHOST], service[NI_MAXSERV];

						j = getnameinfo((struct sockaddr *) &clientaddr,
								addrlen, host, NI_MAXHOST, service, NI_MAXSERV,
								NI_NUMERICSERV);
						if (j != 0) {
							fprintf(stderr, "getnameinfo: %s\n",
									gai_strerror(j));
						}
						printf("%s: New connection from %s on socket %d\n",
								argv[0], host, newfd);
					}
				} else {
					/* handle data from a client */
					/* buffer for client data */
					char buf[1024];
					int nbytes;
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						/* got error or connection closed by client */
						if (nbytes == 0)
							/* connection closed */
							printf("%s: socket %d hung up\n", argv[0], i);
						else
							perror("Negative recv");
						/* close it... */
						close(i);
						/* remove from master set */
						FD_CLR(i, &master);
					} else {
						/* we got some data from a client */
						for (j = 0; j <= fdmax; j++) {
							/* send to everyone! */
							if (FD_ISSET(j, &master)) {
								/* except the listener and ourselves */
								if (j != listener && j != i)
									if (send(j, buf, nbytes, 0) == -1)
										perror("send() error lol!");
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
