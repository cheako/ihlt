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

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "ConnectionsDoublyLinkedList.h"
#include "server.h"

/*
 * The function list for clients is stored in a table, and the items each
 * point to a function to call, defined below.
 */

typedef int (*CommandFunc)(struct ConnectionNode *, int, char **);

typedef struct client_function {
	char * keyword;
	CommandFunc function;
} client_function;

CommandFunc get_command_function(char *cmd);

#endif /* PROTOCOL_H_ */
