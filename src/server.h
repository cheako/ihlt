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
 *
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <sys/types.h>
#include "protocol.h"

extern int sock_send(struct ConnectionNode *, char *, size_t);

struct ListenerOptions {
	char *nodename;
	char *servname;
	/* server address */
	struct addrinfo hints;
};

extern void EnterListener(struct ListenerOptions *);

#endif /* SERVER_H_ */
