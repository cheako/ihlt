/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2017  Michael Mestnik <cheako+github_com@mikemestnik.net>
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
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "storage_commands.h"
#include "BaseLookup.h"
#include "accounting.h"

struct index {
	unsigned char rating;
	uint32_t time;
	struct in6_addr address;
	in_port_t port;
};

int store_func(struct ConnectionNode *conn, int argc, char **argv) {
	if (argc < 6) {
		sock_send(conn->fd, "501 Too few parameters.\r\n", 25);
		return -1;
	}
	if (argc > 7) {
		sock_send(conn->fd, "504 Too many parameters.\r\n", 26);
		return -1;
	}
	printf("store \"%s\" %s on socket %d index %d\n", argv[1], conn->host,
			conn->fd, conn->index);

	struct index t;

	int n;
	n = sscanf(argv[2], "%hhu", &t.rating);
	if (n != 1) {
		sock_send(conn->fd, "501 Invalid rating.\r\n", 21);
		return -1;
	}
	t.rating |= 0xf0;
	t.rating <<= 4;

	/* TODO: Make sure this is not too far into the future. */
	t.time = htonl(atol(argv[3]));
	if (t.time == 0) {
		sock_send(conn->fd, "501 Invalid time.\r\n", 19);
		return -1;
	}
	t.time ^= 0xffffffff; /* Reverse sort. */

	if (!inet_pton(AF_INET6, argv[4], &t.address)) {
		sock_send(conn->fd, "501 Invalid network address.\r\n", 30);
		return -1;
	}

	t.port = htons(atoi(argv[5]));
	if (t.port == 0) {
		sock_send(conn->fd, "501 Invalid port.\r\n", 19);
		return -1;
	}

	struct Base *base = StrToBase(argv[1]);
	void *ret = NULL;
	while (ret == NULL )
		ret = realloc(base, sizeof(struct Base) + 16 + sizeof(t));
	base = ret;
	base->depth = 16 + sizeof(t);
	base->high = false;
	memcpy(base->name + 16, &t, sizeof(t));

	struct BaseTable *baset = LookupBase(base);
	baset->data = strndup(argc == 5 ? "" : argv[6], 511); /* argv[6] is for application use */

	char buf[4095];
	sprintf(buf, "220 Saved%s%s\r\n", argc == 5 ? "" : " ", baset->data);
	sock_send(conn->fd, buf, strlen(buf));

	return 0;
}

int load_func(struct ConnectionNode *conn, int argc, char **argv) {
	size_t n;
	char *t = NULL;
	printf("load \"%s\" %s on socket %d index %d\n", argv[1], conn->host,
			conn->fd, conn->index);

	struct Base *base = StrToBase(argv[1]);
	struct BaseFind *finder = BaseFindInit(LookupBase(base));
	free(base);

	struct BaseTable *b;
	do {
		if (!BaseFindNext(&finder)) {
			free(finder);
			sock_send(conn->fd, "450 No records found.\r\n", 23);
			return -1;
		}
		b = BASE_FIND_CURRENT(*finder).b;
	} while(b->base.depth != 16 + sizeof(struct index)
			|| b->base.high != false || b->data == NULL);
	free(finder);

	struct index *tmp = (void *) b->base.name + 16;

	char address_c[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &tmp->address, address_c, sizeof(address_c));

	char buf[4095];
	sprintf(buf, "220 %s is %d %d %s %d%s%s\r\n", argv[1], (tmp->rating >> 4),
			ntohl(tmp->time ^ 0xffffffff), address_c, ntohs(tmp->port),
			strlen(b->data) > 0 ? " " : "", b->data);
	sock_send(conn->fd, buf, strlen(buf));

	return 0;
}

const static struct client_function storage_commands[] = {
		{ "store", store_func }, { "load", load_func }, { NULL, NULL }, };

void init_storage() {
	register_commands(storage_commands);
}
