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
 * @file ConnectionsDoublyLinkedList.h
 * @brief Doubly Linked List used for each connection
 * @author Mike Mestnik
 *
 * Example from:
 * - [https://gist.github.com/mycodeschool/7429492]
 */

#ifndef ConnectionsDoublyLinkedList_H_
#define ConnectionsDoublyLinkedList_H_

#include <netdb.h>

/*! @brief used for each connection */
struct ConnectionNode {
	struct ConnectionNode * prev; /*! @brief DDL. */
	struct ConnectionNode * next; /*! @brief DDL. */
	int index; /*! @brief counter not used */
	int fd; /*! @brief newly accept()ed socket descriptor */
	int getnameinfo; /*! @brief function return value not used */
	struct sockaddr_storage addr; /*! @brief client address */
	socklen_t addr_len; /*! @brief length of address buffer */
	char * buf; /*! @brief input buffer */
	size_t nbytes; /*! @brief size */
	char host[NI_MAXHOST]; /*! @brief string returned from getnameinfo */
};

/*! @brief how big is out mighty hero? */
#define CONNECTION_NODE_SIZE sizeof(struct ConnectionNode)

/*! @brief pointer to assumed lowest node. */
extern struct ConnectionNode *connections_head;

/*! @brief creates a new Node and returns pointer to it. */
struct ConnectionNode *GetNewConnection();

/*! @brief inserts a Connection into doubly linked list */
void InsertConnectionBefore(struct ConnectionNode **, struct ConnectionNode *);

/*! @brief removes a Connection from doubly linked list */
struct ConnectionNode *RemoveConnection(struct ConnectionNode *);

#endif /* ConnectionsDoublyLinkedList_H_ */
