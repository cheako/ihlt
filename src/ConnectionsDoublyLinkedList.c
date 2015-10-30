/* https://gist.github.com/mycodeschool/7429492 */
/* Doubly Linked List implementation */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "ConnectionsDoublyLinkedList.h"

//Creates a new Node and returns pointer to it.
struct ConnectionNode *GetNewConnection() {
	static int next_index = 0;
	struct ConnectionNode *newNode = NULL;

	while (newNode == NULL )
		newNode = malloc(CONNECTION_NODE_SIZE);

	bzero(newNode, CONNECTION_NODE_SIZE);
	newNode->index = next_index++;
	return newNode;
}

struct ConnectionNode *connections_head = NULL;

//Inserts a Connection into doubly linked list.
void InsertConnectionBefore(struct ConnectionNode **a, struct ConnectionNode *b) {
	if (*a == NULL ) {
		b->prev = b;
		b->next = b;
		*a = b;
		return;
	}
	if (connections_head == NULL )
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

//Removes a Connection from doubly linked list.
struct ConnectionNode *RemoveConnection(struct ConnectionNode *rm) {
	struct ConnectionNode *r;
	if (rm->buf != NULL )
		free(rm->buf);
	if (rm == rm->prev) {
		if (rm == connections_head)
			connections_head = NULL;
		free(rm);
		return NULL ;
	}
	rm->prev->next = rm->next;
	rm->next->prev = rm->prev;
	if (connections_head == rm)
		connections_head = rm->next;
	r = rm->prev;
	free(rm);
	return r;
}
