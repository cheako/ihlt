/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2016,2017  Michael Mestnik <cheako+github_com@mikemestnik.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "channel_commands.h"
#include "BaseLookup.h"
#include "accounting.h"

struct channel_handler {
	void (*func)(struct ConnectionNode *, struct channel_handler *, char *,
			size_t);
	void (*free)(struct ConnectionNode *, struct channel_handler *);
	struct ConnectionNodeHandler *prev;
	fd_set *fds;
};

void channel_handler_func(struct ConnectionNode *i, struct channel_handler *h,
		char *buf, size_t nbytes) {
	if (h->prev != NULL )
		h->prev->func(i, h->prev, buf, nbytes);
}

void channel_handler_free(struct ConnectionNode *i, struct channel_handler *h) {
	if (h->prev != NULL )
		h->prev->free(i, h->prev);
	FD_CLR(i->fd, h->fds);
	free(h);
}

struct channel_data {
	fd_set fd;
	struct token_faucet tf;
};

struct channel_data *channel_get_data(char *arg) {
	struct Base *b = StrToBase(arg);
	struct BaseTable *baset = LookupBase(b);
	free(b);
	if (baset->data == NULL ) {
		while (baset->data == NULL )
			baset->data = malloc(sizeof(struct channel_data));
		FD_ZERO(&((struct channel_data *)(baset->data))->fd);
		token_faucet_init(&((struct channel_data *) (baset->data))->tf, 1);
	}
	return (struct channel_data *) (baset->data);
}

int listen_func(struct ConnectionNode *conn, int argc, char **argv) {
	struct channel_data *r;
	char t[128];
	struct channel_handler *h = NULL;
	printf("listen \"%s\" %s on socket %d index %d\n", argv[1], conn->host,
			conn->fd, conn->index);
	r = channel_get_data(argv[1]);
	FD_SET(conn->fd, &r->fd);

	while (h == NULL )
		h = malloc(sizeof(struct channel_handler));
	h->func = &channel_handler_func;
	h->free = &channel_handler_free;
	h->prev = conn->handler;
	conn->handler = (struct ConnectionNodeHandler *) h;
	h->fds = &r->fd;

	r->tf.tokens = token_faucet_get(&r->tf) + 300000;
	sprintf(t,
			"211 listen: total connections(%d) connection rate(%d) listen rate(%d)\n",
			total_connections, token_faucet_get(&new_connections_faucet),
			token_faucet_get(&r->tf));
	sock_send(conn->fd, t, strlen(t));
	return 0;
}

int send_func(struct ConnectionNode *conn, int argc, char **argv) {
	int i;
	size_t n;
	char *t = NULL;
	struct channel_data *data;
	printf("send \"%s\" %s on socket %d index %d\n", argv[1], conn->host,
			conn->fd, conn->index);
	data = channel_get_data(argv[1]);

	while (t == NULL )
		t = strdup("211 send:");
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
		if (FD_ISSET(dest->fd, &data->fd)) {
			printf("socket send to %s on socket %d index %d\n", dest->host,
					dest->fd, dest->index);
			sock_send(dest->fd, t, strlen(t));
		}
	}
	free(t);
	return 0;
}

const static struct client_function channel_commands[] = { { "listen",
		listen_func }, { "send", send_func }, { NULL, NULL }, };

void init_channel() {
	register_commands(channel_commands);
}
