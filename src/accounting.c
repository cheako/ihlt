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
 * https://github.com/osrg/openvswitch/blob/master/lib/token-bucket.c
 */

#include <limits.h>
#include <time.h>

#include "accounting.h"

struct token_faucet new_connections_faucet;
unsigned int total_connections;

void token_faucet_init(struct token_faucet *tf, unsigned int rate) {
	tf->rate = rate;
	tf->tokens = 0;
	tf->last_drain = LLONG_MIN;
}

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

/* Saturating multiplication of "unsigned int"s: overflow yields UINT_MAX. */
#define SAT_MUL(X, Y)                                                   \
    ((Y) == 0 ? 0                                                       \
     : (X) <= UINT_MAX / (Y) ? (unsigned int) (X) * (unsigned int) (Y)  \
     : UINT_MAX)
static inline unsigned int sat_mul(unsigned int x, unsigned int y) {
	return SAT_MUL(x, y);
}

unsigned int token_faucet_get(struct token_faucet *tf) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	long long int now = (long long int) ts.tv_sec * 1000
			+ ts.tv_nsec / (1000 * 1000);
	if (now > tf->last_drain) {
		unsigned long long int elapsed_ull = (unsigned long long int) now
				- tf->last_drain;
		unsigned int elapsed = MIN(UINT_MAX, elapsed_ull);
		unsigned int sub = sat_mul(tf->rate, elapsed);
		if (sub > tf->tokens) {
			tf->tokens = 0;
		} else
			tf->tokens -= sub;
		tf->last_drain = now;
	}
	return tf->tokens;
}
