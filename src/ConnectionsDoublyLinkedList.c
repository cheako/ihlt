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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "ConnectionsDoublyLinkedList.h"

struct ConnectionNode *GetNewConnection() {
	static int next_index = 0;
	struct ConnectionNode *newNode = NULL;

	while (newNode == NULL)
		newNode = malloc(CONNECTION_NODE_SIZE);

	bzero(newNode, CONNECTION_NODE_SIZE);
	newNode->index = next_index++;
	return newNode;
}

struct ConnectionNode *connections_head = NULL;

void InsertConnectionBefore(struct ConnectionNode **a, struct ConnectionNode *b) {
	if (*a == NULL) {
		b->prev = b;
		b->next = b;
		*a = b;
		return;
	}
	if (connections_head == NULL)
		connections_head = *a;
	if (*a != b) {
		b->prev = (*a)->prev;
		b->prev->next = b;
		b->next = *a;
		b->next->prev = b;
	} else {
		exit(3);
	}
}

struct ConnectionNode *RemoveConnection(struct ConnectionNode *rm) {
	struct ConnectionNode *r;
	if (rm->buf != NULL)
		free(rm->buf);
	if (rm == rm->prev) {
		if (rm == connections_head)
			connections_head = NULL;
		free(rm);
		return NULL;
	}
	rm->prev->next = rm->next;
	rm->next->prev = rm->prev;
	if (connections_head == rm)
		connections_head = rm->next;
	r = rm->prev;
	free(rm);
	return r;
}
