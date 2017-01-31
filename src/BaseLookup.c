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

#include <stdlib.h>
#include <string.h>

#include "BaseLookup.h"
#include "cdecode.h"

// #include <stdio.h>

static struct BaseTable base_head;

struct BaseTable *NewBaseTable(struct Base *base) {
	struct BaseTable *ret = NULL;
	while (ret == NULL ) /* This might be a place we wish to bail? */
		ret = malloc(sizeof(struct BaseTable) + base->depth);
	ret->data = NULL;
	unsigned char i;
	for (i = 0; i <= 15; i++)
		ret->next[i] = NULL;
	ret->base.depth = base->depth;
	ret->base.high = base->high;
	memcpy(&ret->base.name, &base->name, base->depth);
	if (base->high)
		ret->base.name[base->depth] &= 0xF0;
	return ret;
}

static inline unsigned char GetNibble(unsigned char c, bool h) {
	return h ? (c & 0xF0) >> 4 : (c & 0x0F);
}

struct BaseTable *LookupBase(struct Base *base) {
	if (base->depth == 0)
		return &base_head;
	struct BaseTable **lastptr = &base_head.next[GetNibble(base->name[0], true)];
	while (1) {
		if (*lastptr == NULL ) {
			struct BaseTable *new = NewBaseTable(base);
			*lastptr = new;
			return new;
		} else {
			if ((*lastptr)->base.depth > base->depth
					|| ((*lastptr)->base.depth == base->depth
							&& (*lastptr)->base.high < base->high)) {
				struct BaseTable *new = NewBaseTable(base);
				new->next[GetNibble((*lastptr)->base.name[base->depth],
						base->high)] = *lastptr;
				*lastptr = new;
				return new;
			} else if ((*lastptr)->base.depth < base->depth
					|| ((*lastptr)->base.depth == base->depth
							&& (*lastptr)->base.high > base->high)) {
				lastptr = &((*lastptr)->next[GetNibble(
						base->name[(*lastptr)->base.depth],
						(*lastptr)->base.high)]);
				continue;
			} else {
				size_t i;
				bool h;
				for (i = 0;
						i < base->depth
								&& base->name[i] == (*lastptr)->base.name[i];
						i++)
					;
				if (i == base->depth) {
					return *lastptr;
				} else {
					struct Base *t = NULL;
					while (t == NULL )
						t = malloc(sizeof(struct Base) + i);
					t->depth = i;
					t->high = base->name[i] & 0xF0 != (*lastptr)->base.name[i]
							& 0xF0;
					memcpy(&t->name, &base->name, i);
					struct BaseTable *new = NewBaseTable(t);
					new->next[GetNibble((*lastptr)->base.name[i], t->high)] =
							*lastptr;
					struct BaseTable *ret = NewBaseTable(base);
					new->next[GetNibble(ret->base.name[i], t->high)] = ret;
					*lastptr = new;
					free(t);
					return ret;
				}
			}
		}
	}
}

//Remove a Base Table
void DeleteBase(struct BaseTable ** del) {
	// Lots of asserts.
	// There can be one or less.
	// Where to assign one?
}

struct BaseFind *BaseFindInit(struct BaseTable *b) {
	struct BaseFind *ret = NULL;
	while (ret == NULL )
		ret = malloc(sizeof(struct BaseFind) + sizeof(struct BaseSeeker));
	ret->n = 1;
	ret->s[0].i = 0;
	ret->s[0].f = true;
	ret->s[0].b = b;
}

bool BaseFindNext(struct BaseFind **s) {
	struct BaseSeeker *stacki = &BASE_FIND_CURRENT(**s);
	while(1) {
		if(stacki->i > 15) {
			if(stacki->f) {
				stacki->f = false;
				return(true);
			}
			(*s)->n--;
			struct BaseFind *ret = NULL;
			while(ret == NULL)
			ret = realloc(*s, sizeof(struct BaseFind) + sizeof(struct BaseSeeker) * (*s)->n);
			*s = ret;

			if((*s)->n == 0) return(false);
			stacki = &BASE_FIND_CURRENT(**s);
		} else {
			struct BaseTable *canidate = stacki->b->next[stacki->i++];
			if(canidate != NULL) {
				(*s)->n++; /* Can't be too high or we'd be out of mem. */
				struct BaseFind *ret = NULL;
				while(ret == NULL)
					ret = realloc(*s, sizeof(struct BaseFind) + sizeof(struct BaseSeeker) * (*s)->n);
				*s = ret;

				stacki = &BASE_FIND_CURRENT(**s);

				stacki->i = 0;
				stacki->f = true;
				stacki->b = canidate;
			}
		}
	}
}

bool BaseFindPrev(struct BaseFind **s) {
	struct BaseSeeker *stacki = &BASE_FIND_CURRENT(**s);
	while(1) {
		if(stacki->i < 0) {
			if(stacki->f) {
				stacki->f = false;
				return(true);
			}
			(*s)->n--;
			struct BaseFind *ret = NULL;
			while(ret == NULL)
			ret = realloc(*s, sizeof(struct BaseFind) + sizeof(struct BaseSeeker) * (*s)->n);
			*s = ret;

			if((*s)->n == 0) return(false);
			stacki = &BASE_FIND_CURRENT(**s);
		} else {
			struct BaseTable *canidate = stacki->b->next[stacki->i--];
			if(canidate != NULL) {
				(*s)->n++; /* Can't be too high or we'd be out of mem. */
				struct BaseFind *ret = NULL;
				while(ret == NULL)
				ret = realloc(*s, sizeof(struct BaseFind) + sizeof(struct BaseSeeker) * (*s)->n);
				*s = ret;

				stacki = &BASE_FIND_CURRENT(**s);

				stacki->i = 15;
				stacki->f = true;
				stacki->b = canidate;
			}
		}
	}
}

struct Base *StrToBase(char *arg) {
	char r[] = "AAAAAAAAAAAAAAAAAAAAAA==";
	base64_decodestate state_in;
	size_t len = strlen(arg);
	struct Base *ret = NULL;
	while (ret == NULL )
		ret = malloc(sizeof(struct Base) + 16);
	ret->depth = 16;
	ret->high = false;
	memcpy(&r, arg, len < 24 ? len : 24);
	base64_init_decodestate(&state_in);
	base64_decode_block(r, 24, ret->name, &state_in);
	return ret;
}
