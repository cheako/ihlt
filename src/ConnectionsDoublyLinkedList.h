/* https://gist.github.com/mycodeschool/7429492 */
/* Doubly Linked List implementation */

#ifndef ConnectionsDoublyLinkedList_H_
#define ConnectionsDoublyLinkedList_H_

#include <netdb.h>

struct ConnectionNode {
	struct ConnectionNode * prev;
	struct ConnectionNode * next;
	int index;
	/* newly accept()ed socket descriptor */
	int fd;
	int getnameinfo;
	/* client address */
	struct sockaddr_storage addr;
	socklen_t addr_len;
	char * buf;
	size_t nbytes;
	char host[NI_MAXHOST];
};

#define CONNECTION_NODE_SIZE sizeof(struct ConnectionNode)

// global variable - pointer to assumed lowest node.
extern struct ConnectionNode *connections_head;

//Creates a new Node and returns pointer to it.
extern struct ConnectionNode *GetNewConnection();

//Inserts a Connection into doubly linked list
extern void InsertConnectionBefore(struct ConnectionNode **,
		struct ConnectionNode *);

//Removes a Connection from doubly linked list
extern struct ConnectionNode *RemoveConnection(struct ConnectionNode *);

#endif /* ConnectionsDoublyLinkedList_H_ */
