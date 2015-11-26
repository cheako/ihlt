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

/* Example from:
 *   https://github.com/bytemine/lcdproc/tree/master/server/commands
 */

#include <stdlib.h>
#include <string.h>

#include "protocol.h"

struct client_function *commands;

void register_commands(const struct client_function insert[]) {
	int sizeofa, sizeofb;
	struct client_function *nu = NULL;

	for (sizeofb = 0; insert[sizeofb++].keyword != NULL ;)
		;

	if (commands != NULL ) {
		size_t i[2];
		i[0] = sizeofb * sizeof(struct client_function);

		for (sizeofa = 0; commands[sizeofa].keyword != NULL ; sizeofa++)
			;
		i[1] = sizeofa * sizeof(struct client_function);
		i[2] = i[0] + i[1];

		while (nu == NULL )
			nu = realloc(commands, i[2]);

		memcpy(nu + sizeofa, insert, i[0]);

		commands = nu;
	} else {
		size_t i = sizeofb * sizeof(struct client_function);

		while (nu == NULL )
			nu = malloc(i);

		memcpy(nu, insert, i);

		commands = nu;
	}
}

CommandFunc get_command_function(char *cmd) {
	int i;

	if (cmd == NULL )
		return NULL ;

	for (i = 0; commands[i].keyword != NULL ; i++)
		if (0 == strcmp(cmd, commands[i].keyword))
			return commands[i].function;

	return NULL ;
}
