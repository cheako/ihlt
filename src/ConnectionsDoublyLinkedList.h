/* https://gist.github.com/mycodeschool/7429492 */
/* Doubly Linked List implementation */

#ifndef ConnectionsDoublyLinkedList_H_
#define ConnectionsDoublyLinkedList_H_

#include <netdb.h>
#include <gnutls/gnutls.h>

enum gnutls_state {
	_GNUTLS_READY,
	_GNUTLS_HANDSHAKE,
	_GNUTLS_RECV,
	_GNUTLS_SEND,
};
struct ConnectionNodeHandler;
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
	char host[NI_MAXHOST];
	gnutls_session_t session;
	enum gnutls_state gnutls_state;
	struct ConnectionNodeHandler {
		void (*func)(struct ConnectionNode *, char *, size_t);
		void (*free)(struct ConnectionNodeHandler *);
		struct ConnectionNodeHandler *prev;
	} *handler;
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
