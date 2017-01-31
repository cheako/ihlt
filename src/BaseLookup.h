/*  ihlt hopefully the last tracker: seed your network.
 *  Copyright (C) 2016,2017  Michael Mestnik <cheako+github_com@mikemestnik.net>
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

#ifndef BaseLookup_H_
#define BaseLookup_H_

#include <stddef.h>
#include <stdbool.h>

struct Base {
	size_t depth;
	bool high;
	unsigned char name[];
};

struct BaseTable {
	void *data;
	struct BaseTable *next[16];
	struct Base base;
};

//Find a Base
extern struct BaseTable *LookupBase(struct Base *);

//Remove a Base Table
extern void DeleteBase(struct BaseTable **);

struct BaseFind {
	int n;
	struct BaseSeeker {
		struct BaseTable *b;
		signed char i;
		bool f;
	} s[];
};

/* Adjust for index vs numbered count */
#define BASE_FIND_CURRENT(f) ((f).s[(f).n - 1])

extern struct BaseFind *BaseFindInit(struct BaseTable *);

extern bool BaseFindNext(struct BaseFind **);

extern bool BaseFindPrev(struct BaseFind **);

extern struct Base *StrToBase(char *);

#endif /* BaseLookup_H_ */
