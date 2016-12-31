/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2016  Michael Mestnik <cheako+github_com@mikemestnik.net>
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
 * https://github.com/osrg/openvswitch/blob/master/lib/token-bucket.h
 */

#ifndef ACCOUNTING_H_
#define ACCOUNTING_H_

struct token_faucet {
	unsigned int rate;
	unsigned int tokens;
	long long int last_drain;
};

extern struct token_faucet new_connections_faucet;
extern unsigned int total_connections;

void token_faucet_init(struct token_faucet *, unsigned int rate);
unsigned int token_faucet_get(struct token_faucet *);

#endif /* ACCOUNTING_H_ */
