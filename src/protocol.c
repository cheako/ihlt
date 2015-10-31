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
 *   https://github.com/bytemine/lcdproc/tree/master/server/commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

/*****************************************************************************
 * The client should say "bye" before disconnecting
 *
 * The function does not respond to the client: it simply cuts connection
 *
 * Usage: bye
 */
int quit_func(struct ConnectionNode *conn, int argc, char **argv) {
	exit(EXIT_SUCCESS);
	return 0;
}

/***************************************************************
 * Debugging only..  prints out a list of arguments it receives
 */
int echo_func(struct ConnectionNode *conn, int argc, char **argv) {
	int i;
	size_t n;
	char *t = NULL;

	printf("socket send to %s on socket %d index %d\n", conn->host, conn->fd,
			conn->index);
	while (t == NULL )
		t = strdup("211 pong:");
	for (i = 1; i < argc; i++) {
		char *r = NULL;
		n = strlen(t) + strlen(argv[i]) + 4;
		while (r == NULL )
			r = realloc(t, n);
		t = r;
		strncat(t, " ", n);
		strncat(t, argv[i], n);
	}
	strncat(t, "\r\n", n);
	sock_send(conn->fd, t, strlen(t));
	free(t);
	return 0;
}

int wall_func(struct ConnectionNode *conn, int argc, char **argv) {
	int i;
	size_t n;
	char *t = NULL;

	while (t == NULL )
		t = strdup("211 wall:");
	for (i = 1; i < argc; i++) {
		char *r = NULL;
		n = strlen(t) + strlen(argv[i]) + 4;
		while (r == NULL )
			r = realloc(t, n);
		t = r;
		strncat(t, " ", n);
		strncat(t, argv[i], n);
	}
	strncat(t, "\r\n", n);
	struct ConnectionNode *dest;
	for (dest = conn->next; dest != conn; dest = dest->next) {
		char *r = NULL;
		printf("socket send to %s on socket %d index %d\n", dest->host,
				dest->fd, dest->index);
		sock_send(dest->fd, t, strlen(t));
	}
	free(t);
	return 0;
}

int prev_func(struct ConnectionNode *conn, int argc, char **argv) {
	int i;
	size_t n;
	char *t = NULL;

	printf("socket send to %s on socket %d index %d\n", conn->host,
			conn->prev->fd, conn->prev->index);
	while (t == NULL )
		t = strdup("211 next:");
	for (i = 1; i < argc; i++) {
		char *r = NULL;
		n = strlen(t) + strlen(argv[i]) + 4;
		while (r == NULL )
			r = realloc(t, n);
		t = r;
		strncat(t, " ", n);
		strncat(t, argv[i], n);
	}
	strncat(t, "\r\n", n);
	sock_send(conn->prev->fd, t, strlen(t));
	free(t);
	return 0;
}

int next_func(struct ConnectionNode *conn, int argc, char **argv) {
	int i;
	size_t n;
	char *t = NULL;

	printf("socket send to %s on socket %d index %d\n", conn->next->host,
			conn->next->fd, conn->next->index);
	while (t == NULL )
		t = strdup("211 prev:");
	for (i = 1; i < argc; i++) {
		char *r = NULL;
		n = strlen(t) + strlen(argv[i]) + 4;
		while (r == NULL )
			r = realloc(t, n);
		t = r;
		strncat(t, " ", n);
		strncat(t, argv[i], n);
	}
	strncat(t, "\r\n", n);
	sock_send(conn->next->fd, t, strlen(t));
	free(t);
	return 0;
}

static client_function commands[] = { { "ping", echo_func },
		{ "quit", quit_func }, { "wall", wall_func }, { "prev", prev_func }, {
				"next", next_func }, { NULL, NULL }, };

CommandFunc get_command_function(char *cmd) {
	int i;

	if (cmd == NULL )
		return NULL ;

	for (i = 0; commands[i].keyword != NULL ; i++)
		if (0 == strcmp(cmd, commands[i].keyword))
			return commands[i].function;

	return NULL ;
}
